#include "bms_soc.h"

void Soc_Initialize(mainboard_ *mainboard)
{
    ADBMS_UpdateVoltages(&mainboard->adbms);
    float avg_v = mainboard->adbms.avg_v;
    int vidx1 = fmax(0, ((int)floor((avg_v - 2.5) * 1000.0 / 4.0)));
    int vidx2 = fmin(500, ((int)ceil((avg_v - 2.5) * 1000.0 / 4.0)));
    float v = (v2a_lookup[vidx2] - v2a_lookup[vidx1]) * ((avg_v - 2.5) * 1000.0 - floor((avg_v - 2.5) * 1000.0)) + v2a_lookup[vidx1];
    mainboard->soc = fmax(0, 4.5 - v / 1000.0) * 3.0 / (SYSTEM_CAPACITY);
    mainboard->prev_time = HAL_GetTick();
}

void Soc_Update(mainboard_ *mainboard)
{
    float tick = HAL_GetTick();
    float delta = (tick - mainboard->prev_time) / 1000.0f / 3600.0f; // convert to hours
    mainboard->prev_time = tick;

    mainboard->soc = mainboard->soc - delta * mainboard->adbms.current / (SYSTEM_CAPACITY);
    int curridx1 = fmin(fmax(0, (int)floor((SYSTEM_CAPACITY * 1000.0 / 3.0 - (mainboard->soc * SYSTEM_CAPACITY)) * 1000.0 / 3.0 / 6.0)), 4500);
    int curridx2 = fmin(fmax(0, (int)ceil((SYSTEM_CAPACITY * 1000.0 / 3.0 - (mainboard->soc * SYSTEM_CAPACITY)) * 1000.0 / 3.0 / 6.0)), 4500);
    mainboard->dcir = (a2r_lookup[curridx2] - a2r_lookup[curridx1]) * (((SYSTEM_CAPACITY * 1000.0 / 3.0 - (mainboard->soc * SYSTEM_CAPACITY)) * 1000.0 / 3.0) - floor((SYSTEM_CAPACITY - (mainboard->soc * SYSTEM_CAPACITY)) * 1000.0 / 3.0)) + a2r_lookup[curridx1];

    float delta_i = (mainboard->adbms.avg_v - UNDERVOLTAGE) / (mainboard->dcir * 1000.0f) * PARALLEL; // convert mili-ohms to ohms
    mainboard->max_discharge_current = fmin(OVERCURRENT, mainboard->max_discharge_current + delta_i);
}
