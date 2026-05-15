#pragma once

#include "adbms_interface.h"
#include <float.h>


typedef struct
{
    //precontactor voltages
    int32_t vbat1_raw;
    int32_t vbat2_raw;

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
    adbms_2950_raw_data_         raw_data;
    adbms_2950_data_parsed_      data;
}adbms_2950_;


void ADBMS_2950_Calculate_Values(adbms_read_raw_* adbms_2950_read_raw,adbms_2950_* adbms_2950);

//Calculate Values
void ADBMS_2950_Calculate_Vbat(adbms_read_raw_* adbms_2950_read_raw,adbms_2950_* adbms_2950);
void ADBMS_2950_Calculate_Current(adbms_read_raw_* adbms_2950_read_raw, adbms_2950_* adbms_2950);
void ADBMS_2950_Calculate_Shunt_Temp(adbms_read_raw_* adbms_2950_read_raw, adbms_2950_* adbms_2950);

//Transfer Functions
float ADBMS_2950_Transfer_Vbat(int32_t vbat1_raw, int32_t vbat2_raw);
float ADBMS_2950_Transfer_Current(int32_t data);
float ADBMS_2950_Transfer_Shunt_Temp(int16_t voltage);

//Prints
void ADBMS_2950_Print_Vals(adbms_2950_* adbms_2950);
