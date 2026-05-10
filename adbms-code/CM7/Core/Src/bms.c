#include "bms.h"

mainboard_ mainboard;

void Bms_Mainboard_Setup(SPI_HandleTypeDef *hspi, FDCAN_HandleTypeDef *hcan)
{
	// initialize handles
	mainboard.hcan = hcan;

	// initialize ad chip;
	ADBMS_Initialize(&mainboard.adbms_raw, hspi);

	// initialize CAN;
	Bms_Initialize_Can(&mainboard);

	// initialize SOC
	Soc_Initialize(&mainboard);

	// initialize the timers: adbms_mainboard_loop, drive_can, data_can
	timer_ t_adbms = Create_Timer(200, bms_mainboard_loop);
	timer_ t_adbms_owc_check = Create_Timer(30000, adbms_owc_loop);
	timer_ timers[NUM_TIMERS] = {t_adbms, t_adbms_owc_check};
	mainboard.tg = Create_Timer_Group(timers);

	mainboard.start_time = HAL_GetTick();

	//initial states
	mainboard.charging_state = charger_setup;
	mainboard.internal_state = Idle;
	mainboard.imd_status = true; //start healthy because fault is latched (maybe not needed in software)
}


void Tick_Mainboard_Timers()
{
	Tick_Timer_Group(mainboard.tg);
}

// ADBMS loop that gets ticked
void bms_mainboard_loop()
{
    //this loops need i think a lot of changes (because the entire structure chnanged)
	update_values();
	check_faults();
	Control_Loop(&mainboard);

	Can_Loop();
	if(ENABLE_PRINTF_DEBUG_COMMS) send_data_over_printf(); 
	if(ENABLE_USB_COMMS) send_data_over_USB(); 

}

// Seprate loop that gets ticked to run OWC
void adbms_owc_loop()
{ 
	ADBMS_WakeUP_ICs();

	cell_Balance_Off(&mainboard.adbms_raw);

	Owc_c_channel_update(&mainboard.adbms_raw, &mainboard.adbms_6830);
	Owc_s_channel_update(&mainboard.adbms_raw, &mainboard.adbms_6830);
}

void update_values()
{
	//First Read both Voltages and Temps raw values in lower level interface
	ADBMS_WakeUP_ICs();

	//read voltages
	ADBMS_Read_Voltages(&mainboard.adbms_raw.read_raw_c, C_Channel_Read, mainboard.adbms_raw.SPI_data.hspi, mainboard.adbms_raw.SPI_data.spi_dataBuf);
	//Parse 6830 voltage
	ADBMS_6830_Parse_Voltage(&mainboard.adbms_raw.read_raw_c, &mainboard.adbms_6830.voltage);

	//read temp
	ADBMS_Read_Voltages(&mainboard.adbms_raw.read_raw_aux, AUX_Read, mainboard.adbms_raw.SPI_data.hspi, mainboard.adbms_raw.SPI_data.spi_dataBuf);
	//RESTART ADAX
	ADBMS_Write_CMD(mainboard.adbms_raw.SPI_data.hspi, mainboard.adbms_raw.command_bit.adax);
	//parse 6830 temp
	ADBMS_6830_Parse_Temperature(&mainboard.adbms_raw.read_raw_aux, &mainboard.adbms_6830.temperature);

    //parse 2950 data
    ADBMS_2950_Calculate_Values(&mainboard.adbms_raw, &mainboard.adbms_2950);

    //Update Faults
    //this is just 6830 faults -> 2950 faults still need to come
    Update_6830_InternalFault(&mainboard.adbms_6830);


	if (mainboard.internal_state == Charging)
	{
		cell_Balance_On(&mainboard.adbms_raw,&mainboard.adbms_6830);
	}

	// update STM32 Pin values
    mainboard.shutdown_present = HAL_GPIO_ReadPin(GPIOD, Shutdown_Contactors_Pin); 	   		  // shutdown status
    mainboard.imd_status = mainboard.imd_status && HAL_GPIO_ReadPin(GPIOD, IMD_STATUS_IN_Pin); // IMD_Status (software latched)
    mainboard.comms_6822_state = HAL_GPIO_ReadPin(GPIOD, AD6822_State_Pin);	 //Currently Unused  		   		  // 6822_State

	//soc
	Soc_Update(&mainboard);
	
    //this should change into both current_1 and 2 when both start working
	mainboard.adbms_2950.faults.overcurrent_fault = mainboard.adbms_2950.data.current > OVERCURRENT;

}

