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
    float i1;
    float i2;

    //post contactor voltages
    float v_TS;

    //Shunt Thermistor Voltages
    float v_shunt_1;
    float v_shunt_2;

}raw_data_2950_;

typedef struct
{
    float precontactor_voltage;
    float current;
    float postcontactor_voltage;
    float pack_temperature;

}data_2950_;