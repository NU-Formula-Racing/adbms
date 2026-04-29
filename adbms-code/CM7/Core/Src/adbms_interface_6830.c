#include "adbms_interface_6830.h"
#include <math.h>

void ADBMS_6830_Calculate_Values(adbms_raw_* adbms_raw,adbms_6830_* adbms_6830)
{
    ADBMS_6830_Calculate_Voltage(adbms_raw,adbms_6830);
    ADBMS_6830_Calculate_Temperature(adbms_raw,adbms_6830);
}

void ADBMS_6830_Parse_Voltage(adbms_read_raw_* raw_return, adbms_6830_voltage_parsed_* voltage_parsed)
{
    
}

void ADBMS_6830_Parse_Temperatrue(adbms_read_raw_* raw_return, adbms_6830_temperature_parsed_* temp_parsed)
{

}

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


void ADBMS_6830_Calculate_Temperature(adbms_raw_* adbms_raw,adbms_6830_* adbms_6830)
{
    // reset current pec failures if there is no current failure
    if(!adbms_raw->read_failure.read_voltage_pec_failure && !adbms_raw->read_failure.read_temp_pec_failure) { 
        adbms_6830->failures.pec_failure_count = 0;
    }

    // if there is a pec failure, process it and don't update values
    if(adbms_raw->read_failure.read_temp_pec_failure) {
        adbms_6830->failures.pec_failure_count += 1;
        if(adbms_6830->failures.pec_failure_count > PEC_FAILURE_THRESHOLD) {
            adbms_6830->faults.pec_fault = 1;
        }else {
            adbms_6830->faults.pec_fault = 0;
        }
        return;
    }

    // calculate the total, max, and min temp
    float total_temp = 0.0;
    adbms_6830->data.max_temp = 0;
    adbms_6830->data.min_temp = FLT_MAX;
    adbms_6830->faults.openwire_temp_fault = false;

    for (int cic = 0; cic < (NUM_6830); cic++)
    {
        for (uint8_t creg_grp = 0; creg_grp < AUX_REG_GRP; creg_grp++)
        {
            for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)
            {
            	if(creg_grp*DATA_LEN/2 + cbyte/2 >= AUX_GPIO) continue;	// only 10 gpio's

                int16_t raw_val = (((uint16_t)adbms_raw->raw_value.aux[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms_raw->raw_value.aux[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte];
                float raw_temp_voltage = ADBMS_6830_Transfer_Voltage(raw_val);

                // get ref voltage from status reg - not getting status regs because takes too long
                //float vref = ADBMS_getVoltage(ICs[i].stata.vref2);
                float vref = 3; // 3V defined in the datasheet
                if (vref - raw_temp_voltage < 1e-1)
                    adbms_6830->faults.openwire_temp_fault = true;

                float curr_temp = ADBMS_6830_Transfer_Temp(raw_temp_voltage, vref);
                adbms_6830->data.temperatures[cic*NUM_TEMPS_CHIP + creg_grp*DATA_LEN/2 + cbyte/2] = curr_temp;
                total_temp += curr_temp;
                if (curr_temp > adbms_6830->data.max_temp)
                    adbms_6830->data.max_temp = curr_temp;
                if (curr_temp < adbms_6830->data.min_temp)
                    adbms_6830->data.min_temp = curr_temp;
            }
        }
    }

    // calculate the avg temp
    if (NUM_CHIPS > 1){
        adbms_6830->data.avg_temp = total_temp / ((NUM_6830) * NUM_TEMPS_CHIP);
    }
    else{
        adbms_6830->data.avg_temp = 0.0;
    }

}


void ADBMS_6830_Calculate_Voltage(voltages_raw_* raw_voltages, voltages_ voltages, )


void Update_6830_Owc_Fault(adbms_raw_* adbms_raw, adbms_6830_* adbms_6830)
{
    if (adbms_raw->read_failure.read_open_wire_pec_failure){
        adbms_6830->failures.pec_failure_count += 1;
        if (adbms_6830->failures.pec_failure_count > PEC_FAILURE_THRESHOLD){
            adbms_6830->faults.pec_fault = 1;
        }
    }
    else{
        adbms_6830->failures.pec_failure_count = 0;
    }

    for (uint8_t cic = 0; cic < (NUM_6830); cic++)
    {
        if( cic % 2 == 0) //even chip
        {
            uint8_t num_reg_grps = NUM_VOLTAGES_EVEN_CHIP / VOLTAGES_REG_GRP + (NUM_VOLTAGES_EVEN_CHIP % VOLTAGES_REG_GRP != 0);//find how many register groups used to store voltages
            for (uint8_t creg_grp = 0; creg_grp < num_reg_grps; creg_grp++)
            {
                for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)
                {
                    if(creg_grp*DATA_LEN/2 + cbyte/2 >= NUM_VOLTAGES_EVEN_CHIP) break;
                    int16_t raw_val = (((uint16_t)adbms_raw->raw_value.scell[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms_raw->raw_value.scell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    if (ADBMS_6830_Transfer_Voltage(raw_val) < 0.5)
                    {
                        int c_cell = (cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2;
                        adbms_6830->faults.openwire_voltage_fault = 1;
                        return;
                    }
                }
            }
        }
        else //odd chip
        {
            uint8_t num_reg_grps = NUM_VOLTAGES_ODD_CHIP / VOLTAGES_REG_GRP + (NUM_VOLTAGES_ODD_CHIP % VOLTAGES_REG_GRP != 0);//find how many register groups used to store voltages
            for (uint8_t creg_grp = 0; creg_grp < num_reg_grps; creg_grp++)
            {
                for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)
                {
                    if(creg_grp*DATA_LEN/2 + cbyte/2 >= NUM_VOLTAGES_ODD_CHIP) break;
                    int16_t raw_val = (((uint16_t)adbms_raw->raw_value.scell[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms_raw->raw_value.scell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    if (ADBMS_6830_Transfer_Voltage(raw_val) < 0.5)
                    {
                        int c_cell = (cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2;
                        adbms_6830->faults.openwire_voltage_fault = 1;
                        return;
                    }
                }
            }
        }
    }
}



void Update_6830_InternalFault(adbms_6830_* adbms_6830)
{
    //all faults are latching

    // check overvoltage fault
    adbms_6830->faults.overvoltage_fault = adbms_6830->faults.overvoltage_fault || (adbms_6830->data.max_v > OVERVOLTAGE);
    //adbms_6830->faults.overvoltage_fault = (adbms_6830->data.max_v > OVERVOLTAGE); // non-latching faults


    // check undervoltage fault
    adbms_6830->faults.undervoltage_fault = adbms_6830->faults.undervoltage_fault || (adbms_6830->data.min_v < UNDERVOLTAGE);
    //adbms_6830->faults.undervoltage_fault = (adbms_6830->data.min_v < UNDERVOLTAGE); // non-latching faults

    // check overtemperature fault
    adbms_6830->faults.overtemperature_fault = adbms_6830->faults.overtemperature_fault || (adbms_6830->data.max_temp > OVERTEMP);
    //adbms_6830->faults.overtemperature_fault = (adbms_6830->data.max_temp > OVERTEMP); // non-latching faults

    // check undertemperature fault
    adbms_6830->faults.undertemperature_fault = adbms_6830->faults.undertemperature_fault || (adbms_6830->data.min_temp < UNDERTEMP);
    //adbms_6830->faults.undertemperature_fault = (adbms_6830->data.min_temp < UNDERTEMP); // non-latching faults

}



void cell_Balance_On(adbms_raw_* adbms_raw, adbms_6830_* adbms_6830)
{
    // Turn on CB indication LED
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_SET);
 
    for (int cic = 0; cic < (NUM_6830); cic++)
    {
        if (cic % 2 == 0) //even chip, 12 voltages
        {
            uint16_t dcc = 0;
            for (int cvoltage = 0; cvoltage < NUM_VOLTAGES_EVEN_CHIP; cvoltage++)
            {
                float curr_v = adbms_6830->data.voltages[(cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + cvoltage];
                if ((curr_v - adbms_6830->data.min_v) > CB_THRESHOLD && curr_v > CB_MIN_V_THRESHOLD)
                {
                    dcc |= 1 << cvoltage;
                }
            }
            // adbms->cfb[cic].dcc = dcc;
            adbms_raw->command_parameters.parameter_6830.cfb6830[cic].dcc = 1 << 0;
        }
        else //odd chip, 11 voltages
        {
            uint16_t dcc = 0;
            for (int cvoltage = 0; cvoltage < NUM_VOLTAGES_ODD_CHIP; cvoltage++)
            {
                float curr_v = adbms_6830->data.voltages[(cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + cvoltage];
                if ((curr_v - adbms_6830->data.min_v) > CB_THRESHOLD && curr_v > CB_MIN_V_THRESHOLD)
                {
                    dcc |= 1 << cvoltage;
                }
            }
            // adbms->cfb[cic].dcc = dcc;
            adbms_raw->command_parameters.parameter_6830.cfb6830[cic].dcc = 1 << 1;
        }
    }

    //i should probably clean this up later
    command_parameters_6830_ parameters = adbms_raw->command_parameters.parameter_6830;
    config_command_bits_ command_bits = adbms_raw->command_bit;

    ADBMS_Set_Config_B_6830(parameters.cfb6830,command_bits.cfg_b, NUM_6830, POSITION_6830);
    ADBMS_Write_Data(adbms_raw->SPI_data.hspi, WRCFGB, adbms_raw->command_bit.cfg_b, adbms_raw->SPI_data.spi_dataBuf);
    
}

void cell_Balance_Off(adbms_raw_* adbms)
{

    command_parameters_6830_ parameters = adbms->command_parameters.parameter_6830;
    config_command_bits_ command_bits = adbms->command_bit;

    // Turn off CB indication LED
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_RESET);

    for (int cic = 0; cic < (NUM_6830); cic++)
    {
        parameters.cfb6830[cic].dcc = 0;
    }

    ADBMS_Set_Config_B_6830(parameters.cfb6830,command_bits.cfg_b, NUM_6830, POSITION_6830);
    ADBMS_Write_Data(adbms->SPI_data.hspi, WRCFGB, adbms->command_bit.cfg_b, adbms->SPI_data.spi_dataBuf);
}

void Owc_c_channel_update(adbms_raw_* adbms_raw, adbms_6830_* adbms_6830)
{
    //Turn on Even, Read, Update
    Owc_C_Channel_Config(adbms_raw,Channel_Even_On);
    ADBMS_Read_Voltages();
    Update_6830_Owc_Fault(adbms_raw, adbms_6830);

    //Turn on Odd, Read, Update
    Owc_C_Channel_Config(adbms_raw,Channel_Odd_On);
    ADBMS_Read_Voltages();
    Update_6830_Owc_Fault(adbms_raw, adbms_6830);

    //Turn Off
    Owc_C_Channel_Config(adbms_raw,Channel_Off);
}

void Owc_s_channel_update(adbms_raw_* adbms_raw, adbms_6830_* adbms_6830)
{

    //Turn on Even, Read, Update
    Owc_S_Channel_Config(adbms_raw,Channel_Even_On);
    ADBMS_Read_Voltages();
    Update_6830_Owc_Fault(adbms_raw, adbms_6830);

    //Turn on Odd, Read, Update
    Owc_S_Channel_Config(adbms_raw,Channel_Odd_On);
    ADBMS_Read_Voltages();
    Update_6830_Owc_Fault(adbms_raw, adbms_6830);

    //Turn off
    Owc_S_Channel_Off(adbms_raw);
}


//
// Transfer Functions
//
float ADBMS_6830_Transfer_Voltage(int data)
{
    // voltage in Volts
    float voltage_float = ((data + 10000) * 0.000150);
    return voltage_float;
}

float ADBMS_6830_Transfer_Temp(float raw_temp_voltage, float Vref)
{
	// calc thermistor resistance
	float therm_resistance = (raw_temp_voltage * SeriesResistance) / (Vref - raw_temp_voltage);

	// calc temp from resistance
	float temp_kelvin = B / log(therm_resistance / (R25 * exp(-B / T25)));
	return temp_kelvin - 273.15; // Kelvin to Celsius
}



//
// Print Vals
//
void ADBMS_6830_Print_Vals(adbms_6830_* adbms_6830)
{
    printf("\nADBMS 6830 Data");
    printf("\nVOLTAGES\n");
    printf("total v: %f\n", adbms_6830->data.total_v);
    printf("max v: %f\t", adbms_6830->data.max_v);
    printf("min v: %f\t", adbms_6830->data.min_v);
    printf("avg v: %f\t", adbms_6830->data.avg_v);
    printf("max-min: %f\n", adbms_6830->data.max_v - adbms_6830->data.min_v);

    // print every voltage
    for (int i = 0; i < NUM_6830; i++)
    {
        if (i % 2 == 0)
        {
            for (int j = 0; j < NUM_VOLTAGES_EVEN_CHIP; j++)
            {
                printf("C%d=%fV\t", ((i * NUM_VOLTAGES_ODD_CHIP + (i + 1)/2) + j + 1), adbms_6830->data.voltages[(i * NUM_VOLTAGES_ODD_CHIP + (i + 1)/2) + j]);
            }
        }
        else
        {
            for (int j = 0; j < NUM_VOLTAGES_ODD_CHIP; j++)
            {
                printf("C%d=%fV\t", ((i * NUM_VOLTAGES_ODD_CHIP + (i + 1)/2) + j + 1), adbms_6830->data.voltages[(i * NUM_VOLTAGES_ODD_CHIP + (i + 1)/2) + j]);
            }
        }
    }
    printf("\n");

    // print the total, max, min, and avg temp
    printf("\nTEMPS\n");
    printf("max temp: %f\t", adbms_6830->data.max_temp);
    printf("min temp: %f\t", adbms_6830->data.min_temp);
    printf("avg temp: %f\n", adbms_6830->data.avg_temp);

    for (int i = 0; i < NUM_6830; i++)
    {
        for (int j = 0; j < NUM_TEMPS_CHIP; j++)
        {
            printf("T%d=%f\t", (i * NUM_TEMPS_CHIP + j + 1), adbms_6830->data.temperatures[i * NUM_TEMPS_CHIP + j]);
        }
        printf("\n");
    }

    printf("\nFaults\n");
    printf("undervoltage: %d\t", adbms_6830->faults.undervoltage_fault);
    printf("overvoltage: %d\t", adbms_6830->faults.overvoltage_fault);
    printf("pec: %d\t", adbms_6830->faults.pec_fault);
    printf("overtemperature: %d\t", adbms_6830->faults.overtemperature_fault);
    printf("openwire: %d\t", adbms_6830->faults.openwire_voltage_fault);
    printf("openwire_temp: %d\n", adbms_6830->faults.openwire_temp_fault);
}