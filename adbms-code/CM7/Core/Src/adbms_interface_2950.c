#include "adbms_interface_2950.h"


void ADBMS_2950_Calculate_Values(adbms_raw_* adbms_raw,adbms_2950_* adbms_2950)
{
    ADBMS_2950_Calculate_Vbat(adbms_raw, adbms_2950);
    ADBMS_2950_Calculate_Current(adbms_raw, adbms_2950);
    ADBMS_2950_Calculate_Post_Voltage(adbms_raw, adbms_2950);
    ADBMS_2950_Calculate_Shunt_Temp(adbms_raw, adbms_2950);
}

void ADBMS_2950_Calculate_Vbat(adbms_raw_* adbms_raw,adbms_2950_* adbms_2950)
{
    //initialize values
    adbms_2950->raw_data.vbat1_raw = 0;
    adbms_2950->raw_data.vbat2_raw = 0;

    //offset because RDCVB is the second command sent
    int command_offset = NUM_CHIPS * DATA_LEN;
    //offset because 2950 is the last chip on the daisy chain
    int offset = (NUM_6830) * DATA_LEN;

    //take raw bit values from adbms and place into raw_data in adbms_2950
    adbms_2950->raw_data.vbat1_raw = ((int16_t)(adbms_raw->raw_voltages.c_channel_raw[3 + command_offset + offset]) << 8) | (int16_t)(adbms_raw->raw_voltages.c_channel_raw[2 + command_offset + offset]);
    adbms_2950->raw_data.vbat2_raw = ((int16_t)(adbms_raw->raw_voltages.c_channel_raw[5 + command_offset + offset]) << 8) | (int16_t)(adbms_raw->raw_voltages.c_channel_raw[4 + command_offset + offset]);

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

void ADBMS_2950_Calculate_Current(adbms_raw_* adbms_raw, adbms_2950_* adbms_2950)
{
    //initialize values
    adbms_2950->raw_data.i1_raw = 0;
    adbms_2950->raw_data.i2_raw = 0;

    //offset because 2950 is the last chip on the daisy chain
    int offset =  (NUM_6830) * DATA_LEN;

    //getting raw data from cell readings and putting it in adbms_2950 
    adbms_2950->raw_data.i1_raw = ((int32_t)(adbms_raw->raw_voltages.c_channel_raw[2 + offset]) << 16) | ((int32_t)(adbms_raw->raw_voltages.c_channel_raw[1 + offset]) << 8) | adbms_raw->raw_voltages.c_channel_raw[0 + offset];
    adbms_2950->raw_data.i2_raw = ((int32_t)(adbms_raw->raw_voltages.c_channel_raw[5 + offset]) << 16) | ((int32_t)(adbms_raw->raw_voltages.c_channel_raw[4 + offset]) << 8) | adbms_raw->raw_voltages.c_channel_raw[3 + offset];

    //sign extend because it is a 24 bit number but stored int32
    if (adbms_2950->raw_data.i1_raw & 0x00800000) {     
        adbms_2950->raw_data.i1_raw |= 0xFF000000;      
    }

    if(adbms_2950->raw_data.i2_raw & 0x00800000){
        adbms_2950->raw_data.i2_raw |= 0xFF000000;
    }

    //average the two
    adbms_2950->data.current = (-ADBMS_2950_Transfer_Current(adbms_2950->raw_data.i1_raw) + ADBMS_2950_Transfer_Current(adbms_2950->raw_data.i2_raw)) / 2 ;
}

float ADBMS_2950_Transfer_Current(int32_t data)
{
  float current;
  //the actual measured resistance is closer to 94 microolms instead of 100
  current = (float) data / 94.0;
  return current;
}


void ADBMS_2950_Calculate_Shunt_Temp(adbms_raw_* adbms_raw, adbms_2950_* adbms_2950)
{

    //initilaize values
    adbms_2950->raw_data.v7 = 0;
    adbms_2950->raw_data.v9 = 0;

    //offset because 2950 is the last chip ont he daisy chain
    int offset = (NUM_6830) * DATA_LEN;

    //getting raw data 
    adbms_2950->raw_data.v7 = (int16_t)(adbms_raw->raw_value.shunt_temp[1+offset] << 8) | (int16_t)(adbms_raw->raw_value.shunt_temp[0+offset]);
    adbms_2950->raw_data.v9 = (int16_t)(adbms_raw->raw_value.shunt_temp[5+offset] << 8) | (int16_t)(adbms_raw->raw_value.shunt_temp[4+offset]);

    adbms_2950->data.pack_temperature_1 = ADBMS_2950_Transfer_Shunt_Temp(adbms_2950->raw_data.v7);
    adbms_2950->data.pack_temperature_2 = ADBMS_2950_Transfer_Shunt_Temp(adbms_2950->raw_data.v9);

}

float ADBMS_2950_Transfer_Shunt_Temp(int16_t voltage){

//write this later :)

}

void ADBMS_2950_Print_Vals(adbms_2950_* adbms_2950)
{
    // 2950 prints
    printf("ADBMS 2950 Data\n\n");
    printf("adbms2950 Precontactor voltage: %f\n", adbms_2950->data.precontactor_voltage);
    printf("adbms2950 Current: %f\n\n", adbms_2950->data.current);
}




//
// Post Contactor Voltage No longe exists
//

//void ADBMS_2950_Calculate_Post_Voltage(adbms_raw_* adbms_raw, adbms_2950_* adbms_2950)
// {

//     //initialize values
//     adbms_2950->raw_data.v1_raw = 0;
//     adbms_2950->raw_data.v2_raw = 0;

//     //RDCVD is the 4th command that we send that get puts into cell
//     int command_offset = (DATA_LEN * NUM_CHIPS) * 3;
//     //offset because 2950 is the lsat chip on the daisy chain
//     int offset = (NUM_6830) * DATA_LEN;

//     //getting raw data from cell readings
//     adbms_2950->raw_data.v1_raw = (int16_t)(adbms_raw->raw_voltages.c_channel_raw[3+offset+command_offset] << 8) | (int16_t)(adbms_raw->raw_voltages.c_channel_raw[2+offset+command_offset]);
//     adbms_2950->raw_data.v2_raw = (int16_t)(adbms_raw->raw_voltages.c_channel_raw[5+offset+command_offset] << 8) | (int16_t)(adbms_raw->raw_voltages.c_channel_raw[1+offset+command_offset]);

//     adbms_2950->data.postcontactor_voltage = ADBMS_2950_Transfer_Post_Voltage(adbms_2950->raw_data.v1_raw, adbms_2950->raw_data.v2_raw);
// }

// float ADBMS_2950_Transfer_Post_Voltage(int16_t v1_raw, int16_t v2_raw){

//     float v_TS = 0.0;

//     float v1 = (float) (v1_raw / 0.0041938); // (15000/3,600,000 + 15000)
//     float v2 = (float) (v2_raw);

//     v_TS = (v1-v2) * 0.0001; //100 microolms
    
//     return v_TS;
// }
