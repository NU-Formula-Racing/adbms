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
    float voltages[NUM_CHIPS * NUM_VOLTAGES_ODD_CHIP + ((NUM_CHIPS + 1)/2)]; //0 indexed, even chips (including 0) have 12 voltages, odds have 11
    float temperatures[NUM_CHIPS * NUM_TEMPS_CHIP];

    float total_v;
    float max_v;
    float min_v;
    float avg_v;

    float max_temp;
    float min_temp;
    float avg_temp;

}data_6830_;

typedef struct 
{
    int pec_failure_count;


}adbms_6830_failures_;

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
    data_6830_ data;
    adbms_6830_failures_    failures;
    adbms_6830_faults_  faults;

}adbms_6830_;


//Calculate Values
void ADBMS_6830_Calculate_Values(adbms_raw_* adbms_raw,adbms_6830_* adbms_6830);
void ADBMS_6830_Calculate_Voltage(adbms_raw_* adbms_raw,adbms_6830_* adbms_6830);
void ADBMS_6830_Calculate_Temperature(adbms_raw_* adbms_raw,adbms_6830_* adbms_6830);

//Upate Faults
void Update_6830_Owc_Fault(adbms_raw_* adbms_raw, adbms_6830_* adbms_6830);
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