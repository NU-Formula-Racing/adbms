#include "bms.h"

mainboard_ mainboard;

// FSAE lamp check: RML + TSSI red + TSSI green on for LAMP_CHECK_MS after startup, then normal operation
static volatile bool lamp_check_active = false;
static uint32_t lamp_check_start = 0;

void Bms_Mainboard_Setup(SPI_HandleTypeDef *hspi, FDCAN_HandleTypeDef *hcan)
{
	// lamp check: all indicator lamps on (the TSSI timer callback ends it after LAMP_CHECK_MS)
	lamp_check_start = HAL_GetTick();
	lamp_check_active = true;
	HAL_GPIO_WritePin(GPIOB, TSSI_G_Pin|TSSI_R_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(RML_GPIO_GPIO_Port, RML_GPIO_Pin, GPIO_PIN_SET);

	// initialize handles
	mainboard.hcan = hcan;

	// cycle counter for loop timing stats (also used by the isoSPI fast wake)
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->LAR = 0xC5ACCE55U;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

	// discharge resistor temperature ADC (PA3)
	Discharge_Temp_Init();

	// initialize ad chip;
	ADBMS_Initialize(&mainboard.adbms_raw, hspi);

	// initialize CAN;
	Bms_Initialize_Can(&mainboard);

	// initialize SOC
	Soc_Initialize(&mainboard);

	// initialize the timers: adbms_mainboard_loop, drive_can, data_can
	timer_ t_adbms = Create_Timer(PACK_POLL_MS - 1, bms_mainboard_loop); // fires after > PACK_POLL_MS-1 ms -> every PACK_POLL_MS
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

// ADBMS loop that gets ticked every PACK_POLL_MS (10 ms):
//   packboard (2950) every pass -> 100 Hz, cell voltages + temps (6830) every CELL_POLL_DIVIDER passes -> 10 Hz
// measured poll rates for the debug print
static uint32_t stat_pack_polls = 0, stat_cell_polls = 0, stat_max_pass_cycles = 0;

void bms_mainboard_loop()
{
	static uint8_t pass = 0;
	const bool cell_pass = (pass % CELL_POLL_DIVIDER) == 0;
	const uint32_t pass_start = DWT->CYCCNT;

	update_pack_values();
	if (cell_pass) update_cell_values();
	update_pin_values();

	// isoSPI ring: split / ring mode, backup-path probe, break isolation
	ADBMS_Ring_Manage(&mainboard.adbms_raw);

	check_faults();
	Control_Loop(&mainboard);

	Can_Send_Packboard();
	if ((pass % CAN_STATUS_DIVIDER) == 0) Can_Send_Status();
	if (cell_pass) Can_Send_Cells();

	if(ENABLE_PRINTF_DEBUG_COMMS) send_data_over_printf();

	pass = (uint8_t)((pass + 1U) % CELL_POLL_DIVIDER);

	stat_pack_polls++;
	if (cell_pass) stat_cell_polls++;
	const uint32_t pass_cycles = DWT->CYCCNT - pass_start;
	if (pass_cycles > stat_max_pass_cycles) stat_max_pass_cycles = pass_cycles;
}

// Seprate loop that gets ticked to run OWC
void adbms_owc_loop()
{ 
	ADBMS_WakeUP_ICs();

	cell_Balance_Off(&mainboard.adbms_raw);

	Owc_c_channel_update(&mainboard.adbms_raw, &mainboard.adbms_6830);
	Owc_s_channel_update(&mainboard.adbms_raw, &mainboard.adbms_6830);

	owc_can_loop();
}


// Packboard (2950): accumulated current + voltage (new result every 8 ms, ACCI = 001), SOC
void update_pack_values()
{
	ADBMS_Wake_Scope(ADBMS_2950_CHIPS);

	ADBMS_Read_Voltages(&mainboard.adbms_raw.read_raw_2950_filtered, Accumulate_Read_2950, mainboard.adbms_raw.SPI_data.hspi, mainboard.adbms_raw.SPI_data.spi_dataBuf);
    ADBMS_2950_Calculate_Values(&mainboard.adbms_raw.read_raw_2950_filtered, &mainboard.adbms_2950, mainboard.adbms_raw.command_parameters.parameter_2950.cfa2950.acci);
	Update_2950_InternalFault(&mainboard.adbms_2950);

	//soc
	Soc_Update(&mainboard);
}

// Daughterboards (6830s): cell voltages and thermistors, 6830 faults, discharge resistor temperature
void update_cell_values()
{
	ADBMS_Wake_Scope(ADBMS_6830_CHIPS);

	// turn cb off if charging to read true voltage (balancing now stays on between the 10 Hz reads)
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

    //Update Faults
    Update_6830_InternalFault(&mainboard.adbms_6830);

	// restart adcs if needed
    if (mainboard.adbms_6830.voltage.adc_off)
    {
		ADBMS_Initialize(&mainboard.adbms_raw, mainboard.adbms_raw.SPI_data.hspi);
    }

	// discharge resistor temperature (read only for now)
	mainboard.discharge_temp_valid = Discharge_Temp_Read(&mainboard.discharge_temp_v, &mainboard.discharge_temp);
}

// STM32 inputs, every pass
void update_pin_values()
{
    mainboard.shutdown_present = HAL_GPIO_ReadPin(Shutdown_Contactors_GPIO_Port, Shutdown_Contactors_Pin); 	   		  // shutdown status
    mainboard.imd_status = mainboard.imd_status && HAL_GPIO_ReadPin(GPIOD, IMD_STATUS_IN_Pin); // IMD_Status (software latched)
    mainboard.comms_6822_state = HAL_GPIO_ReadPin(GPIOD, AD6822_State_Pin);	 //Currently Unused  		   		  // 6822_State
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

#if !PD7_HOLD_LOW_ENABLED
	// write BMS_Status - healthy is high
	HAL_GPIO_WritePin(BMS_STATUS_OUT_GPIO_Port, BMS_STATUS_OUT_Pin, !mainboard.bms_fault);
#else
	// !!! TESTING OVERRIDE (PD7_HOLD_LOW_ENABLED in main.h): BMS_STATUS_OUT / BMS shutdown output (PD7)
	// !!! is held LOW permanently. The BMS will NOT close the shutdown circuit. Set to 0 for normal operation.
#endif

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
	// lamp check: hold RML + both TSSI lamps on, then hand RML back to the hardware (PB6 low)
	if (lamp_check_active)
	{
		if (HAL_GetTick() - lamp_check_start < LAMP_CHECK_MS) return;
		lamp_check_active = false;
		HAL_GPIO_WritePin(RML_GPIO_GPIO_Port, RML_GPIO_Pin, GPIO_PIN_RESET);
	}

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
	// Printing every 20 ms loop overflows the USB CDC log queue, so limit the rate.
	static uint32_t last_print = 0;
	if (HAL_GetTick() - last_print < DEBUG_PRINT_PERIOD_MS) return;
	last_print = HAL_GetTick();

	// AD Prints for both chips
	ADBMS_Ring_Print();
	{
		// measured over the last print window (DEBUG_PRINT_PERIOD_MS)
		static uint32_t last_ms = 0;
		const uint32_t now = HAL_GetTick();
		const float window_s = (now - last_ms) / 1000.0f;
		if (last_ms != 0 && window_s > 0.0f)
		{
			printf("Poll rates: packboard %.1f Hz, cells+temps %.1f Hz | longest pass %lu us\n",
			       stat_pack_polls / window_s, stat_cell_polls / window_s,
			       (unsigned long)(stat_max_pass_cycles / (SystemCoreClock / 1000000U)));
		}
		last_ms = now;
		stat_pack_polls = 0;
		stat_cell_polls = 0;
		stat_max_pass_cycles = 0;
	}
    ADBMS_6830_Print_Vals(&mainboard.adbms_6830);
	ADBMS_2950_Print_Vals(&mainboard.adbms_2950);


	// Mainboard Prints
	printf("Time: %d\n", (int)(HAL_GetTick() - mainboard.start_time));
	printf("BMS fault: %d\n", mainboard.bms_fault);
	printf("External fault: %d\n", mainboard.external_fault);
	printf("BMS State: %d\n", mainboard.internal_state);
	printf("Charging State: %d\n", mainboard.charging_state);
	printf("SOC: %f\n", mainboard.soc);
	printf("Discharge temp: %f V, %f C%s\n", mainboard.discharge_temp_v, mainboard.discharge_temp,
	       mainboard.discharge_temp_valid ? "" : " (ADC read failed)");
	printf("Clock: PLL1 from %s, SYSCLK %lu Hz\n",
	       ((RCC->PLLCKSELR & RCC_PLLCKSELR_PLLSRC) == RCC_PLLCKSELR_PLLSRC_HSE) ? "HSE 8 MHz crystal" : "HSI (internal, HSE failed to start)",
	       (unsigned long)HAL_RCC_GetSysClockFreq());
	
	// TODO Add more prints as needed
}