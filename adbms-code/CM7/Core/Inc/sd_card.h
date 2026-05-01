#include "fatfs.h"
#include <string.h>
#include <stdio.h>
#include "bms_system_prams.h"

extern FATFS fs;
extern FIL fil;



void sd_init_bin();

void sd_init_csv();

void print_sd_info(SD_HandleTypeDef hsd);

void log_bms_data_bin(uint32_t tick, uint8_t* voltages, int num_voltages, uint8_t* temps, int num_temps);

void log_bms_data_csv(uint32_t tick, float* voltages, int num_voltages, float* temps, int num_temps);

