#pragma once

#include "adbms_interface.h"
#include <float.h>

//for the thermister function
#define SeriesResistance    10000.0
#define R25                 10000.0
#define B                   3435
#define T25                 298.15     



typedef struct 
{
    float voltages[NUM_CHIPS * NUM_VOLTAGES_ODD_CHIP + ((NUM_CHIPS + 1)/2)];

    float total_v;
    float max_v;
    float min_v;
    float avg_v;

    int pec_counts;

}adbms_6830_voltage_parsed_;

typedef struct
{
    float temperatures[NUM_CHIPS * NUM_TEMPS_CHIP];

    float max_temp;
    float min_temp;
    float avg_temp;

    int pec_counts;
    bool openwire_temp_fault;

}adbms_6830_temperature_parsed_;


typedef struct
{
    bool pec_fault;
    bool openwire_temp_fault;
    bool openwire_voltage_fault;

    bool overvoltage_fault;
    bool undervoltage_fault;

    bool overtemperature_fault;
    bool undertemperature_fault;


}adbms_6830_faults_;

typedef struct
{
    adbms_6830_voltage_parsed_         voltage;
    adbms_6830_temperature_parsed_     temperature;
    adbms_6830_faults_                 faults;

}adbms_6830_;


void ADBMS_6830_Calculate_Values(adbms_raw_* adbms_raw,adbms_6830_* adbms_6830);

//Parsers Voltage/Temperature
void ADBMS_6830_Parse_Voltage(adbms_read_raw_* raw_return, adbms_6830_voltage_parsed_* voltage_parsed);
void ADBMS_6830_Parse_Temperatrue(adbms_read_raw_* raw_return, adbms_6830_temperature_parsed_* temp_parsed);

//Upate Faults
void Update_6830_InternalFault(adbms_6830_* adbms_6830);

//Cell Balancing On Off
void cell_Balance_On(adbms_raw_* adbms_raw, adbms_6830_* adbms_6830);
void cell_Balance_Off(adbms_raw_* adbms);

//Update OWC Loops C/S Channel
void Owc_c_channel_update(adbms_raw_* adbms_raw, adbms_6830_* adbms_6830);
void Owc_s_channel_update(adbms_raw_* adbms_raw, adbms_6830_* adbms_6830);

//Transfer Functions
float ADBMS_6830_Transfer_Voltage(int data);
float ADBMS_6830_Transfer_Temp(float raw_temp_voltage, float Vref);

//Prints
void ADBMS_6830_Print_Vals(adbms_6830_* adbms_6830);