#include "bms.h"

mainboard_ mainboard;

void Bms_Mainbaord_Setup(SPI_HandleTypeDef *hspi, FDCAN_HandleTypeDef *hcan)
{
	// initialize handles
	mainboard.hcan = hcan;

	// initialize ad chip;
	ADBMS_Initialize(&mainboard.adbms, hspi);

	// initialize CAN;
	Bms_Initialize_Can(&mainboard);

	// initialize SOC
	//Soc_Initialize(&mainboard);

	// initialize the timers: adbms_mainboard_loop, drive_can, data_can
	timer_ t_adbms = Create_Timer(250, bms_mainboard_loop);
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
	update_values();
	check_faults();
	Can_Loop();
	Control_Loop(&mainboard);
}

// Seprate loop that gets ticked to run OWC
void adbms_owc_loop()
{ 
	Update_Owc_Fault(&mainboard.adbms);
	Update_Owc_C_Channel_Fault(&mainboard.adbms);
}

void update_values()
{
	// ADBMS values
	ADBMS_UpdateVoltages(&mainboard.adbms);
	ADBMS_UpdateTemps(&mainboard.adbms);

	UpdateADInternalFault(&mainboard.adbms);

	// update STM32 Pin values
    mainboard.shutdown_present = HAL_GPIO_ReadPin(GPIOD, Shutdown_Contactors_Pin); 	   		  // shutdown status
    mainboard.imd_status = mainboard.imd_status && HAL_GPIO_ReadPin(GPIOD, IMD_STATUS_IN_Pin); // IMD_Status (software latched)
    mainboard.comms_6822_state = HAL_GPIO_ReadPin(GPIOD, AD6822_State_Pin);	 //Currently Unused  		   		  // 6822_State

	//soc
	//Soc_Update(&mainboard);
	
	
	mainboard.current = 0; //HARD CODE no pack board

	mainboard.overcurrent_fault = mainboard.current > OVERCURRENT;

	if(ENABLE_PRINTF_DEBUG_COMMS) send_data_over_printf(); 
	if(ENABLE_USB_COMMS) send_data_over_USB(); 
}

void check_faults()
{
	// raise fault flag if any fault is true
	// faults are latching
	mainboard.bms_fault = mainboard.bms_fault
							|| mainboard.adbms.overvoltage_fault_
							|| mainboard.adbms.undervoltage_fault_
							|| mainboard.adbms.overtemperature_fault_
							|| mainboard.adbms.undertemperature_fault_
							|| mainboard.adbms.openwire_fault_
							|| mainboard.adbms.openwire_temp_fault_
							|| mainboard.adbms.pec_fault_
							|| mainboard.overcurrent_fault;

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
	// AD Prints
	ADBMS_Print_Vals(&mainboard.adbms);

	// Mainboard Prints
	printf("Time: %d\n", (int)(HAL_GetTick() - mainboard.start_time));
	printf("BMS fault: %d\n", mainboard.bms_fault);
	printf("External fault: %d\n", mainboard.external_fault);
	printf("Current: %f\n", mainboard.current);
	
	// TODO Add more prints as needed

}

void send_data_over_USB()
{
	// USB Serial Print ADBMS values 
	ADBMS_USB_Serial_Print_Vals(&mainboard.adbms);

	#define BUFFER_SIZE 256  // Increase this if more snprintfs are added
    char logBuf[BUFFER_SIZE];
    int len = 0;
    int remaining = BUFFER_SIZE;

    len += snprintf(logBuf + len, remaining, "Time: %d\r\n", (int)(HAL_GetTick() - mainboard.start_time));
    remaining = BUFFER_SIZE - len;

	len += snprintf(logBuf + len, remaining, "Current: %f\r\n", mainboard.current);
   remaining = BUFFER_SIZE - len;

	len += snprintf(logBuf + len, remaining, "BMS_fault: %d\r\n", mainboard.bms_fault);
   remaining = BUFFER_SIZE - len;

	len += snprintf(logBuf + len, remaining, "External_fault: %d\r\n", mainboard.external_fault);
   remaining = BUFFER_SIZE - len;

	if (remaining <= 0) HardFault_Handler();

	CDC_Transmit_FS((uint8_t*) logBuf, strlen(logBuf));
}
