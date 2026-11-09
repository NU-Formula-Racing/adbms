#include "adbms_interface_6830.h"
#include <math.h>


void ADBMS_6830_Parse_Voltage(adbms_read_raw_* raw_return, adbms_6830_voltage_parsed_* voltage_parsed)
{
    //if there is a pec, process and don't update values
    if(raw_return->read_pec_failure.pec_6830)
    {
        voltage_parsed->pec_counts += 1;
        return;
    }


    //initialize
    voltage_parsed->avg_v = 0;
    voltage_parsed->max_v = 0;
    voltage_parsed->min_v = FLT_MAX;

    float even_total = 0;
    float odd_total = 0;
    for (uint8_t cic = 0; cic < (NUM_6830); cic++)
    {
        if (cic % 2 == 0) //even chip, NUM_VOLTAGES_EVEN_CHIP voltages
        {
            uint8_t num_reg_grps = NUM_VOLTAGES_EVEN_CHIP / VOLTAGES_REG_GRP + (NUM_VOLTAGES_EVEN_CHIP % VOLTAGES_REG_GRP != 0);//find how many register groups used to store voltages
            for (uint8_t creg_grp = 0; creg_grp < num_reg_grps; creg_grp++) //index register groups, each contain 3 registers that represent a voltage
            {
                for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)//each reg_grp is 2 bytes (each reg is 1 byte) INDEX REGISTER THEN BYTES?
                {
                    if(creg_grp*DATA_LEN/2 + cbyte/2 >= NUM_VOLTAGES_EVEN_CHIP) break;  //stop processing registers when desired voltage reading count is reached
                    int16_t raw_val = (((uint16_t)raw_return->read_return[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | raw_return->read_return[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte];
                    float curr_voltage = ADBMS_6830_Transfer_Voltage(raw_val);

                    // check if adcs are turned off or
                    if(raw_val == MIN_6830_ADC_READING || raw_val == FAULT_6830_ADC_READING)
                    {
                        // register as a pec failure and set adc off flag
                        voltage_parsed->pec_counts += 1;
                        voltage_parsed->adc_off = true;
                        return;
                    }
                    
                    voltage_parsed->voltages[CHIP_VOLTAGE_OFFSET(cic) + creg_grp*DATA_LEN/2 + cbyte/2] = curr_voltage; //index section of current chip + index section of register group (group of 3 voltages) + index section the register (contains single voltage)

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
                    
                    // check if adcs are turned off
                    if(raw_val == MIN_6830_ADC_READING || raw_val == FAULT_6830_ADC_READING)
                    {
                        // register as a pec failure and set adc off flag
                        voltage_parsed->pec_counts += 1;
                        voltage_parsed->adc_off = true;
                        return;
                    }

                    voltage_parsed->voltages[CHIP_VOLTAGE_OFFSET(cic) + creg_grp*DATA_LEN/2 + cbyte/2] = curr_voltage; //index section of current chip + index section of register group (group of 3 voltages) + index section the register (contains single voltage)

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
    const unsigned cell_count = NUM_VOLTAGES;
    voltage_parsed->avg_v = cell_count > 0 ? voltage_parsed->total_v / cell_count : 0.0f;

    //reset current pec failures if there are no read pec failures
    voltage_parsed->pec_counts = 0;
    voltage_parsed->adc_off = false;
}


void ADBMS_6830_Parse_Temperature(adbms_read_raw_* raw_return, adbms_6830_temperature_parsed_* temp_parsed)
{
    //if there is a pec, process and don't update values
    if(raw_return->read_pec_failure.pec_6830)
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
            	if(creg_grp*DATA_LEN/2 + cbyte/2 >= NUM_TEMPS_CHIP) continue;	// only GPIO1..NUM_TEMPS_CHIP have thermistors

                int16_t raw_val = (((uint16_t)raw_return->read_return[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | raw_return->read_return[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte];
                float raw_temp_voltage = ADBMS_6830_Transfer_Voltage(raw_val);

                // get ref voltage from status reg - not getting status regs because takes too long
                //float vref = ADBMS_getVoltage(ICs[i].stata.vref2);
                float vref = 3; // 3V defined in the datasheet
                if (vref - raw_temp_voltage < 1e-1)
                    temp_parsed->openwire_temp_fault = true;

                float curr_temp = ADBMS_6830_Transfer_Temp(raw_temp_voltage, vref);
                temp_parsed->temperatures[cic*NUM_TEMPS_CHIP + creg_grp*DATA_LEN/2 + cbyte/2] = curr_temp;
                temp_parsed->gpio_v[cic*NUM_TEMPS_CHIP + creg_grp*DATA_LEN/2 + cbyte/2] = raw_temp_voltage;
                total_temp += curr_temp;
                if (curr_temp > temp_parsed->max_temp)
                    temp_parsed->max_temp = curr_temp;
                if (curr_temp < temp_parsed->min_temp)
                    temp_parsed->min_temp = curr_temp;
            }
        }
    }

    // calculate the avg temp
    const unsigned thermistor_count = NUM_TEMPS;
    temp_parsed->avg_temp = thermistor_count > 0 ? total_temp / thermistor_count : 0.0f;

}


void Update_6830_InternalFault(adbms_6830_* adbms_6830)
{
    // update ftti counts
    Update_6830_FTTI(adbms_6830);

    //all faults are latching
    // check overvoltage fault
    adbms_6830->faults.overvoltage_fault = adbms_6830->faults.overvoltage_fault 
                                            || (adbms_6830->faults.overvoltage_th_1_counts > FTTI_VOLTAGE_TH_1_COUNTS)
                                            || (adbms_6830->faults.overvoltage_th_2_counts > FTTI_VOLTAGE_TH_2_COUNTS)
                                            || (adbms_6830->voltage.max_v > OVERVOLTAGE + FTTI_VOLTAGE_TH_2);

    // check undervoltage fault
    adbms_6830->faults.undervoltage_fault = adbms_6830->faults.undervoltage_fault 
                                            || (adbms_6830->faults.undervoltage_th_1_counts > FTTI_VOLTAGE_TH_1_COUNTS)
                                            || (adbms_6830->faults.undervoltage_th_2_counts > FTTI_VOLTAGE_TH_2_COUNTS)
                                            || (adbms_6830->voltage.min_v < UNDERVOLTAGE - FTTI_VOLTAGE_TH_2 && adbms_6830->voltage.min_v > 0);

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

void Update_6830_Owc_Faults(adbms_6830_voltage_parsed_* voltage_owc, adbms_6830_faults_* faults, Owc_Channel_ channel)
{
    // Debug: show what each OW pass measured (called once for the even pass, then the odd pass)
    if (ENABLE_PRINTF_DEBUG_COMMS)
    {
        printf("OW %c pass:", channel == C_Channel ? 'C' : 'S');
        if (voltage_owc->pec_counts)
        {
            printf(" skipped - %s (count %d)\n",
                   voltage_owc->adc_off ? "ADC returned cleared/fault value" : "PEC failure", voltage_owc->pec_counts);
        }
        else
        {
            for (int i = 0; i < NUM_VOLTAGES; i++)
            {
                int cic = ADBMS_6830_Cell_Chip(i);
                if (i == CHIP_VOLTAGE_OFFSET(cic)) printf("%s IC%d:", cic ? " |" : "", cic + 1);
                printf(" %d:%.3f%s", i - CHIP_VOLTAGE_OFFSET(cic) + 1, voltage_owc->voltages[i], voltage_owc->voltages[i] < OWC_VOLTAGE_THRESHOLD ? "*" : "");
            }
            printf("   (* = below %.2f V -> open)\n", OWC_VOLTAGE_THRESHOLD);
        }
    }

    // pec check
    if (voltage_owc->pec_counts > PEC_FAILURE_THRESHOLD)
    {
        faults->pec_fault = true;
    }

    // Keep running tally of total pec failures for data collection and return
    if (voltage_owc->pec_counts) {
        faults->total_pec_failures++;
        return;
    }


    // owc check
    for (int i = 0; i < NUM_VOLTAGES; i++)
    {
        if (voltage_owc->voltages[i] < OWC_VOLTAGE_THRESHOLD){
            faults->openwire_voltage_fault = true;

            switch (channel)
            {
                case C_Channel:
                    faults->owc_c_faults[i] = 1;
                    break;

                case S_Channel:
                    faults->owc_s_faults[i] = 1;
                    break;
            }
        }  
    }
}

void cell_Balance_On(adbms_raw_* adbms_raw, adbms_6830_* adbms_6830)
{
    // Turn on CB indication LED
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_SET);
 
    for (int cic = 0; cic < (NUM_6830); cic++)
    {
        if (cic % 2 == 0) //even chip, NUM_VOLTAGES_EVEN_CHIP voltages
        {
            uint16_t dcc = 0;
            for (int cvoltage = 0; cvoltage < NUM_VOLTAGES_EVEN_CHIP; cvoltage++)
            {
                float curr_v = adbms_6830->voltage.voltages[CHIP_VOLTAGE_OFFSET(cic) + cvoltage];
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
                float curr_v = adbms_6830->voltage.voltages[CHIP_VOLTAGE_OFFSET(cic) + cvoltage];
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
    Update_6830_Owc_Faults(&adbms_6830->voltage_owc, &adbms_6830->faults, C_Channel);

    //Turn on Odd, Read, Update
    ADBMS_Owc_Config(adbms_raw, C_Channel, Channel_Odd_On);
    ADBMS_Read_Voltages(&adbms_raw->read_raw_c_owc, C_Channel_Read, adbms_raw->SPI_data.hspi, adbms_raw->SPI_data.spi_dataBuf);
    ADBMS_6830_Parse_Voltage(&adbms_raw->read_raw_c_owc, &adbms_6830->voltage_owc);
    Update_6830_Owc_Faults(&adbms_6830->voltage_owc, &adbms_6830->faults, C_Channel);

    //Turn Off
    ADBMS_Owc_Config(adbms_raw, C_Channel, Channel_Off);
}

void Owc_s_channel_update(adbms_raw_* adbms_raw, adbms_6830_* adbms_6830)
{
    //Turn on Even, Read, Update
    ADBMS_Owc_Config(adbms_raw, S_Channel, Channel_Even_On);
    ADBMS_Read_Voltages(&adbms_raw->read_raw_s_owc, S_Channel_Read, adbms_raw->SPI_data.hspi, adbms_raw->SPI_data.spi_dataBuf);
    ADBMS_6830_Parse_Voltage(&adbms_raw->read_raw_s_owc, &adbms_6830->voltage_owc);
    Update_6830_Owc_Faults(&adbms_6830->voltage_owc, &adbms_6830->faults, S_Channel);

    //Turn on Odd, Read, Update
    ADBMS_Owc_Config(adbms_raw,S_Channel,Channel_Odd_On);
    ADBMS_Read_Voltages(&adbms_raw->read_raw_s_owc, S_Channel_Read, adbms_raw->SPI_data.hspi, adbms_raw->SPI_data.spi_dataBuf);
    ADBMS_6830_Parse_Voltage(&adbms_raw->read_raw_s_owc, &adbms_6830->voltage_owc);
    Update_6830_Owc_Faults(&adbms_6830->voltage_owc, &adbms_6830->faults, S_Channel);

    //Turn off
    ADBMS_Owc_Config(adbms_raw, S_Channel, Channel_Off);
}


void Update_6830_FTTI(adbms_6830_* adbms_6830)
{
    // overvoltage
    if (adbms_6830->voltage.max_v > OVERVOLTAGE) {
        adbms_6830->faults.overvoltage_th_1_counts++;
    } else {
        adbms_6830->faults.overvoltage_th_1_counts = 0;
    }

    if (adbms_6830->voltage.max_v > OVERVOLTAGE + FTTI_VOLTAGE_TH_1) {
            adbms_6830->faults.overvoltage_th_2_counts++;
    } else {
        adbms_6830->faults.overvoltage_th_2_counts = 0;
    }


    // undervoltage
    if (adbms_6830->voltage.min_v < UNDERVOLTAGE && adbms_6830->voltage.min_v > 0) {
        adbms_6830->faults.undervoltage_th_1_counts++;
    } else {
        adbms_6830->faults.undervoltage_th_1_counts = 0;
    }
    
    if (adbms_6830->voltage.min_v < UNDERVOLTAGE - FTTI_VOLTAGE_TH_1 && adbms_6830->voltage.min_v > 0) {
            adbms_6830->faults.undervoltage_th_2_counts++;
    } else {
        adbms_6830->faults.undervoltage_th_2_counts = 0;
    }
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
                printf("C%d=%fV\t", (CHIP_VOLTAGE_OFFSET(i) + j + 1), adbms_6830->voltage.voltages[CHIP_VOLTAGE_OFFSET(i) + j]);
            }
        }
        else
        {
            for (int j = 0; j < NUM_VOLTAGES_ODD_CHIP; j++)
            {
                printf("C%d=%fV\t", (CHIP_VOLTAGE_OFFSET(i) + j + 1), adbms_6830->voltage.voltages[CHIP_VOLTAGE_OFFSET(i) + j]);
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

// Compact cell voltage print for bench testing
void ADBMS_6830_Print_Voltages(adbms_6830_* adbms_6830)
{
    adbms_6830_voltage_parsed_ *v = &adbms_6830->voltage;

    printf("\n--- ADBMS6830 cell voltages (t=%lu ms) ---\n", (unsigned long)HAL_GetTick());
    if (v->pec_counts) printf("PEC failures in a row: %d (values below are from the last good read)\n", v->pec_counts);
    if (v->adc_off)    printf("ADC returned cleared/fault value - ADCs restarted\n");

    // one block per 6830, cells numbered locally (IC1 C1..C10, IC2 C1..C10, ...), 5 per row
    for (int cic = 0; cic < NUM_6830; cic++)
    {
        int first = CHIP_VOLTAGE_OFFSET(cic);
        int n = CHIP_VOLTAGE_OFFSET(cic + 1) - first;
        float chip_total = 0;
        for (int j = 0; j < n; j++) chip_total += v->voltages[first + j];

        printf("IC%d (%.4f V)\n", cic + 1, chip_total);
        for (int j = 0; j < n; j++)
            printf("  C%-2d %7.4f V%s", j + 1, v->voltages[first + j], ((j + 1) % 5 == 0 || j == n - 1) ? "\n" : " ");
    }
    printf("total %8.4f V   max %7.4f V   min %7.4f V   avg %7.4f V   max-min %7.4f V\n",
           v->total_v, v->max_v, v->min_v, v->avg_v, v->max_v - v->min_v);
}

// Thermistor input print for bench testing. Divider: VREF2 (3 V) -> 10k pull-up -> GPIO -> NTC -> V-
//   open (nothing connected)  -> GPIO ~3.0 V
//   shorted                   -> GPIO ~0 V
//   10k resistor (= 25 C NTC) -> GPIO ~1.5 V
void ADBMS_6830_Print_Temps(adbms_6830_* adbms_6830)
{
    adbms_6830_temperature_parsed_ *t = &adbms_6830->temperature;

    if (t->pec_counts) printf("TEMP PEC failures in a row: %d (values below are from the last good read)\n", t->pec_counts);
    for (int i = 0; i < NUM_TEMPS; i++)
    {
        int cic = i / NUM_TEMPS_CHIP, j = i % NUM_TEMPS_CHIP;	// thermistor j is on GPIO(j+1) of chip cic
        if (TEMP_PRINT_IC && cic + 1 != TEMP_PRINT_IC) continue;	// only print the selected IC
        float v = t->gpio_v[i];
        float r = (v < 2.999f) ? v * SeriesResistance / (3.0f - v) : -1.0f;
        const char *state = (v > 2.9f) ? "OPEN" : (v < 0.05f) ? "SHORT" : "ok";
        if (j == 0) printf("IC%d temps\n", cic + 1);
        printf("  T%-2d %6.3f V  %8.0f ohm  %7.1f C  %-5s%s", j + 1, v, r, t->temperatures[i], state,
               ((j + 1) % 2 == 0 || j == NUM_TEMPS_CHIP - 1) ? "\n" : "   |");
    }
    printf("temp max %.1f C   min %.1f C   avg %.1f C   openwire_temp_fault %d\n",
           t->max_temp, t->min_temp, t->avg_temp, adbms_6830->faults.openwire_temp_fault);
}
