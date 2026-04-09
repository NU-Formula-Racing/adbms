#include "adbms_interface_v2.h"
#include <float.h>

typedef struct
{
    raw_data_2950_  raw_data;
    data_2950_      data;

}adbms_2950_;

typedef struct
{
    //precontactor voltages
    int16_t vbat1_raw;
    int16_t vbat2_raw;

    //currents 
    int32_t i1_raw;
    int32_t i2_raw;

    //post contactor voltages
    int16_t v1_raw;
    int16_t v2_raw;

    //Shunt Thermistor Voltages
    int16_t v7;
    int16_t v9;

}raw_data_2950_;

typedef struct
{
    float precontactor_voltage;
    float current_1;
    float current_2;
    float postcontactor_voltage;
    float pack_temperature_1;
    float pack_temperature_2;

}data_2950_;

void ADBMS_2950_Calculate_Values(adbms_raw_* adbms_raw,adbms_2950_* adbms_2950);
void ADBMS_2950_Calculate_Vbat(adbms_raw_* adbms_raw,adbms_2950_* adbms_2950);
float ADBMS_2950_Transfer_Vbat(int16_t vbat1_raw, int16_t vbat2_raw);
void ADBMS_2950_Calculate_Current(adbms_raw_* adbms_raw, adbms_2950_* adbms_2950);
float ADBMS_2950_Transfer_Current(int32_t data);
void ADBMS_2950_Calculate_Post_Voltage(adbms_raw_* adbms_raw, adbms_2950_* adbms_2950);
float ADBMS_2950_Transfer_Post_Voltage(int16_t v1_raw, int16_t v2_raw);
void ADBMS_2950_Calculate_Shunt_Temp(adbms_raw_* adbms_raw, adbms_2950_* adbms_2950);
float ADBMS_2950_Transfer_Shunt_Temp(int16_t voltage);