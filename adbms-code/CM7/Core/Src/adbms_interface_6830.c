#include "adbms_interface_6830.h"
#include <math.h>


void ADBMS_6830_Parse_Voltage(adbms_read_raw_* raw_return, adbms_6830_voltage_parsed_* voltage_parsed)
{
    //if there is a pec, process and don't update values
    if(raw_return->read_pec_failure)
    {
        voltage_parsed->pec_counts += 1;
        return;
    }

    //reset current pec failures if there are no read pec failures
    voltage_parsed->pec_counts = 0;


    //initialize
    voltage_parsed->avg_v = 0;
    voltage_parsed->max_v = 0;
    voltage_parsed->min_v = FLT_MAX;

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
                    int16_t raw_val = (((uint16_t)raw_return->read_return[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | raw_return->read_return[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte];
                    float curr_voltage = ADBMS_6830_Transfer_Voltage(raw_val);
                    
                    voltage_parsed->voltages[(cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2] = curr_voltage; //index section of current chip + index section of register group (group of 3 voltages) + index section the register (contains single voltage)

                    even_total += curr_voltage;
                    if (curr_voltage > voltage_parsed->max_v){
                        voltage_parsed->max_v = curr_voltage;
                    }
                    if (curr_voltage < voltage_parsed->min_v){
                        voltage_parsed->min_v = curr_voltage;
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

                    int16_t raw_val = (((uint16_t)raw_return->read_return[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | raw_return->read_return[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte];
                    float curr_voltage = ADBMS_6830_Transfer_Voltage(raw_val);
                    
                    voltage_parsed->voltages[(cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2] = curr_voltage; //index section of current chip + index section of register group (group of 3 voltages) + index section the register (contains single voltage)

                    odd_total += curr_voltage;
                    if (curr_voltage > voltage_parsed->max_v){
                        voltage_parsed->max_v = curr_voltage;
                    }
                    if (curr_voltage < voltage_parsed->min_v){
                        voltage_parsed->min_v = curr_voltage;
                    }
                }
            }
        }
    }
    
    voltage_parsed->total_v = even_total + odd_total;
    // calculate the avg voltage
    if(NUM_6830 > 0){
        voltage_parsed->avg_v = voltage_parsed->total_v / ((NUM_6830) * NUM_VOLTAGES_ODD_CHIP + (((NUM_6830+1))/2));
    }
    else{
        voltage_parsed->avg_v = 0.0;
    }
}


void ADBMS_6830_Parse_Temperature(adbms_read_raw_* raw_return, adbms_6830_temperature_parsed_* temp_parsed)
{
    //if there is a pec, process and don't update values
    if(raw_return->read_pec_failure)
    {
        temp_parsed->pec_counts += 1;
        return;
    }

    //reset current pec failures if there are no read pec failures
    temp_parsed->pec_counts = 0;

    //initialize
    float total_temp = 0.0;
    temp_parsed->max_temp = 0;
    temp_parsed->min_temp = FLT_MAX;
    temp_parsed->openwire_temp_fault = false;
    
    for (int cic = 0; cic < (NUM_6830); cic++)
    {
        for (uint8_t creg_grp = 0; creg_grp < AUX_REG_GRP; creg_grp++)
        {
            for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)
            {
            	if(creg_grp*DATA_LEN/2 + cbyte/2 >= AUX_GPIO) continue;	// only 10 gpio's

                int16_t raw_val = (((uint16_t)raw_return->read_return[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | raw_return->read_return[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte];
                float raw_temp_voltage = ADBMS_6830_Transfer_Voltage(raw_val);

                // get ref voltage from status reg - not getting status regs because takes too long
                //float vref = ADBMS_getVoltage(ICs[i].stata.vref2);
                float vref = 3; // 3V defined in the datasheet
                if (vref - raw_temp_voltage < 1e-1)
                    temp_parsed->openwire_temp_fault = true;

                float curr_temp = ADBMS_6830_Transfer_Temp(raw_temp_voltage, vref);
                temp_parsed->temperatures[cic*NUM_TEMPS_CHIP + creg_grp*DATA_LEN/2 + cbyte/2] = curr_temp;
                total_temp += curr_temp;
                if (curr_temp > temp_parsed->max_temp)
                    temp_parsed->max_temp = curr_temp;
                if (curr_temp < temp_parsed->min_temp)
                    temp_parsed->min_temp = curr_temp;
            }
        }
    }

    // calculate the avg temp
    if (NUM_CHIPS > 1){
        temp_parsed->avg_temp = total_temp / ((NUM_6830) * NUM_TEMPS_CHIP);
    }
    else{
        temp_parsed->avg_temp = 0.0;
    }

}


void Update_6830_InternalFault(adbms_6830_* adbms_6830)
{
    //all faults are latching

    // check overvoltage fault
    adbms_6830->faults.overvoltage_fault = adbms_6830->faults.overvoltage_fault || (adbms_6830->voltage.max_v > OVERVOLTAGE);

    // check undervoltage fault
    adbms_6830->faults.undervoltage_fault = adbms_6830->faults.undervoltage_fault || (adbms_6830->voltage.min_v < UNDERVOLTAGE);

    // check overtemperature fault
    adbms_6830->faults.overtemperature_fault = adbms_6830->faults.overtemperature_fault || (adbms_6830->temperature.max_temp > OVERTEMP);
   
    // check undertemperature fault
    adbms_6830->faults.undertemperature_fault = adbms_6830->faults.undertemperature_fault || (adbms_6830->temperature.min_temp < UNDERTEMP);

    // check openwire temp fault
    adbms_6830->faults.openwire_temp_fault = adbms_6830->faults.openwire_temp_fault || (adbms_6830->temperature.openwire_temp_fault);


    // check pec fault
    if ((adbms_6830->voltage.pec_counts > PEC_FAILURE_THRESHOLD) || (adbms_6830->temperature.pec_counts > PEC_FAILURE_THRESHOLD))
    {
        adbms_6830->faults.pec_fault = true;
    }
    
    // Keep running tally of total pec failures for data collection
    if (adbms_6830->voltage.pec_counts) adbms_6830->faults.total_pec_failures++;
    if (adbms_6830->temperature.pec_counts) adbms_6830->faults.total_pec_failures++;
}

void Update_6830_Owc_Faults(adbms_6830_voltage_parsed_* voltage_owc, adbms_6830_faults_* faults)
{
    // pec check
    if (voltage_owc->pec_counts > PEC_FAILURE_THRESHOLD)
    {
        faults->pec_fault = true;
    }

    // Keep running tally of total pec failures for data collection
    if (voltage_owc->pec_counts) faults->total_pec_failures++;


    // owc check
    for (int i = 0; i < NUM_VOLTAGES; i++)
    {
        if (voltage_owc->voltages[i] < OWC_VOLTAGE_THRESHOLD){
            faults->openwire_voltage_fault = true;
        }  
    }
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
                float curr_v = adbms_6830->voltage.voltages[(cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + cvoltage];
                if ((curr_v - adbms_6830->voltage.min_v) > CB_THRESHOLD && curr_v > CB_MIN_V_THRESHOLD)
                {
                    dcc |= 1 << cvoltage;
                }
            }
            adbms_raw->command_parameters.parameter_6830.cfb6830[cic].dcc = dcc;
        }
        else //odd chip, 11 voltages
        {
            uint16_t dcc = 0;
            for (int cvoltage = 0; cvoltage < NUM_VOLTAGES_ODD_CHIP; cvoltage++)
            {
                float curr_v = adbms_6830->voltage.voltages[(cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + cvoltage];
                if ((curr_v - adbms_6830->voltage.min_v) > CB_THRESHOLD && curr_v > CB_MIN_V_THRESHOLD)
                {
                    dcc |= 1 << cvoltage;
                }
            }
            adbms_raw->command_parameters.parameter_6830.cfb6830[cic].dcc = dcc;
        }
    }

    ADBMS_Set_Config_B_6830(adbms_raw->command_parameters.parameter_6830.cfb6830, adbms_raw->command_bit.cfg_b, NUM_6830, POSITION_6830);
    ADBMS_Write_Data(adbms_raw->SPI_data.hspi, WRCFGB, adbms_raw->command_bit.cfg_b, adbms_raw->SPI_data.spi_dataBuf);
    
}

void cell_Balance_Off(adbms_raw_* adbms_raw)
{
    // Turn off CB indication LED
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_RESET);

    for (int cic = 0; cic < (NUM_6830); cic++)
    {
        adbms_raw->command_parameters.parameter_6830.cfb6830[cic].dcc = 0;
    }

    ADBMS_Set_Config_B_6830(adbms_raw->command_parameters.parameter_6830.cfb6830, adbms_raw->command_bit.cfg_b, NUM_6830, POSITION_6830);
    ADBMS_Write_Data(adbms_raw->SPI_data.hspi, WRCFGB, adbms_raw->command_bit.cfg_b, adbms_raw->SPI_data.spi_dataBuf);
}

void Owc_c_channel_update(adbms_raw_* adbms_raw, adbms_6830_* adbms_6830)
{
    //Turn on Even, Read, Update
    ADBMS_Owc_Config(adbms_raw, C_Channel, Channel_Even_On);
    ADBMS_Read_Voltages(&adbms_raw->read_raw_c_owc, C_Channel_Read, adbms_raw->SPI_data.hspi, adbms_raw->SPI_data.spi_dataBuf);
    ADBMS_6830_Parse_Voltage(&adbms_raw->read_raw_c_owc, &adbms_6830->voltage_owc);
    Update_6830_Owc_Faults(&adbms_6830->voltage_owc, &adbms_6830->faults);

    //Turn on Odd, Read, Update
    ADBMS_Owc_Config(adbms_raw, C_Channel, Channel_Odd_On);
    ADBMS_Read_Voltages(&adbms_raw->read_raw_c_owc, C_Channel_Read, adbms_raw->SPI_data.hspi, adbms_raw->SPI_data.spi_dataBuf);
    ADBMS_6830_Parse_Voltage(&adbms_raw->read_raw_c_owc, &adbms_6830->voltage_owc);
    Update_6830_Owc_Faults(&adbms_6830->voltage_owc, &adbms_6830->faults);

    //Turn Off
    ADBMS_Owc_Config(adbms_raw, C_Channel, Channel_Off);
}

void Owc_s_channel_update(adbms_raw_* adbms_raw, adbms_6830_* adbms_6830)
{

    //Turn on Even, Read, Update
    ADBMS_Owc_Config(adbms_raw, S_Channel, Channel_Even_On);
    ADBMS_Read_Voltages(&adbms_raw->read_raw_s_owc, S_Channel_Read, adbms_raw->SPI_data.hspi, adbms_raw->SPI_data.spi_dataBuf);
    ADBMS_6830_Parse_Voltage(&adbms_raw->read_raw_s_owc, &adbms_6830->voltage_owc);
    Update_6830_Owc_Faults(&adbms_6830->voltage_owc, &adbms_6830->faults);

    //Turn on Odd, Read, Update
    ADBMS_Owc_Config(adbms_raw,S_Channel,Channel_Odd_On);
    ADBMS_Read_Voltages(&adbms_raw->read_raw_s_owc, S_Channel_Read, adbms_raw->SPI_data.hspi, adbms_raw->SPI_data.spi_dataBuf);
    ADBMS_6830_Parse_Voltage(&adbms_raw->read_raw_s_owc, &adbms_6830->voltage_owc);
    Update_6830_Owc_Faults(&adbms_6830->voltage_owc, &adbms_6830->faults);

    //Turn off
    ADBMS_Owc_Config(adbms_raw, S_Channel, Channel_Off);
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
    printf("total v: %f\n", adbms_6830->voltage.total_v);
    printf("max v: %f\t", adbms_6830->voltage.max_v);
    printf("min v: %f\t", adbms_6830->voltage.min_v);
    printf("avg v: %f\t", adbms_6830->voltage.avg_v);
    printf("max-min: %f\n", adbms_6830->voltage.max_v - adbms_6830->voltage.min_v);

    // print every voltage
    for (int i = 0; i < NUM_6830; i++)
    {
        if (i % 2 == 0)
        {
            for (int j = 0; j < NUM_VOLTAGES_EVEN_CHIP; j++)
            {
                printf("C%d=%fV\t", ((i * NUM_VOLTAGES_ODD_CHIP + (i + 1)/2) + j + 1), adbms_6830->voltage.voltages[(i * NUM_VOLTAGES_ODD_CHIP + (i + 1)/2) + j]);
            }
        }
        else
        {
            for (int j = 0; j < NUM_VOLTAGES_ODD_CHIP; j++)
            {
                printf("C%d=%fV\t", ((i * NUM_VOLTAGES_ODD_CHIP + (i + 1)/2) + j + 1), adbms_6830->voltage.voltages[(i * NUM_VOLTAGES_ODD_CHIP + (i + 1)/2) + j]);
            }
        }
    }
    printf("\n");

    // print the total, max, min, and avg temp
    printf("\nTEMPS\n");
    printf("max temp: %f\t", adbms_6830->temperature.max_temp);
    printf("min temp: %f\t", adbms_6830->temperature.min_temp);
    printf("avg temp: %f\n", adbms_6830->temperature.avg_temp);

    for (int i = 0; i < NUM_6830; i++)
    {
        for (int j = 0; j < NUM_TEMPS_CHIP; j++)
        {
            printf("T%d=%f\t", (i * NUM_TEMPS_CHIP + j + 1), adbms_6830->temperature.temperatures[i * NUM_TEMPS_CHIP + j]);
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
