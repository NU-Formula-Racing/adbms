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
	timer_ t_adbms = Create_Timer(19, bms_mainboard_loop); // bc of timer group delay is +1ms of the duration
	timer_ t_adbms_owc_check = Create_Timer(OWC_PERIOD_MS, adbms_owc_loop);
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
	Control_Loop(&mainboard);
	Can_Loop();
	
	// Test: once, OC_PIN_TEST_DELAY_MS after boot, drive the selected OCA/OCB pin(s) high
	static bool oc_pin_test_done = false;
	if (OC_PIN_TEST && !oc_pin_test_done && (HAL_GetTick() - mainboard.start_time) >= OC_PIN_TEST_DELAY_MS)
	{
		ADBMS_2950_OC_Pin_Test(&mainboard.adbms_raw, OC_PIN_TEST_OCA, OC_PIN_TEST_OCB);
		oc_pin_test_done = true;
		if (ENABLE_PRINTF_DEBUG_COMMS) printf("\n*** OC pin test: OCA -> %s, OCB -> %s ***\n",
		                                      OC_PIN_TEST_OCA ? "HIGH" : "LOW", OC_PIN_TEST_OCB ? "HIGH" : "LOW");
	}

	if(ENABLE_PRINTF_DEBUG_COMMS) send_data_over_printf();

	if(mainboard.internal_state == Fault) HAL_Delay(10);	// delay loop to send values slower in fault
}

// Last open-wire check result, for the periodic printout
static uint32_t owc_last_time  = 0;
static int      owc_last_c_pec = 0;
static int      owc_last_s_pec = 0;
static bool     owc_has_run    = false;

// Seprate loop that gets ticked to run OWC
void adbms_owc_loop()
{
	if (!ENABLE_OWC || NUM_6830 == 0) return;
 
	ADBMS_WakeUP_ICs();

	cell_Balance_Off(&mainboard.adbms_raw);

	// Clear the per-channel results so they show this run only.
	// (openwire_voltage_fault and bms_fault stay latched as before.)
	adbms_6830_faults_ *f = &mainboard.adbms_6830.faults;
	memset(f->owc_c_faults, 0, sizeof(f->owc_c_faults));
	memset(f->owc_s_faults, 0, sizeof(f->owc_s_faults));

	Owc_c_channel_update(&mainboard.adbms_raw, &mainboard.adbms_6830);
	owc_last_c_pec = mainboard.adbms_6830.voltage_owc.pec_counts;
	Owc_s_channel_update(&mainboard.adbms_raw, &mainboard.adbms_6830);
	owc_last_s_pec = mainboard.adbms_6830.voltage_owc.pec_counts;
	owc_last_time  = HAL_GetTick();
	owc_has_run    = true;

	if (ENABLE_PRINTF_DEBUG_COMMS)
	{
		printf("\n=== Open wire check ===\n");
		print_owc_results();
	}

	owc_can_loop();
}

// Print the result of the most recent open-wire check (OW voltage < OWC_VOLTAGE_THRESHOLD = open).
// owc_c_faults / owc_s_faults are cleared at the start of each run, so they hold the last run only.
void print_owc_results()
{
	if (!ENABLE_OWC || NUM_6830 == 0) return;
	if (!owc_has_run)
	{
		printf("Last OWC: not run yet (first check at ~%d ms)\n", OWC_PERIOD_MS);
		return;
	}

	adbms_6830_faults_ *f = &mainboard.adbms_6830.faults;
	int n_c = 0, n_s = 0;

	printf("Last OWC (t=%lu ms, %lu ms ago)  C open:", (unsigned long)owc_last_time, (unsigned long)(HAL_GetTick() - owc_last_time));
	for (int i = 0; i < NUM_VOLTAGES; i++) if (f->owc_c_faults[i]) { printf(" C%d", i + 1); n_c++; }
	printf("%s%s   S open:", n_c ? "" : " none", owc_last_c_pec ? " (PEC FAIL - not updated)" : "");
	for (int i = 0; i < NUM_VOLTAGES; i++) if (f->owc_s_faults[i]) { printf(" S%d", i + 1); n_s++; }
	printf("%s%s\n", n_s ? "" : " none", owc_last_s_pec ? " (PEC FAIL - not updated)" : "");
}