void check_faults()
{
	// raise fault flag if any fault is true
	// faults are latching
	mainboard.bms_fault = mainboard.bms_fault
							|| mainboard.adbms_6830.faults.overvoltage_fault
							|| mainboard.adbms_6830.faults.undervoltage_fault
							|| mainboard.adbms_6830.faults.overtemperature_fault
							|| mainboard.adbms_6830.faults.undertemperature_fault
							|| mainboard.adbms_6830.faults.openwire_voltage_fault
							|| mainboard.adbms_6830.faults.openwire_temp_fault
							|| mainboard.adbms_6830.faults.pec_fault;

	// write BMS_Status - healthy is high
	HAL_GPIO_WritePin(BMS_STATUS_OUT_GPIO_Port, BMS_STATUS_OUT_Pin, !mainboard.bms_fault);

	// set external faults
	// timeouts
	if (mainboard.internal_state == Charging) //in charger
	{
		// In charge states only check for charger timeout
		float charger_dt = HAL_GetTick() - mainboard.charger_last_msg_time;
		mainboard.charger_timeout = charger_dt > CHARGER_CAN_TIMEOUT;

		mainboard.timeout_fault = mainboard.charger_timeout;
		
	}
	else //in car
	{
		//In non-charge states check for inverter and vcu timeouts
		float vcu_dt = HAL_GetTick() - mainboard.vcu_last_msg_time;
		mainboard.vcu_timeout = vcu_dt > VCU_CAN_TIMEOUT;

		float inverter_dt = HAL_GetTick() - mainboard.inverter_last_msg_time;
		mainboard.inverter_timeout = inverter_dt > INVERTER_CAN_TIMEOUT;

		mainboard.timeout_fault = mainboard.vcu_timeout || mainboard.inverter_timeout;
	}

	mainboard.external_fault = !mainboard.shutdown_present || mainboard.timeout_fault;

	// Turns on external LED if external fault
	HAL_GPIO_WritePin(GPIOE, GPIO_LED_1_Pin, mainboard.external_fault);

}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) //TSSI Callback
{
	//Currently not checking which timer caused interrupt, should only be one
	//TSSI Logic
	if (!mainboard.imd_status || mainboard.bms_fault)
	{
		HAL_GPIO_WritePin(GPIOB, TSSI_G_Pin, GPIO_PIN_RESET);
		HAL_GPIO_TogglePin(GPIOB, TSSI_R_Pin);
	}
	else
	{
		HAL_GPIO_WritePin(GPIOB, TSSI_R_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, TSSI_G_Pin, GPIO_PIN_SET);
	}
}

void send_data_over_printf()
{
	// AD Prints for both chips
    ADBMS_6830_Print_Vals(&mainboard.adbms_6830);
	ADBMS_2950_Print_Vals(&mainboard.adbms_2950);


	// Mainboard Prints
	printf("Time: %d\n", (int)(HAL_GetTick() - mainboard.start_time));
	printf("BMS fault: %d\n", mainboard.bms_fault);
	printf("External fault: %d\n", mainboard.external_fault);
	printf("BMS State: %d\n", mainboard.internal_state);
	printf("Charging State: %d\n", mainboard.charging_state);
	printf("SOC: %f\n", mainboard.soc);
	
	// TODO Add more prints as needed
}

void send_data_over_USB()
{
	//work on this later because i have more important things to fix right now
}