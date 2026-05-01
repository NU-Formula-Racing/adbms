#pragma once

#ifndef ADBMS_MAINBOARD_H
#define ADBMS_MAINBOARD_H

#ifndef HSEM_ID_1
#define HSEM_ID_1 (1U)
#endif

//#include "bms_main_struct.h"
#include "bms_can.h" 
#include "bms_soc.h"
#include "bms_control.h"
#include "adbms_interface.h"
#include <stdio.h>
#include <stdint.h>

void Bms_Mainbaord_Setup(FDCAN_HandleTypeDef *hcan);

void bms_mainboard_loop();

void update_values();
void check_faults();

void TSSI_Function();

void send_data_over_printf();
void send_data_over_USB();

#endif // ADBMS_MAINBOARD_H
