#include "bms.h"

mainboard_ mainboard;

void Bms_Mainboard_Setup(SPI_HandleTypeDef *hspi, FDCAN_HandleTypeDef *hcan)
{
	// initialize handles
	mainboard.hcan = hcan;

	// initialize ad chip;
	ADBMS_Initialize(&mainboard.adbms, hspi);

	// initialize CAN;
	Bms_Initialize_Can(&mainboard);


	// initialize the timers: adbms_mainboard_loop, drive_can, data_can
	timer_ t_adbms = Create_Timer(1000, bms_mainboard_loop);
	timer_ timers[NUM_TIMERS] = {t_adbms};
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
}

void update_values()
{
	// ADBMS values
	ADBMS_UpdateVoltages(&mainboard.adbms);

	if(ENABLE_USB_COMMS) send_data_over_USB(); 
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) //TSSI Callback
{
	return;
}

void send_data_over_USB()
{
	// USB Serial Print ADBMS values 
	// ADBMS_USB_Serial_Print_Vals(&mainboard.adbms);

	#define BUFFER_SIZE 64  // Increase this if more snprintfs are added
    char logBuf[BUFFER_SIZE];
    int len = 0;
    int remaining = BUFFER_SIZE;

    len += snprintf(logBuf + len, remaining, "Time: %d\r\n", (int)(HAL_GetTick() - mainboard.start_time));
    remaining = BUFFER_SIZE - len;

	len += snprintf(logBuf + len, remaining, "Current: %f\r\n", mainboard.adbms.current);
   remaining = BUFFER_SIZE - len;

// 	len += snprintf(logBuf + len, remaining, "BMS_fault: %d\r\n", mainboard.bms_fault);
//    remaining = BUFFER_SIZE - len;

// 	len += snprintf(logBuf + len, remaining, "External_fault: %d\r\n", mainboard.external_fault);
//    remaining = BUFFER_SIZE - len;

	if (remaining <= 0) HardFault_Handler();

	CDC_Transmit_FS((uint8_t*) logBuf, strlen(logBuf));
}
