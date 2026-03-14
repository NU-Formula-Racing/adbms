#pragma once

#ifndef ADBMS_MAINBOARD_H
#define ADBMS_MAINBOARD_H

//#include "bms_main_struct.h"
#include "bms_can.h" 
#include "bms_soc.h"
#include "bms_control.h"
#include <stdio.h>

void Bms_Mainbaord_Setup(SPI_HandleTypeDef *hspi, FDCAN_HandleTypeDef *hcan);

void Tick_Mainboard_Timers();

void bms_mainboard_loop();
void adbms_owc_loop();

void do_can();
void volt_temp();
void update_values();
void check_faults();

void TSSI_Callback(TIM_HandleTypeDef *htim);

void send_data_over_printf();
void send_data_over_USB();

#endif // ADBMS_MAINBOARD_H
