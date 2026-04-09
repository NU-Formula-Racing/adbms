#include "adbms_interface_v2.h"
#include <float.h>

//for the thermister function
#define SeriesResistance    10000.0
#define R25                 10000.0
#define B                   3435
#define T25                 298.15     


typedef struct
{
    data_6830_ data;
    adbms_6830_failures_    failures;
    adbms_6830_faults_  faults;

}adbms_6830_;


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
