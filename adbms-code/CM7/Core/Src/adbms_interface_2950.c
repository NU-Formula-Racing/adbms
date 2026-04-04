#include "adbms_interface_2950.h"


void ADBMS_2950_Calculate_Values(adbms_* adbms,adbms_2950_* adbms_2950)
{
    ADBMS_2950_Calculate_Vbat(adbms, adbms_2950);
    ADBMS_2950_Calculate_Current(adbms, adbms_2950);
    ADBMS_2950_Calculate_Post_Voltage(adbms, adbms_2950);
    ADBMS_2950_Calculat_Shunt_Temp(adbms, adbms_2950);
}

void ADBMS_2950_Calculate_Vbat(adbms_* adbms,adbms_2950_* adbms_2950)
{
    //initialize values
    adbms_2950->raw_data.vbat1_raw = 0.0;
    adbms_2950->raw_data.vbat2_raw = 0.0;

    //offset because RDCVB is the second command sent
    int command_offset = NUM_CHIPS * DATA_LEN;
    //offset because 2950 is the last chip on the daisy chain
    int offset = (NUM_CHIPS-1) * DATA_LEN;

    //take raw bit values from adbms and place into raw_data in adbms_2950
    adbms_2950->raw_data.vbat1_raw = ((int16_t)(adbms->raw_value.cell[3 + command_offset + offset]) << 8) | (int16_t)(adbms->raw_value.cell[2 + command_offset + offset]);
    adbms_2950->raw_data.vbat2_raw = ((int16_t)(adbms->raw_value.cell[5 + command_offset + offset]) << 8) | (int16_t)(adbms->raw_value.cell[4 + command_offset + offset]);

    adbms_2950->data.precontactor_voltage = ADBMS_2950_Transfer_Vbat(adbms_2950->raw_data.vbat1_raw,adbms_2950->raw_data.vbat2_raw);

}

float ADBMS_2950_Transfer_Vbat(int16_t vbat1_raw, int16_t vbat2_raw){
    float vbat_final = 0.0;

    //transfer to real value and store
    float vbat1 = (float)(vbat1_raw / 0.0041938); // (15000/3,600,000 + 15000)
    float vbat2 = (float) vbat2_raw;

    vbat_final = (vbat1-vbat2) * 0.0001; //100 microolms
    
    return vbat_final;
}

void ADBMS_2950_Calculate_Current(adbms_* adbms, adbms_2950_* adbms_2950)
{
    
}