#pragma once

#include "adbms_interface.h"
#include <float.h>


typedef struct
{
    //precontactor voltages
    int16_t vbat1_raw;
    int16_t vbat2_raw;

    //currents 
    int32_t i1_raw;
    int32_t i2_raw;

    //Shunt Thermistor Voltages
    int16_t v7;
    int16_t v9;

}adbms_2950_raw_data_;

typedef struct
{
    float precontactor_voltage;
    float current;
    float pack_temperature_1;
    float pack_temperature_2;

}adbms_2950_data_parsed_;

typedef struct
{
    bool overcurrent_warning;

    bool pec_warning;
    int pec_warning_count;

}adbms_2950_warnings_;

typedef struct
{
    adbms_2950_raw_data_         raw_data;
    adbms_2950_data_parsed_      data;
    adbms_2950_warnings_         warnings;

}adbms_2950_;


void ADBMS_2950_Calculate_Values(adbms_raw_* adbms_raw,adbms_2950_* adbms_2950);

//Calculate Values
void ADBMS_2950_Calculate_Vbat(adbms_raw_* adbms_raw,adbms_2950_* adbms_2950);
void ADBMS_2950_Calculate_Current(adbms_raw_* adbms_raw, adbms_2950_* adbms_2950);
void ADBMS_2950_Calculate_Shunt_Temp(adbms_raw_* adbms_raw, adbms_2950_* adbms_2950);

//Transfer Functions
float ADBMS_2950_Transfer_Vbat(int16_t vbat1_raw, int16_t vbat2_raw);
float ADBMS_2950_Transfer_Current(int32_t data);
float ADBMS_2950_Transfer_Shunt_Temp(int16_t voltage);

//Warning Updates
bool ADBMS_2950_Pec_Update(adbms_raw_* adbms_raw, adbms_2950_* adbms_2950);
void Update_2950_InternalFault(adbms_2950_* adbms_2950);

//Prints
void ADBMS_2950_Print_Vals(adbms_2950_* adbms_2950);