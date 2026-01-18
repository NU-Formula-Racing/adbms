#include "bms_main_struct.h"

void Control_Loop(mainboard_ *mainboard);

void in_car(mainboard_ *mainboard);
void charger_control(mainboard_ *mainboard);

bool check_fault_status(mainboard_ *mainboard);
