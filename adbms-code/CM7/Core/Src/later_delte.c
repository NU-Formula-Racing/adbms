#include "adbms_interface_6830.h"
#include <math.h>


void ADBMS_6830_Calculate_Voltage(adbms_raw_* adbms_raw,adbms_6830_* adbms_6830)
{
    // reset current pec failures if there is no current failure
    if(!adbms_raw->read_failure.read_voltage_pec_failure && !adbms_raw->read_failure.read_temp_pec_failure) { 
        adbms_6830->failures.pec_failure_count = 0;
    }

    // if there is a pec failure, process it and don't update values
    if(adbms_raw->read_failure.read_voltage_pec_failure) {
        
        //this is the original version, we're adding a bool, this doesn't look right
        //adbms_6830->failures.current_pec_failures += adbms_raw->read_filure.read_voltage_pec_failure;

        //new version, we just increment by 1
        adbms_6830->failures.pec_failure_count += 1;

        if(adbms_6830->failures.pec_failure_count > PEC_FAILURE_THRESHOLD) {
            adbms_6830->faults.pec_fault = 1;
        }else {
            adbms_6830->faults.pec_fault = 0;
        }
        return;
    }

    // calculate the total, max, and min voltage
    adbms_6830->data.total_v = 0;
    adbms_6830->data.max_v = 0;
    adbms_6830->data.min_v = FLT_MAX;
    float even_total = 0;
    float odd_total = 0;
    for (uint8_t cic = 0; cic < (NUM_6830); cic++)
    {
        if (cic % 2 == 0) //even chip, 12 voltages
        {
            uint8_t num_reg_grps = NUM_VOLTAGES_EVEN_CHIP / VOLTAGES_REG_GRP + (NUM_VOLTAGES_EVEN_CHIP % VOLTAGES_REG_GRP != 0);//find how many register groups used to store voltages
            for (uint8_t creg_grp = 0; creg_grp < num_reg_grps; creg_grp++) //index register groups, each contain 3 registers that represent a voltage
            {
                for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)//each reg_grp is 2 bytes (each reg is 1 byte) INDEX REGISTER THEN BYTES?
                {
                    if(creg_grp*DATA_LEN/2 + cbyte/2 >= NUM_VOLTAGES_EVEN_CHIP) break;  //stop processing registers when desired voltage reading count is reached
                    int16_t raw_val = (((uint16_t)adbms_raw->raw_value.cell[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms_raw->raw_value.cell[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte];
                    float curr_voltage = ADBMS_6830_Transfer_Voltage(raw_val);
                    
                    adbms_6830->data.voltages[(cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2] = curr_voltage; //index section of current chip + index section of register group (group of 3 voltages) + index section the register (contains single voltage)

                    even_total += curr_voltage;
                    if (curr_voltage > adbms_6830->data.max_v){
                        adbms_6830->data.max_v = curr_voltage;
                    }
                    if (curr_voltage < adbms_6830->data.min_v){
                        adbms_6830->data.min_v = curr_voltage;
                    }
                }
            }
        }
        else //odd chip, 11 voltages
        {
            uint8_t num_reg_grps = NUM_VOLTAGES_ODD_CHIP / VOLTAGES_REG_GRP + (NUM_VOLTAGES_ODD_CHIP % VOLTAGES_REG_GRP != 0);//find how many register groups used to store voltages
            for (uint8_t creg_grp = 0; creg_grp < num_reg_grps; creg_grp++) //index register groups, each contain 3 registers that represent a voltage
            {
                for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)//each reg_grp is 2 bytes (each reg is 1 byte) INDEX REGISTER THEN BYTES?
                {
                    if(creg_grp*DATA_LEN/2 + cbyte/2 >= NUM_VOLTAGES_ODD_CHIP) break;  //stop processing registers when desired voltage reading count is reached

                    int16_t raw_val = (((uint16_t)adbms_raw->raw_value.cell[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms_raw->raw_value.cell[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte];
                    float curr_voltage = ADBMS_6830_Transfer_Voltage(raw_val);
                    
                    adbms_6830->data.voltages[(cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2] = curr_voltage; //index section of current chip + index section of register group (group of 3 voltages) + index section the register (contains single voltage)

                    odd_total += curr_voltage;
                    if (curr_voltage > adbms_6830->data.max_v){
                        adbms_6830->data.max_v = curr_voltage;
                    }
                    if (curr_voltage < adbms_6830->data.min_v){
                        adbms_6830->data.min_v = curr_voltage;
                    }
                }
            }
        }
    }
    
    adbms_6830->data.total_v = even_total + odd_total;
    // calculate the avg voltage
    if(NUM_6830 > 0){
        adbms_6830->data.avg_v = adbms_6830->data.total_v / ((NUM_6830) * NUM_VOLTAGES_ODD_CHIP + (((NUM_6830+1))/2));
    }
    else{
        adbms_6830->data.avg_v = 0.0;
    }

}