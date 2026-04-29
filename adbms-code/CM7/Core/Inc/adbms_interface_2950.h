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

}raw_data_2950_;

typedef struct
{
    float precontactor_voltage;
    float current;
    float pack_temperature_1;
    float pack_temperature_2;

}data_2950_;

typedef struct
{
    bool overcurrent_fault;

}adbms_2950_faults_;

typedef struct
{
    raw_data_2950_  raw_data;
    data_2950_      data;
    adbms_2950_faults_   faults;

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

//Prints
void ADBMS_2950_Print_Vals(adbms_2950_* adbms_2950);