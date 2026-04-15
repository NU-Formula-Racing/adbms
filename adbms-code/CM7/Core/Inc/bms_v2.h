#pragma once

#ifndef ADBMS_MAINBOARD_H
#define ADBMS_MAINBOARD_H

//#include "bms_main_struct.h"

//these are all new version of inc files
#include "bms_can_v2.h" 
#include "bms_soc_v2.h"
#include "bms_control_v2.h"
#include <stdio.h>

void Bms_Mainboard_Setup(SPI_HandleTypeDef *hspi, FDCAN_HandleTypeDef *hcan);

void Tick_Mainboard_Timers();

void bms_mainboard_loop();
void adbms_owc_loop();

void update_values();
void check_faults();

void send_data_over_printf();
void send_data_over_USB();

#endif // ADBMS_MAINBOARD_H
