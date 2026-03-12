#include "adbms_interface.h"

void ADBMS_Initialize(adbms_ *adbms, SPI_HandleTypeDef *hspi)
{
    adbms->ICs.hspi = hspi;
    // Set initial configurations
    for (uint8_t cic = 0; cic < NUM_CHIPS; cic++)
    {
        // Init config A
        adbms->cfa[cic].refon = 1;
        adbms->cfa[cic].gpo = 0x3FF;  // all gpo tunred on

        // Init config B
        adbms->cfb[cic].vuv = Set_UnderOver_Voltage_Threshold(UNDERVOLTAGE);
        adbms->cfb[cic].vov = Set_UnderOver_Voltage_Threshold(OVERVOLTAGE);
    }
    // Init sensing cmd
    adbms->adcv.cont = 1;

    // Package config and sensing structs into transmitable data
    ADBMS_Set_Config_A(adbms->cfa, adbms->ICs.cfg_a);
    ADBMS_Set_Config_B(adbms->cfb, adbms->ICs.cfg_b);
    ADBMS_Set_ADCV(adbms->adcv, &adbms->ICs.adcv);
    ADBMS_Set_ADAX(adbms->adax, &adbms->ICs.adax);

    // Write Config 
    ADBMS_WakeUP_ICs();
    ADBMS_WakeUP_ICs();
    ADBMS_Write_Data(adbms->ICs.hspi, WRCFGA, adbms->ICs.cfg_a, adbms->ICs.spi_dataBuf);
    ADBMS_WakeUP_ICs();
    ADBMS_Write_Data(adbms->ICs.hspi, WRCFGB, adbms->ICs.cfg_b, adbms->ICs.spi_dataBuf);

    // Turn on sensing
    ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adcv, adbms->ICs.cmd_buf);
    HAL_Delay(1);
    ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adax, adbms->ICs.cmd_buf);
    HAL_Delay(8); // ADCs are updated at their conversion rate of 1ms
}

void ADBMS_UpdateVoltages(adbms_ *adbms)
{
    // get voltages from ADBMS
    bool pec = 0;
    ADBMS_WakeUP_ICs();

    //getting to the
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVA, (adbms->ICs.cell + 0 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf); //read voltages 0-2 for each chip
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVB, (adbms->ICs.cell + 1 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf); //read voltages 3-5 for each chip
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVC, (adbms->ICs.cell + 2 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf); //read voltages 6-8 for each chip
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVD, (adbms->ICs.cell + 3 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf); //read voltages 9-11 for each chip
    //pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVE, (adbms->ICs.cell + 4 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf); //DONT NEED VOLTAGES OVER 12 for nfr26
    adbms->voltage_pec_failure = pec;

    // calulate new values with the updated raw ones
    ADBMS_CalculateValues_Voltages(adbms);
}

