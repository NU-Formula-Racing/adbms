#pragma once

#include "adbms_driver.h"
#include "thermistor_driver.h"
#include "bms_system_prams.h"
#include <float.h>

// Container that holds all the adbms values. 
// Different from the ICs struct that the raw SPI data gets put into.
typedef struct
{
    adbms6830_ICs ICs;

    //all the data collected from 2950 will go into this struct
    //has attibutes for current and voltage
    data_2950 data_2950;

    cfa_ cfa[NUM_CHIPS]; //one less because of the 2950
    cfb_ cfb[NUM_CHIPS];
    adcv_ adcv;
    adsv_ adsv;
    adax_ adax;

    //2950 specific configurations and commands
    cfa2950_ cfa2950;
    adv_ adv;

    //2950 
    float current;
    float precontactor_voltage;
    float postcontactor_voltage;
    float pack_temperature;

    float voltages[NUM_CHIPS * NUM_VOLTAGES_ODD_CHIP + ((NUM_CHIPS + 1)/2)]; //0 indexed, even chips (including 0) have 12 voltages, odds have 11
    float temperatures[NUM_CHIPS * NUM_TEMPS_CHIP];

    float total_v;
    float max_v;
    float min_v;
    float avg_v;

    float max_temp;
    float min_temp;
    float avg_temp;

    bool undervoltage_fault_;
    bool overvoltage_fault_;
    bool undertemperature_fault_;
    bool overtemperature_fault_;
    bool openwire_fault_;
    bool openwire_temp_fault_;
    bool pec_fault_;

    bool voltage_pec_failure;
    bool temp_pec_failure;
    bool status_reg_pec_failure;
    float current_owc_failures;
    float current_pec_failures;
    float total_pec_failures;
} adbms_;


void ADBMS_Initialize(adbms_ *adbms, SPI_HandleTypeDef *hspi);
void ADBMS_2950_config(uint8_t* cfg_a, cfa2950_* cfa2950);

void UpdateADInternalFault(adbms_ *adbms);

void ADBMS_UpdateVoltages(adbms_ *adbms);

//2950 function headerfiles
void ADBMS_2950_Calculate_Values(adbms_* adbms);
void ADBMS2950_Calculate_Vbat(adbms_* adbms);
float ADBMS_2950_Transfer_Vbat(int16_t vbat1_raw, int16_t vbat2_raw);
void ADBMS2950_Calculate_Current(adbms_* adbms);
float ADBMS2950_Transfer_Current(int32_t data);

float ADBMS2950_Calculate_Post_Voltage(adbms_ *adbms);
float ADBMS_Calculate_Post_Voltage(int16_t v1_raw, int16_t v2_raw);

float ADBMS2950_Calculate_Shunt_Temp(adbms_ *adbms);
float ADBMS2950_Transfer_Shunt_Temp(int16_t voltage);


void ADBMS_UpdateTemps(adbms_ *adbms);
void Update_Owc_Fault(adbms_ *adbms);
void Update_Owc_C_Channel_Fault(adbms_ *adbms);

void ADBMS_CalculateValues_Voltages(adbms_ *adbms);
void ADBMS_CalculateValues_Temps(adbms_ *adbms);

bool ADBMS_PEC_Check(adbms_ *adbms);

void cellBalanceOn(adbms_ *adbms);
void cellBalanceOff(adbms_ *adbms);

void ADBMS_Print_Vals(adbms_ *adbms);
void ADBMS_USB_Serial_Print_Vals(adbms_ *adbms);
