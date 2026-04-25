#pragma once

#include "bms_main_struct.h"
#include "adbms_interface.h"
#include "adbms_interface_2950.h"
#include "adbms_interface_6830.h"


void Control_Loop(mainboard_ *mainboard);

void in_car(mainboard_ *mainboard);
void charger_control(mainboard_ *mainboard);

bool check_fault_status(mainboard_ *mainboard);