void ADBMS_UpdateTemps(adbms_ *adbms)
{
    // get temps from ADBMS
    bool pec = 0;
    ADBMS_WakeUP_ICs();
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDAUXA, (adbms->ICs.aux + 0 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDAUXB, (adbms->ICs.aux + 1 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDAUXC, (adbms->ICs.aux + 2 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDAUXD, (adbms->ICs.aux + 3 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf);
    adbms->temp_pec_failure = pec;

    // need to start new poll for conversion before next read (no continous mode)
    ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adax, adbms->ICs.cmd_buf);

    // calulate new values with the updated raw ones
    ADBMS_CalculateValues_Temps(adbms);
}

void ADBMS_CalculateValues_Voltages(adbms_ *adbms)
{
    // reset current pec failures if there is no current failure
    if(!adbms->voltage_pec_failure && !adbms->temp_pec_failure && !adbms->status_reg_pec_failure) { 
        adbms->current_pec_failures = 0;
    }

    // if there is a pec failure, process it and don't update values
    if(adbms->voltage_pec_failure) {
        adbms->current_pec_failures += adbms->voltage_pec_failure;
        if(adbms->current_pec_failures > PEC_FAILURE_THRESHOLD) {
            adbms->pec_fault_ = 1;
        }else {
            adbms->pec_fault_ = 0;
        }
        return;
    }

    // calculate the total, max, and min voltage
    adbms->total_v = 0;
    adbms->max_v = 0;
    adbms->min_v = FLT_MAX;
    float even_total = 0;
    float odd_total = 0;
    for (uint8_t cic = 0; cic < NUM_CHIPS; cic++)
    {
        if (cic % 2 == 0) //even chip, 12 voltages
        {
            uint8_t num_reg_grps = NUM_VOLTAGES_EVEN_CHIP / VOLTAGES_REG_GRP + (NUM_VOLTAGES_EVEN_CHIP % VOLTAGES_REG_GRP != 0);//find how many register groups used to store voltages
            for (uint8_t creg_grp = 0; creg_grp < num_reg_grps; creg_grp++) //index register groups, each contain 3 registers that represent a voltage
            {
                for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)//each reg_grp is 2 bytes (each reg is 1 byte) INDEX REGISTER THEN BYTES?
                {
                    if(creg_grp*DATA_LEN/2 + cbyte/2 >= NUM_VOLTAGES_EVEN_CHIP) break;  //stop processing registers when desired voltage reading count is reached
                    int16_t raw_val = (((uint16_t)adbms->ICs.cell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.cell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    float curr_voltage = ADBMS_getVoltage(raw_val);
                    
                    adbms->voltages[(cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2] = curr_voltage; //index section of current chip + index section of register group (group of 3 voltages) + index section the register (contains single voltage)

                    even_total += curr_voltage;
                    if (curr_voltage > adbms->max_v){
                        adbms->max_v = curr_voltage;
                    }
                    if (curr_voltage < adbms->min_v){
                        adbms->min_v = curr_voltage;
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

                    int16_t raw_val = (((uint16_t)adbms->ICs.cell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.cell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    float curr_voltage = ADBMS_getVoltage(raw_val);
                    
                    adbms->voltages[(cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2] = curr_voltage; //index section of current chip + index section of register group (group of 3 voltages) + index section the register (contains single voltage)

                    odd_total += curr_voltage;
                    if (curr_voltage > adbms->max_v){
                        adbms->max_v = curr_voltage;
                    }
                    if (curr_voltage < adbms->min_v){
                        adbms->min_v = curr_voltage;
                    }
                }
            }
        }
    }
    
    // adbms->total_v = even_total + odd_total;
    adbms->total_v = 60;
    // calculate the avg voltage
    adbms->avg_v = adbms->total_v / (NUM_CHIPS * NUM_VOLTAGES_ODD_CHIP + ((NUM_CHIPS + 1)/2));
}

void ADBMS_CalculateValues_Temps(adbms_ *adbms)
{
    // reset current pec failures if there is no current failure
    if(!adbms->voltage_pec_failure && !adbms->temp_pec_failure && !adbms->status_reg_pec_failure) { 
        adbms->current_pec_failures = 0;
    }

    // if there is a pec failure, process it and don't update values
    if(adbms->temp_pec_failure) {
        adbms->current_pec_failures += adbms->temp_pec_failure;
        if(adbms->current_pec_failures > PEC_FAILURE_THRESHOLD) {
            adbms->pec_fault_ = 1;
        }else {
            adbms->pec_fault_ = 0;
        }
        return;
    }
    
    // calculate the total, max, and min temp
    float total_temp = 0.0;
    adbms->max_temp = 0;
    adbms->min_temp = FLT_MAX;
    bool openwire_temp_fault = false;
    for (int cic = 0; cic < NUM_CHIPS; cic++)
    {
        for (uint8_t creg_grp = 0; creg_grp < AUX_REG_GRP; creg_grp++)
        {
            for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)
            {
            	if(creg_grp*DATA_LEN/2 + cbyte/2 >= AUX_GPIO) continue;	// only 10 gpio's

                int16_t raw_val = (((uint16_t)adbms->ICs.aux[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.aux[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                float raw_temp_voltage = ADBMS_getVoltage(raw_val);

                // get ref voltage from status reg - not getting status regs because takes too long
                //float vref = ADBMS_getVoltage(ICs[i].stata.vref2);
                float vref = 3; // 3V defined in the datasheet
                if (vref - raw_temp_voltage < 1e-1)
                    openwire_temp_fault = true;

                float curr_temp = getTemp(raw_temp_voltage, vref);
                adbms->temperatures[cic*NUM_TEMPS_CHIP + creg_grp*DATA_LEN/2 + cbyte/2] = curr_temp;
                total_temp += curr_temp;
                if (curr_temp > adbms->max_temp)
                    adbms->max_temp = curr_temp;
                if (curr_temp < adbms->min_temp)
                    adbms->min_temp = curr_temp;
            }
        }
    }
    // adbms->openwire_temp_fault_ = adbms->openwire_temp_fault_ || openwire_temp_fault;
    adbms->openwire_temp_fault_ = openwire_temp_fault;

    // calculate the avg temp
    adbms->avg_temp = total_temp / (NUM_CHIPS * NUM_TEMPS_CHIP);
    
}

void UpdateADInternalFault(adbms_ *adbms)
{
    // check overvoltage fault
    adbms->overvoltage_fault_ = adbms->overvoltage_fault_ || (adbms->max_v > OVERVOLTAGE);

    // check undervoltage fault
    adbms->undervoltage_fault_ = adbms->undervoltage_fault_ || (adbms->min_v < UNDERVOLTAGE);

    // check overtemperature fault
    adbms->overtemperature_fault_ = adbms->overtemperature_fault_ || (adbms->max_temp > OVERTEMP);

    // check undertemperature fault
    adbms->undertemperature_fault_ = adbms->undertemperature_fault_ || (adbms->min_temp < UNDERTEMP);

    // TODO: check status regs for faults - need calcuate status reg values fn that handles status reg pec fualts
}

void cellBalanceOn(adbms_ *adbms)
{
    // Turn on CB indication LED
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);

    for (int cic = 0; cic < NUM_CHIPS; cic++)
    {
        if (cic % 2 == 0) //even chip, 12 voltages
        {
            uint16_t dcc = 0;
            for (int cvoltage = 0; cvoltage < NUM_VOLTAGES_EVEN_CHIP; cvoltage++)
            {
                float curr_v = adbms->voltages[(cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + cvoltage];
                if ((curr_v - adbms->min_v) > CB_THRESHOLD && curr_v > CB_MIN_V_THRESHOLD)
                {
                    dcc |= 1 << cvoltage;
                }
            }
            adbms->cfb[cic].dcc = dcc;
        }
        else //odd chip, 11 voltages
        {
            uint16_t dcc = 0;
            for (int cvoltage = 0; cvoltage < NUM_VOLTAGES_ODD_CHIP; cvoltage++)
            {
                float curr_v = adbms->voltages[(cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + cvoltage];
                if ((curr_v - adbms->min_v) > CB_THRESHOLD && curr_v > CB_MIN_V_THRESHOLD)
                {
                    dcc |= 1 << cvoltage;
                }
            }
            adbms->cfb[cic].dcc = dcc;
        }
    }
    ADBMS_Set_Config_B(adbms->cfb, adbms->ICs.cfg_b);
    ADBMS_Write_Data(adbms->ICs.hspi, WRCFGB, adbms->ICs.cfg_b, adbms->ICs.spi_dataBuf);
}

void cellBalanceOff(adbms_ *adbms)
{
    // Turn off CB indication LED
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);

    for (int cic = 0; cic < NUM_CHIPS; cic++)
    {
        adbms->cfb[cic].dcc = 0;
    }
    ADBMS_Set_Config_B(adbms->cfb, adbms->ICs.cfg_b);
    ADBMS_Write_Data(adbms->ICs.hspi, WRCFGB, adbms->ICs.cfg_b, adbms->ICs.spi_dataBuf);
}

void Update_Owc_Fault(adbms_ *adbms)
{
    // check openwire fault
    ADBMS_WakeUP_ICs();
    cellBalanceOff(adbms);   // need to turn off cell balancing to check for OWC

    /// OWC EVEN Check
    adbms->adsv.cont = 1;
    adbms->adsv.ow = 1; // Enable OW on even-channel 
    ADBMS_Set_ADSV(adbms->adsv, &adbms->ICs.adsv);
    ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adsv, adbms->ICs.cmd_buf);
    HAL_Delay(8);    // S-Channels are updated at 8ms

    // Get new s-channel voltages
    bool pec = 0;
    ADBMS_WakeUP_ICs();
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVA, (adbms->ICs.scell + 0 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVB, (adbms->ICs.scell + 1 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVC, (adbms->ICs.scell + 2 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVD, (adbms->ICs.scell + 3 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVE, (adbms->ICs.scell + 4 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf); // probably don't need this

    if(pec){
        adbms->current_owc_failures += 1;
        if(adbms->current_owc_failures > PEC_FAILURE_THRESHOLD){
            adbms->pec_fault_ = 1;
        }
        return;
    }else adbms->current_owc_failures = 0;

    for (uint8_t cic = 0; cic < NUM_CHIPS; cic++)
    {
        if( cic % 2 == 0) //even chip
        {
            uint8_t num_reg_grps = NUM_VOLTAGES_EVEN_CHIP / VOLTAGES_REG_GRP + (NUM_VOLTAGES_EVEN_CHIP % VOLTAGES_REG_GRP != 0);//find how many register groups used to store voltages
            for (uint8_t creg_grp = 0; creg_grp < num_reg_grps; creg_grp++)
            {
                for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)
                {
                    if(creg_grp*DATA_LEN/2 + cbyte/2 >= NUM_VOLTAGES_EVEN_CHIP) break;
                    int16_t raw_val = (((uint16_t)adbms->ICs.scell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.scell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    if (ADBMS_getVoltage(raw_val) < 0.5)
                    {
                        int c_cell = (cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2;
                        adbms->openwire_fault_ = 1;
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
                    int16_t raw_val = (((uint16_t)adbms->ICs.scell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.scell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    if (ADBMS_getVoltage(raw_val) < 0.5)
                    {
                        int c_cell = (cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2;
                        adbms->openwire_fault_ = 1;
                    }
                }
            }
        }
    }

    /// OWC ODD Check
    adbms->adsv.cont = 1;
    adbms->adsv.ow = 2; // Enable OW on odd-channel 
    ADBMS_Set_ADSV(adbms->adsv, &adbms->ICs.adsv);
    ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adsv, adbms->ICs.cmd_buf);
    HAL_Delay(8);    // S-Channels are updated at 8ms

    // Get new s-channel voltages
    ADBMS_WakeUP_ICs();
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVA, (adbms->ICs.scell + 0 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVB, (adbms->ICs.scell + 1 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVC, (adbms->ICs.scell + 2 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVD, (adbms->ICs.scell + 3 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVE, (adbms->ICs.scell + 4 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf); // probably don't need this

    if(pec){
        adbms->current_owc_failures += 1;
        if(adbms->current_owc_failures > PEC_FAILURE_THRESHOLD){
            adbms->pec_fault_ = 1;
        }
        return;
    }else adbms->current_owc_failures = 0;

    for (uint8_t cic = 0; cic < NUM_CHIPS; cic++)
    {
        if (cic % 2 == 0)
        {
            uint8_t num_reg_grps = NUM_VOLTAGES_EVEN_CHIP / VOLTAGES_REG_GRP + (NUM_VOLTAGES_EVEN_CHIP% VOLTAGES_REG_GRP != 0);
            for (uint8_t creg_grp = 0; creg_grp < num_reg_grps; creg_grp++)
            {
                for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)
                {
                    if(creg_grp*DATA_LEN/2 + cbyte/2 >= NUM_VOLTAGES_EVEN_CHIP) break;  
                    int16_t raw_val = (((uint16_t)adbms->ICs.scell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.scell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    if (ADBMS_getVoltage(raw_val) < 0.5)
                    {
                        int c_cell = (cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2;
                        adbms->openwire_fault_ = 1;
                    }
                }
            }
        }
        else
        {
            uint8_t num_reg_grps = NUM_VOLTAGES_ODD_CHIP / VOLTAGES_REG_GRP + (NUM_VOLTAGES_ODD_CHIP % VOLTAGES_REG_GRP != 0);
            for (uint8_t creg_grp = 0; creg_grp < num_reg_grps; creg_grp++)
            {
                for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)
                {
                    if(creg_grp*DATA_LEN/2 + cbyte/2 >= NUM_VOLTAGES_ODD_CHIP) break;
                    int16_t raw_val = (((uint16_t)adbms->ICs.scell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.scell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    if (ADBMS_getVoltage(raw_val) < 0.5)
                    {
                        int c_cell = (cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2;
                        adbms->openwire_fault_ = 1;
                    }
                }
            }
        }
    }

    /// Turn off owc
    adbms->adsv.cont = 0;
    adbms->adsv.ow = 0; // Enable OW on odd-channel 
    ADBMS_Set_ADSV(adbms->adsv, &adbms->ICs.adsv);
    ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adsv, adbms->ICs.cmd_buf);
    HAL_Delay(1);    // S-Channels are updated at 8ms
}


void Update_Owc_C_Channel_Fault(adbms_ *adbms)
{
    // check openwire fault
    ADBMS_WakeUP_ICs();

    adbms->adcv.cont = 1;
    adbms->adcv.ow = 1; // Enable OW on even-channel 
    ADBMS_Set_ADCV(adbms->adcv, &adbms->ICs.adcv);
    ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adcv, adbms->ICs.cmd_buf);
    HAL_Delay(8);    // C-Channels are updated at 1ms

    // Get new C-channel voltages
    bool pec = 0;
    ADBMS_WakeUP_ICs();
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVA, (adbms->ICs.cell + 0 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf); //read voltages 0-2 for each chip
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVB, (adbms->ICs.cell + 1 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf); //read voltages 3-5 for each chip
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVC, (adbms->ICs.cell + 2 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf); //read voltages 6-8 for each chip
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVD, (adbms->ICs.cell + 3 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf); //read voltages 9-11 for each chip
    //pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVE, (adbms->ICs.cell + 4 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf); //DONT NEED VOLTAGES OVER 12 for nfr26

    if(pec){
        adbms->current_owc_failures += 1;
        if(adbms->current_owc_failures > PEC_FAILURE_THRESHOLD){
            adbms->pec_fault_ = 1;
        }
        return;
    }else adbms->current_owc_failures = 0;

    for (uint8_t cic = 0; cic < NUM_CHIPS; cic++)
    {
        if( cic % 2 == 0) //even chip
        {
            uint8_t num_reg_grps = NUM_VOLTAGES_EVEN_CHIP / VOLTAGES_REG_GRP + (NUM_VOLTAGES_EVEN_CHIP % VOLTAGES_REG_GRP != 0);//find how many register groups used to store voltages
            for (uint8_t creg_grp = 0; creg_grp < num_reg_grps; creg_grp++)
            {
                for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)
                {
                    if(creg_grp*DATA_LEN/2 + cbyte/2 >= NUM_VOLTAGES_EVEN_CHIP) break;
                    int16_t raw_val = (((uint16_t)adbms->ICs.cell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.cell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    if (ADBMS_getVoltage(raw_val) < 0.5)
                    {
                        int c_cell = (cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2;
                        adbms->openwire_fault_ = 1;
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
                    int16_t raw_val = (((uint16_t)adbms->ICs.cell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.cell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    if (ADBMS_getVoltage(raw_val) < 0.5)
                    {
                        int c_cell = (cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2;
                        adbms->openwire_fault_ = 1;
                    }
                }
            }
        }
    }

    /// OWC ODD Check
    adbms->adcv.cont = 1;
    adbms->adcv.ow = 2; // Enable OW on odd-channel 
    ADBMS_Set_ADCV(adbms->adcv, &adbms->ICs.adcv);
    ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adcv, adbms->ICs.cmd_buf);
    HAL_Delay(8);    // C-Channels are updated at 8ms

    // Get new s-channel voltages
    ADBMS_WakeUP_ICs();
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVA, (adbms->ICs.cell + 0 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf); //read voltages 0-2 for each chip
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVB, (adbms->ICs.cell + 1 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf); //read voltages 3-5 for each chip
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVC, (adbms->ICs.cell + 2 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf); //read voltages 6-8 for each chip
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVD, (adbms->ICs.cell + 3 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf); //read voltages 9-11 for each chip
    //pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVE, (adbms->ICs.cell + 4 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf, adbms->ICs.spi_tx_dataBuf); //DONT NEED VOLTAGES OVER 12 for nfr26

    if(pec){
        adbms->current_owc_failures += 1;
        if(adbms->current_owc_failures > PEC_FAILURE_THRESHOLD){
            adbms->pec_fault_ = 1;
        }
        return;
    }else adbms->current_owc_failures = 0;

    for (uint8_t cic = 0; cic < NUM_CHIPS; cic++)
    {
        if (cic % 2 == 0)
        {
            uint8_t num_reg_grps = NUM_VOLTAGES_EVEN_CHIP / VOLTAGES_REG_GRP + (NUM_VOLTAGES_EVEN_CHIP% VOLTAGES_REG_GRP != 0);
            for (uint8_t creg_grp = 0; creg_grp < num_reg_grps; creg_grp++)
            {
                for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)
                {
                    if(creg_grp*DATA_LEN/2 + cbyte/2 >= NUM_VOLTAGES_EVEN_CHIP) break;  
                    int16_t raw_val = (((uint16_t)adbms->ICs.cell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.cell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    if (ADBMS_getVoltage(raw_val) < 0.5)
                    {
                        int c_cell = (cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2;
                        adbms->openwire_fault_ = 1;
                    }
                }
            }
        }
        else
        {
            uint8_t num_reg_grps = NUM_VOLTAGES_ODD_CHIP / VOLTAGES_REG_GRP + (NUM_VOLTAGES_ODD_CHIP % VOLTAGES_REG_GRP != 0);
            for (uint8_t creg_grp = 0; creg_grp < num_reg_grps; creg_grp++)
            {
                for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)
                {
                    if(creg_grp*DATA_LEN/2 + cbyte/2 >= NUM_VOLTAGES_ODD_CHIP) break;
                    int16_t raw_val = (((uint16_t)adbms->ICs.cell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.cell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    if (ADBMS_getVoltage(raw_val) < 0.5)
                    {
                        int c_cell = (cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2;
                        adbms->openwire_fault_ = 1;
                    }
                }
            }
        }
    }

    /// Turn off owc
    adbms->adcv.cont = 1;
    adbms->adcv.ow = 0; // Enable OW on odd-channel 
    ADBMS_Set_ADCV(adbms->adcv, &adbms->ICs.adcv);
    ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adcv, adbms->ICs.cmd_buf);
    HAL_Delay(1);    // S-Channels are updated at 8ms
}


void ADBMS_Print_Vals(adbms_ *adbms)
{
    // print the total, max, min, and avg voltage
    printf("\nVOLTAGES\n");
    printf("total v: %f\n", adbms->total_v);
    printf("max v: %f\t", adbms->max_v);
    printf("min v: %f\t", adbms->min_v);
    printf("avg v: %f\t", adbms->avg_v);
    printf("max-min: %f\n", adbms->max_v - adbms->min_v);

    // print every voltage
    for (int i = 0; i < NUM_CHIPS; i++)
    {
        if (i % 2 == 0)
        {
            for (int j = 0; j < NUM_VOLTAGES_EVEN_CHIP; j++)
            {
                printf("C%d=%fV\t", ((i * NUM_VOLTAGES_ODD_CHIP + (i + 1)/2) + j + 1), adbms->voltages[(i * NUM_VOLTAGES_ODD_CHIP + (i + 1)/2) + j]);
            }
        }
        else
        {
            for (int j = 0; j < NUM_VOLTAGES_ODD_CHIP; j++)
            {
                printf("C%d=%fV\t", ((i * NUM_VOLTAGES_ODD_CHIP + (i + 1)/2) + j + 1), adbms->voltages[(i * NUM_VOLTAGES_ODD_CHIP + (i + 1)/2) + j]);
            }
        }
    }
    printf("\n");

    // print the total, max, min, and avg temp
    printf("\nTEMPS\n");
    printf("max temp: %f\t", adbms->max_temp);
    printf("min temp: %f\t", adbms->min_temp);
    printf("avg temp: %f\n", adbms->avg_temp);

    for (int i = 0; i < NUM_CHIPS; i++)
    {
        for (int j = 0; j < NUM_TEMPS_CHIP; j++)
        {
            printf("T%d=%f\t", (i * NUM_TEMPS_CHIP + j + 1), adbms->temperatures[i * NUM_TEMPS_CHIP + j]);
        }
    }
    printf("\n");

    printf("Faults\n");
    printf("undervoltage: %d\t", adbms->undervoltage_fault_);
    printf("overvoltage: %d\t", adbms->overvoltage_fault_);
    printf("pec: %d\t", adbms->pec_fault_);
    printf("overtemperature: %d\t", adbms->overtemperature_fault_);
    printf("openwire: %d\t", adbms->openwire_fault_);
    printf("openwire_temp: %d\n", adbms->openwire_temp_fault_);
}

void ADBMS_USB_Serial_Print_Vals(adbms_ *adbms) //TODO
{
    #define BUFFER_SIZE 3500  // Increase this if more snprintfs are added
    char logBuf[BUFFER_SIZE];
    int len = 0;
    int remaining = BUFFER_SIZE;

    
    len += snprintf(logBuf + len, remaining, "total_v: %f\t", adbms->total_v);
    remaining = BUFFER_SIZE - len;

    len += snprintf(logBuf + len, remaining, "max_v: %f\t", adbms->max_v);
    remaining = BUFFER_SIZE - len;

    len += snprintf(logBuf + len, remaining, "min_v: %f\t", adbms->min_v);
    remaining = BUFFER_SIZE - len;

    len += snprintf(logBuf + len, remaining, "avg_v: %f\t", adbms->avg_v);
    remaining = BUFFER_SIZE - len;

    len += snprintf(logBuf + len, remaining, "max-min: %f\r\n", adbms->max_v - adbms->min_v);
    remaining = BUFFER_SIZE - len;

    for (int i = 0; i < NUM_CHIPS; i++)
    {
        if (i % 2 == 0)
        {
            for (int j = 0; j < NUM_VOLTAGES_EVEN_CHIP; j++)
            {
                len += snprintf(logBuf + len, remaining, "C%d=%fV\t", 
                                (i * NUM_VOLTAGES_EVEN_CHIP + j + 1), 
                                adbms->voltages[i * NUM_VOLTAGES_EVEN_CHIP + j]);
                remaining = BUFFER_SIZE - len;
                if (remaining <= 0) break;
            }
        }
        else
        {
            for (int j = 0; j < NUM_VOLTAGES_ODD_CHIP; j++)
            {
                len += snprintf(logBuf + len, remaining, "C%d=%fV\t", 
                                (i * NUM_VOLTAGES_ODD_CHIP + j + 1), 
                                adbms->voltages[i * NUM_VOLTAGES_ODD_CHIP + j]);
                remaining = BUFFER_SIZE - len;
                if (remaining <= 0) break;
            }
        }
    }
    len += snprintf(logBuf + len, remaining, "\r\n");
    remaining = BUFFER_SIZE - len;

    len += snprintf(logBuf + len, remaining, "max_temp: %f\t", adbms->max_temp);
    remaining = BUFFER_SIZE - len;

    len += snprintf(logBuf + len, remaining, "min_temp: %f\t", adbms->min_temp);
    remaining = BUFFER_SIZE - len;

    len += snprintf(logBuf + len, remaining, "avg_temp: %f\r\n", adbms->avg_temp);
    remaining = BUFFER_SIZE - len;

    /*for (int i = 0; i < NUM_CHIPS; i++)
    {
        for (int j = 0; j < NUM_TEMPS_CHIP; j++)
        {
            len += snprintf(logBuf + len, remaining, "T%d=%f\t", 
                            (i * NUM_TEMPS_CHIP + j + 1),
                            adbms->temperatures[i * NUM_TEMPS_CHIP + j]);
            remaining = BUFFER_SIZE - len;
            if (remaining <= 0) break;
        }
    }
    len += snprintf(logBuf + len, remaining, "\r\n");
    remaining = BUFFER_SIZE - len;*/

    len += snprintf(logBuf + len, remaining, "undervoltage: %d\t", adbms->undervoltage_fault_);
    remaining = BUFFER_SIZE - len;

    len += snprintf(logBuf + len, remaining, "overvoltage: %d\t", adbms->overvoltage_fault_);
    remaining = BUFFER_SIZE - len;

    len += snprintf(logBuf + len, remaining, "pec: %d\t", adbms->pec_fault_);
    remaining = BUFFER_SIZE - len;

    len += snprintf(logBuf + len, remaining, "overtemperature: %d\t", adbms->overtemperature_fault_);
    remaining = BUFFER_SIZE - len;

    len += snprintf(logBuf + len, remaining, "openwire: %d\t", adbms->openwire_fault_);
    remaining = BUFFER_SIZE - len;

    len += snprintf(logBuf + len, remaining, "openwire_temp: %d\r\n", adbms->openwire_temp_fault_);
    remaining = BUFFER_SIZE - len;

//    for (int i = 0; i < NUM_CHIPS; i++)
//    {
//    	len += snprintf(logBuf + len, remaining, "dcc%d: %02x\t", i+1, ICs[i].tx_cfgb.dcc);
//    	remaining = BUFFER_SIZE - len;
//    }
//    len += snprintf(logBuf + len, remaining, "\r\n");
//	remaining = BUFFER_SIZE - len;

    if (remaining <= 0) HardFault_Handler();

    CDC_Transmit_FS((uint8_t*) logBuf, strlen(logBuf));
}