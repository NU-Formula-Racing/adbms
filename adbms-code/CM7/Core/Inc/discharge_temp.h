#ifndef DISCHARGE_TEMP_H
#define DISCHARGE_TEMP_H

#include <stdbool.h>
#include "main.h"

/*
 * Discharge resistor temperature (DISCHARGE_TEMP net -> PA3 / ADC1_INP15).
 *
 * Signal chain on the NFR27 mainboard (Indicators sheet):
 *   MCP9700 (U25, TS side) -> 1k/10nF -> AMC3311 isolated amp (U21, gain 1)
 *   -> OPA320 difference amp (U20, 4x 3.3k, gain 1) -> 1k/100nF -> PA3
 * so the PA3 voltage is the MCP9700 output: 500 mV at 0 C, 10 mV/C.
 *
 * Register-level ADC1 driver: this CubeMX project does not include the HAL ADC driver.
 */

#define DISCHARGE_TEMP_VREF      3.3f    /* ADC reference (VREF+ = 3V3) */
#define DISCHARGE_TEMP_V_AT_0C   0.500f  /* MCP9700 output at 0 C */
#define DISCHARGE_TEMP_V_PER_C   0.010f  /* MCP9700 slope */

/* Configure PA3 + ADC1 (calibrate, enable). Call once at startup. */
void Discharge_Temp_Init(void);

/* One blocking conversion (~50 us). Returns false if the ADC is not ready or timed out. */
bool Discharge_Temp_Read(float *volts, float *deg_c);

#endif // DISCHARGE_TEMP_H