void update_values()
{
	//First Read both Voltages and Temps raw values in lower level interface
	ADBMS_WakeUP_ICs();
	
	// turn cb off if charging to read true voltage
	if (mainboard.internal_state == Charging) 
	{
		cell_Balance_Off(&mainboard.adbms_raw);
		ADBMS_Write_CMD(mainboard.adbms_raw.SPI_data.hspi, mainboard.adbms_raw.command_bit.adcv); // clear adc filter
    	HAL_Delay(1); // ADCs are updated at their conversion rate of 1ms
	}

	
	//read voltages
	ADBMS_Read_Voltages(&mainboard.adbms_raw.read_raw_c, C_Channel_Filtered_Read, mainboard.adbms_raw.SPI_data.hspi, mainboard.adbms_raw.SPI_data.spi_dataBuf);
	//Parse 6830 voltage
	ADBMS_6830_Parse_Voltage(&mainboard.adbms_raw.read_raw_c, &mainboard.adbms_6830.voltage);

	// turn cb back on if charging
	if (mainboard.internal_state == Charging) cell_Balance_On(&mainboard.adbms_raw,&mainboard.adbms_6830);


	//read temp
	ADBMS_Read_Voltages(&mainboard.adbms_raw.read_raw_aux, AUX_Read, mainboard.adbms_raw.SPI_data.hspi, mainboard.adbms_raw.SPI_data.spi_dataBuf);
	//RESTART ADAX
	ADBMS_Write_CMD(mainboard.adbms_raw.SPI_data.hspi, mainboard.adbms_raw.command_bit.adax);
	//parse 6830 temp
	ADBMS_6830_Parse_Temperature(&mainboard.adbms_raw.read_raw_aux, &mainboard.adbms_6830.temperature);

	// read filtered voltages
	ADBMS_Read_Voltages(&mainboard.adbms_raw.read_raw_2950_filtered, Accumulate_Read_2950, mainboard.adbms_raw.SPI_data.hspi, mainboard.adbms_raw.SPI_data.spi_dataBuf);
    // parse 2950 data
    ADBMS_2950_Calculate_Values(&mainboard.adbms_raw.read_raw_2950_filtered, &mainboard.adbms_2950, mainboard.adbms_raw.command_parameters.parameter_2950.cfa2950.acci);

    //Update Faults
    Update_6830_InternalFault(&mainboard.adbms_6830);
	Update_2950_InternalFault(&mainboard.adbms_2950);


	// restart adcs if needed
    if (mainboard.adbms_6830.voltage.adc_off)
    {
		ADBMS_Initialize(&mainboard.adbms_raw, mainboard.adbms_raw.SPI_data.hspi);
    }

	// update STM32 Pin values
    mainboard.shutdown_present = HAL_GPIO_ReadPin(GPIOD, Shutdown_Contactors_Pin); 	   		  // shutdown status
    mainboard.imd_status = mainboard.imd_status && HAL_GPIO_ReadPin(GPIOD, IMD_STATUS_IN_Pin); // IMD_Status (software latched)
    mainboard.comms_6822_state = HAL_GPIO_ReadPin(GPIOD, AD6822_State_Pin);	 //Currently Unused  		   		  // 6822_State

	//soc
	Soc_Update(&mainboard);

	// delay to allow higher duty cycle on cb
	if (mainboard.internal_state == Charging) HAL_Delay(32);
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
//    ADBMS_6830_Print_Vals(&mainboard.adbms_6830);
//	ADBMS_2950_Print_Vals(&mainboard.adbms_2950);
//	ADBMS_2950_Print_I1_Diff(&mainboard.adbms_2950, mainboard.adbms_raw.command_parameters.parameter_2950.cfa2950.acci);
	if (NUM_6830 > 0)
	{
		ADBMS_6830_Print_Voltages(&mainboard.adbms_6830);
		print_owc_results();
	}
	if (NUM_2950 > 0) ADBMS_2950_Print_Raw_ADC(&mainboard.adbms_raw);


	// Mainboard Prints
//	printf("Time: %d\n", (int)(HAL_GetTick() - mainboard.start_time));
//	printf("BMS fault: %d\n", mainboard.bms_fault);
//	printf("External fault: %d\n", mainboard.external_fault);
//	printf("BMS State: %d\n", mainboard.internal_state);
//	printf("Charging State: %d\n", mainboard.charging_state);
//	printf("SOC: %f\n", mainboard.soc);
	
	// TODO Add more prints as needed
}
