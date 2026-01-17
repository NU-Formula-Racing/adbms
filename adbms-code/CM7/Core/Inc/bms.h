#ifndef ADBMS_MAINBOARD_H
#define ADBMS_MAINBOARD_H

//#include "bms_main_struct.h"
#include "bms_can.h" 
//#include "control.h"

void bms_mainbaord_setup(SPI_HandleTypeDef *hspi, FDCAN_HandleTypeDef *hcan);
void bms_mainboard_loop();

void print_test();
void tick_mainboard_timers();

void UpdateValues();
void CheckFaults();

void adbms_owc_loop();

void send_data_over_printf();
void send_data_over_USB();

#endif // ADBMS_MAINBOARD_H
