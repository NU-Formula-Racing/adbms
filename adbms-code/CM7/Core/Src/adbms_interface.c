#include "adbms_interface.h"

void ADBMS_Initialize(adbms_ *adbms, SPI_HandleTypeDef *hspi)
{
    adbms->ICs.hspi = hspi;
    // Set initial configurations
    for (uint8_t cic = 0; cic < (NUM_CHIPS-1); cic++)
    {
        // Init config A
        adbms->cfa[cic].refon = 1;
        adbms->cfa[cic].gpo = 0x3FF;  // all gpo tunred on

        // Init config B
        adbms->cfb[cic].vuv = Set_UnderOver_Voltage_Threshold(UNDERVOLTAGE);
        adbms->cfb[cic].vov = Set_UnderOver_Voltage_Threshold(OVERVOLTAGE);
    }

    //configurate 2950
    ADBMS_2950_config(adbms->ICs.cfg_a, &adbms->cfa2950);

    // Init sensing cmd
    adbms->adcv.cont = 1;
    //for 2950 measurements
    adbms->adsv.cont = 1;

    //2950 adv 
    adbms->adv.ow = 0; //open wire source off
    adbms->adv.vch = 0; //vch = 0 (this is complicated, for reference check datasheet table 57/58)

    // Package config and sensing structs into transmitable data
    ADBMS_Set_Config_A(adbms->cfa, adbms->ICs.cfg_a, (NUM_CHIPS));
    ADBMS_Set_Config_B(adbms->cfb, adbms->ICs.cfg_b);
    ADBMS_Set_ADCV(adbms->adcv, &adbms->ICs.adcv);
    ADBMS_Set_ADAX(adbms->adax, &adbms->ICs.adax);
    ADBMS_Set_ADSV(adbms->adsv, &adbms->ICs.adsv);
    // ADBMS_Set_ADV(adbms->adv, &adbms->ICs.adv); //2950 ADV command to turn on V1adc and V2adc


    // Write Config 
    ADBMS_WakeUP_ICs();
    ADBMS_WakeUP_ICs();

    //hardcode the 2950 refup bit to 1;
    //adbms->ICs.cfg_a[5] = 0x10;
    //adbms->ICs.cfg_a[5] |= 0x10; 

    //also need to toggle GPO1C to 1, GPO1OD to 0 to open the mosfet for vbat
    //adbms->ICs.cfg_a[3] |= 0x01;
    //adbms->ICs.cfg_a[4] &= 0xFE;

    ADBMS_Write_Data(adbms->ICs.hspi, WRCFGA, adbms->ICs.cfg_a, adbms->ICs.spi_dataBuf);

    //read configuration
    //ADBMS_WakeUP_ICs();
    //ADBMS_Read_Data(adbms->ICs.hspi,RDCFGA,adbms->ICs.cell,adbms->ICs.spi_dataBuf);
    ADBMS_WakeUP_ICs();
    ADBMS_Write_Data(adbms->ICs.hspi, WRCFGB, adbms->ICs.cfg_b, adbms->ICs.spi_dataBuf);

    // Turn on sensing
    ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adcv);
    HAL_Delay(1);
    ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adax);
    HAL_Delay(8); // ADCs are updated at their conversion rate of 1ms
    ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adsv);
    HAL_Delay(8);
    // ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adv); //new 2950 command to start V1adc and V2adc
    // HAL_Delay(8);
}

void ADBMS_2950_config(uint8_t* cfg_a, cfa2950_* cfa2950){
    //refup bit to 1
    cfa2950->refup = 1;

    //also need to toggle GPO1C to 1, GPO1OD to 0 to open the mosfet for vbat
    cfa2950->gpo1c = 1;
    cfa2950->gpo1od = 0;

    //set config
    ADBMS_Set_Config_A_2950(cfa2950, cfg_a, NUM_CHIPS);
}

void ADBMS_UpdateVoltages(adbms_ *adbms)
{
    // get voltages from ADBMS
    bool pec = 0;
    // ADBMS_WakeUP_ICs();

    //getting to the
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVA, (adbms->ICs.cell + 0 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf); //read voltages 0-2 for each chip
    // pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVB, (adbms->ICs.cell + 1 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf); //read voltages 3-5 for each chip
    // pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVC, (adbms->ICs.cell + 2 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf); //read voltages 6-8 for each chip
    // pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVD, (adbms->ICs.cell + 3 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf); //read voltages 9-11 for each chip
    //pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVE, (adbms->ICs.cell + 4 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf); //DONT NEED VOLTAGES OVER 12 for nfr26
    
    //this is reading v1adc and v2adc
    //THIS IS THE WRONG SENT data, the v1 v2 gets read with RDCVD
    //pec |= ADBMS_READ_DATA(adbms->ICs.hspi, RDCVD, (adbms->ICs.voltages), adbms->ICs.spi_dataBuf);

    //this is reading v_shunt_1 (v7) and v_shunt_2 (v9)
    //pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDAUXC, (adbms->ICs.shunt_temp), adbms->ICs.spi_dataBuf);
    
    adbms->voltage_pec_failure = pec;
    if (adbms->voltage_pec_failure){
        #define BUFFER_SIZE 64  // Increase this if more snprintfs are added
        char logBuf[BUFFER_SIZE];
        int len = 0;
        int remaining = BUFFER_SIZE;
        
        len += snprintf(logBuf + len, remaining, "\r\nPEC Fault\r\n");
        remaining = BUFFER_SIZE - len;

        if (remaining <= 0) HardFault_Handler();

        CDC_Transmit_FS((uint8_t*) logBuf, strlen(logBuf));
    }

    // calulate new values with the updated raw ones
    // ADBMS_CalculateValues_Voltages(adbms);

    // calculate 2950 values 
    ADBMS_2950_Calculate_Values(adbms);
}



//calculates values for 2950 from this
void ADBMS_2950_Calculate_Values(adbms_* adbms){
    
    // ADBMS2950_Calculate_Vbat(adbms);
    ADBMS2950_Calculate_Current(adbms);
    //ADBMS2950_Calculate_Post_Voltage(adbms);
    //ADBMS2950_Calculate_Shunt_Temp(adbms);
}

//calculate 2950 vbat
void ADBMS2950_Calculate_Vbat(adbms_* adbms){
   
    //initialize values 
    adbms->data_2950.vbat= 0.0;

    //offset because RDCVB is the second command sent
    int command_offset = NUM_CHIPS * DATA_LEN;
    //offset because 2950 is the last chip on the daisy chain
    int offset = (NUM_CHIPS-1) * DATA_LEN;

    //getting raw data from cell readings
    int16_t vbat1_raw = ((int16_t)(adbms->ICs.cell[3 + command_offset + offset]) << 8) | (int16_t)(adbms->ICs.cell[2 + command_offset + offset]);
    int16_t vbat2_raw = ((int16_t)(adbms->ICs.cell[5 + command_offset + offset]) << 8) | (int16_t)(adbms->ICs.cell[4 + command_offset + offset]);

    adbms->data_2950.vbat = ADBMS_2950_Transfer_Vbat(vbat1_raw,vbat2_raw);
    adbms->precontactor_voltage = adbms->data_2950.vbat;
}

float ADBMS_2950_Transfer_Vbat(int16_t vbat1_raw, int16_t vbat2_raw){

    float vbat_final = 0.0;

    //transfer to real value and store
    float vbat1 = (float)(vbat1_raw / 0.0041938); // (15000/3,600,000 + 15000)
    float vbat2 = (float) vbat2_raw;

    vbat_final = (vbat1-vbat2) * 0.0001; //100 microolms
    
    return vbat_final;
}

//calculate 2950 current
void ADBMS2950_Calculate_Current(adbms_* adbms){
    
    //initialize values 
    adbms->data_2950.i1 = 0.0;
    adbms->data_2950.i2 = 0.0;

    //offset because 2950 is the last chip on the daisy chain
    int offset =  (NUM_CHIPS-1) * DATA_LEN;

    //getting raw data from cell readings
    int32_t i1_raw = ((int32_t)(adbms->ICs.cell[2 + offset]) << 16) | ((int32_t)(adbms->ICs.cell[1 + offset]) << 8) | adbms->ICs.cell[0 + offset];
    int32_t i2_raw = ((int32_t)(adbms->ICs.cell[5 + offset]) << 16) | ((int32_t)(adbms->ICs.cell[4 + offset]) << 8) | adbms->ICs.cell[3 + offset];

    //sign extend because it is a 24 bit number but stored int32
    if (i1_raw & 0x00800000) {     
        i1_raw |= 0xFF000000;      
    }

    if(i2_raw & 0x00800000){
        i2_raw |= 0xFF000000;
    }

    //transfer to real value and store
    //i1 is negative by default


    adbms->data_2950.i1 = -ADBMS2950_Transfer_Current(i1_raw);
    adbms->data_2950.i2 = ADBMS2950_Transfer_Current(i2_raw);

    adbms->current = adbms->data_2950.i1;
    // printf("Current: %f\n", adbms->current);
    //i2 doesn't work yet, technically we should take the average of i1 and i2
    //adbms->current = (adbms->data_2950.i1 + adbms->data_2950.i2) / 2;
}

float ADBMS2950_Transfer_Current(int32_t data)
{
  float current;
  //the actual measured resistance is closer to 94 microolms instead of 100
  current = (float) data / 94.0;
  return current;
}

float ADBMS2950_Calculate_Post_Voltage(adbms_ *adbms){

    //initialize values
    adbms->data_2950.v_TS = 0.0;

    //RDCVD is the 4th command that we send that get puts into cell
    int command_offset = (DATA_LEN * NUM_CHIPS) * 3;
    //offset because 2950 is the lsat chip on the daisy chain
    int offset = (NUM_CHIPS-1) * DATA_LEN;

    //getting raw data from cell readings
    int16_t v1_raw = (int16_t)(adbms->ICs.cell[3+offset+command_offset] << 8) | (int16_t)(adbms->ICs.cell[2+offset+command_offset]);
    int16_t v2_raw = (int16_t)(adbms->ICs.cell[5+offset+command_offset] << 8) | (int16_t)(adbms->ICs.cell[1+offset+command_offset]);

    adbms->data_2950.v_TS = ADBMS_Calculate_Post_Voltage(v1_raw, v2_raw);
}

float ADBMS_Calculate_Post_Voltage(int16_t v1_raw, int16_t v2_raw){

    float v_TS = 0.0;

    float v1 = (float) (v1_raw / 0.0041938); // (15000/3,600,000 + 15000)
    float v2 = (float) (v2_raw);

    v_TS = (v1-v2) * 0.0001; //100 microolms
    
    return v_TS;
}


float ADBMS2950_Calculate_Shunt_Temp(adbms_ *adbms){

    //initilaize values
    adbms->data_2950.v_shunt_1 = 0.0;
    adbms->data_2950.v_shunt_2 = 0.0;

    //offset because 2950 is the last chip ont he daisy chain
    int offset = (NUM_CHIPS-1) * DATA_LEN;

    //getting raw data 
    int16_t v7 = (int16_t)(adbms->ICs.shunt_temp[1+offset] << 8) | (int16_t)(adbms->ICs.shunt_temp[0+offset]);
    int16_t v9 = (int16_t)(adbms->ICs.shunt_temp[5+offset] << 8) | (int16_t)(adbms->ICs.shunt_temp[4+offset]);

    adbms->data_2950.v_shunt_1 = ADBMS2950_Transfer_Shunt_Temp(v7);
    adbms->data_2950.v_shunt_2 = ADBMS2950_Transfer_Shunt_Temp(v9);
}

float ADBMS2950_Transfer_Shunt_Temp(int16_t voltage){

    //use vref = 1.25V


}



  
void ADBMS_UpdateTemps(adbms_ *adbms)
{
    // get temps from ADBMS
    bool pec = 0;
    ADBMS_WakeUP_ICs();
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDAUXA, (adbms->ICs.aux + 0 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDAUXB, (adbms->ICs.aux + 1 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDAUXC, (adbms->ICs.aux + 2 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDAUXD, (adbms->ICs.aux + 3 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf);
    adbms->temp_pec_failure = pec;

    // need to start new poll for conversion before next read (no continous mode)
    ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adax);

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
    for (uint8_t cic = 0; cic < (NUM_CHIPS-1); cic++)
    {
        if (cic % 2 == 0) //even chip, 12 voltages
        {
            uint8_t num_reg_grps = NUM_VOLTAGES_EVEN_CHIP / VOLTAGES_REG_GRP + (NUM_VOLTAGES_EVEN_CHIP % VOLTAGES_REG_GRP != 0);//find how many register groups used to store voltages
            for (uint8_t creg_grp = 0; creg_grp < num_reg_grps; creg_grp++) //index register groups, each contain 3 registers that represent a voltage
            {
                for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)//each reg_grp is 2 bytes (each reg is 1 byte) INDEX REGISTER THEN BYTES?
                {
                    if(creg_grp*DATA_LEN/2 + cbyte/2 >= NUM_VOLTAGES_EVEN_CHIP) break;  //stop processing registers when desired voltage reading count is reached
                    int16_t raw_val = (((uint16_t)adbms->ICs.cell[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.cell[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte];
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

                    int16_t raw_val = (((uint16_t)adbms->ICs.cell[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.cell[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte];
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
    
    //adbms->total_v = even_total + odd_total;
    adbms->total_v = 60.0;
    // calculate the avg voltage
    if(NUM_CHIPS > 1){
        adbms->avg_v = adbms->total_v / ((NUM_CHIPS-1) * NUM_VOLTAGES_ODD_CHIP + (((NUM_CHIPS))/2));
    }
    else{
        adbms->avg_v = 0.0;
    }
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
    for (int cic = 0; cic < (NUM_CHIPS-1); cic++)
    {
        for (uint8_t creg_grp = 0; creg_grp < AUX_REG_GRP; creg_grp++)
        {
            for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)
            {
            	if(creg_grp*DATA_LEN/2 + cbyte/2 >= AUX_GPIO) continue;	// only 10 gpio's

                int16_t raw_val = (((uint16_t)adbms->ICs.aux[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.aux[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte];
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

    if (NUM_CHIPS > 1){
        adbms->avg_temp = total_temp / ((NUM_CHIPS-1) * NUM_TEMPS_CHIP);
    }
    else{
        adbms->avg_temp = 0.0;
    }
    
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
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_SET);

    for (int cic = 0; cic < (NUM_CHIPS-1); cic++)
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
            // adbms->cfb[cic].dcc = dcc;
            adbms->cfb[cic].dcc = 1 << 0;
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
            // adbms->cfb[cic].dcc = dcc;
            adbms->cfb[cic].dcc = 1 << 1;
        }
    }
    ADBMS_Set_Config_B(adbms->cfb, adbms->ICs.cfg_b);
    ADBMS_Write_Data(adbms->ICs.hspi, WRCFGB, adbms->ICs.cfg_b, adbms->ICs.spi_dataBuf);
}

void cellBalanceOff(adbms_ *adbms)
{
    // Turn off CB indication LED
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_RESET);

    for (int cic = 0; cic < (NUM_CHIPS-1); cic++)
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
    ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adsv);
    HAL_Delay(8);    // S-Channels are updated at 8ms

    // Get new s-channel voltages
    bool pec = 0;
    ADBMS_WakeUP_ICs();
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVA, (adbms->ICs.scell + 0 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVB, (adbms->ICs.scell + 1 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVC, (adbms->ICs.scell + 2 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVD, (adbms->ICs.scell + 3 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVE, (adbms->ICs.scell + 4 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf); // probably don't need this

    if(pec){
        adbms->current_owc_failures += 1;
        if(adbms->current_owc_failures > PEC_FAILURE_THRESHOLD){
            adbms->pec_fault_ = 1;
        }
        return;
    }else adbms->current_owc_failures = 0;

    for (uint8_t cic = 0; cic < (NUM_CHIPS-1); cic++)
    {
        if( cic % 2 == 0) //even chip
        {
            uint8_t num_reg_grps = NUM_VOLTAGES_EVEN_CHIP / VOLTAGES_REG_GRP + (NUM_VOLTAGES_EVEN_CHIP % VOLTAGES_REG_GRP != 0);//find how many register groups used to store voltages
            for (uint8_t creg_grp = 0; creg_grp < num_reg_grps; creg_grp++)
            {
                for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)
                {
                    if(creg_grp*DATA_LEN/2 + cbyte/2 >= NUM_VOLTAGES_EVEN_CHIP) break;
                    int16_t raw_val = (((uint16_t)adbms->ICs.scell[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.scell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    if (ADBMS_getVoltage(raw_val) < 0.5)
                    {
                        int c_cell = (cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2;
                        adbms->openwire_fault_ = 1;
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
                    int16_t raw_val = (((uint16_t)adbms->ICs.scell[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.scell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    if (ADBMS_getVoltage(raw_val) < 0.5)
                    {
                        int c_cell = (cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2;
                        adbms->openwire_fault_ = 1;
                        return;
                    }
                }
            }
        }
    }

    /// OWC ODD Check
    adbms->adsv.cont = 1;
    adbms->adsv.ow = 2; // Enable OW on odd-channel 
    ADBMS_Set_ADSV(adbms->adsv, &adbms->ICs.adsv);
    ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adsv);
    HAL_Delay(8);    // S-Channels are updated at 8ms

    // Get new s-channel voltages
    ADBMS_WakeUP_ICs();
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVA, (adbms->ICs.scell + 0 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVB, (adbms->ICs.scell + 1 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVC, (adbms->ICs.scell + 2 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVD, (adbms->ICs.scell + 3 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDSVE, (adbms->ICs.scell + 4 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf); // probably don't need this

    if(pec){
        adbms->current_owc_failures += 1;
        if(adbms->current_owc_failures > PEC_FAILURE_THRESHOLD){
            adbms->pec_fault_ = 1;
        }
        return;
    }else adbms->current_owc_failures = 0;

    for (uint8_t cic = 0; cic < (NUM_CHIPS-1); cic++)
    {
        if (cic % 2 == 0)
        {
            uint8_t num_reg_grps = NUM_VOLTAGES_EVEN_CHIP / VOLTAGES_REG_GRP + (NUM_VOLTAGES_EVEN_CHIP% VOLTAGES_REG_GRP != 0);
            for (uint8_t creg_grp = 0; creg_grp < num_reg_grps; creg_grp++)
            {
                for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)
                {
                    if(creg_grp*DATA_LEN/2 + cbyte/2 >= NUM_VOLTAGES_EVEN_CHIP) break;  
                    int16_t raw_val = (((uint16_t)adbms->ICs.scell[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.scell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    if (ADBMS_getVoltage(raw_val) < 0.5)
                    {
                        int c_cell = (cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2;
                        adbms->openwire_fault_ = 1;
                        return;
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
                    int16_t raw_val = (((uint16_t)adbms->ICs.scell[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.scell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    if (ADBMS_getVoltage(raw_val) < 0.5)
                    {
                        int c_cell = (cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2;
                        adbms->openwire_fault_ = 1;
                        return;
                    }
                }
            }
        }
    }

    /// Turn off owc
    adbms->adsv.cont = 1;
    adbms->adsv.ow = 0; // Enable OW on odd-channel 
    ADBMS_Set_ADSV(adbms->adsv, &adbms->ICs.adsv);
    ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adsv);
    HAL_Delay(1);    // S-Channels are updated at 8ms
}


void Update_Owc_C_Channel_Fault(adbms_ *adbms)
{
    // check openwire fault
    ADBMS_WakeUP_ICs();

    adbms->adcv.cont = 1;
    adbms->adcv.ow = 1; // Enable OW on even-channel 
    ADBMS_Set_ADCV(adbms->adcv, &adbms->ICs.adcv);
    ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adcv);
    HAL_Delay(8);    // C-Channels are updated at 1ms

    // Get new C-channel voltages
    bool pec = 0;
    ADBMS_WakeUP_ICs();
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVA, (adbms->ICs.cell + 0 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf); //read voltages 0-2 for each chip
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVB, (adbms->ICs.cell + 1 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf); //read voltages 3-5 for each chip
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVC, (adbms->ICs.cell + 2 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf); //read voltages 6-8 for each chip
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVD, (adbms->ICs.cell + 3 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf); //read voltages 9-11 for each chip
    //pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVE, (adbms->ICs.cell + 4 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf); //DONT NEED VOLTAGES OVER 12 for nfr26

    if(pec){
        adbms->current_owc_failures += 1;
        if(adbms->current_owc_failures > PEC_FAILURE_THRESHOLD){
            adbms->pec_fault_ = 1;
        }
        return;
    }else adbms->current_owc_failures = 0;

    for (uint8_t cic = 0; cic < (NUM_CHIPS-1); cic++)
    {
        if( cic % 2 == 0) //even chip
        {
            uint8_t num_reg_grps = NUM_VOLTAGES_EVEN_CHIP / VOLTAGES_REG_GRP + (NUM_VOLTAGES_EVEN_CHIP % VOLTAGES_REG_GRP != 0);//find how many register groups used to store voltages
            for (uint8_t creg_grp = 0; creg_grp < num_reg_grps; creg_grp++)
            {
                for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)
                {
                    if(creg_grp*DATA_LEN/2 + cbyte/2 >= NUM_VOLTAGES_EVEN_CHIP) break;
                    int16_t raw_val = (((uint16_t)adbms->ICs.cell[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.cell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    if (ADBMS_getVoltage(raw_val) < 0.5)
                    {
                        int c_cell = (cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2;
                        adbms->openwire_fault_ = 1;
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
                    int16_t raw_val = (((uint16_t)adbms->ICs.cell[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.cell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    if (ADBMS_getVoltage(raw_val) < 0.5)
                    {
                        int c_cell = (cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2;
                        adbms->openwire_fault_ = 1;
                        return;
                    }
                }
            }
        }
    }

    /// OWC ODD Check
    adbms->adcv.cont = 1;
    adbms->adcv.ow = 2; // Enable OW on odd-channel 
    ADBMS_Set_ADCV(adbms->adcv, &adbms->ICs.adcv);
    ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adcv);
    HAL_Delay(8);    // C-Channels are updated at 8ms

    // Get new s-channel voltages
    ADBMS_WakeUP_ICs();
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVA, (adbms->ICs.cell + 0 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf); //read voltages 0-2 for each chip
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVB, (adbms->ICs.cell + 1 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf); //read voltages 3-5 for each chip
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVC, (adbms->ICs.cell + 2 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf); //read voltages 6-8 for each chip
    pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVD, (adbms->ICs.cell + 3 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf); //read voltages 9-11 for each chip
    //pec |= ADBMS_Read_Data(adbms->ICs.hspi, RDCVE, (adbms->ICs.cell + 4 * NUM_CHIPS * DATA_LEN), adbms->ICs.spi_dataBuf); //DONT NEED VOLTAGES OVER 12 for nfr26

    if(pec){
        adbms->current_owc_failures += 1;
        if(adbms->current_owc_failures > PEC_FAILURE_THRESHOLD){
            adbms->pec_fault_ = 1;
        }
        return;
    }else adbms->current_owc_failures = 0;

    for (uint8_t cic = 0; cic < (NUM_CHIPS-1); cic++)
    {
        if (cic % 2 == 0)
        {
            uint8_t num_reg_grps = NUM_VOLTAGES_EVEN_CHIP / VOLTAGES_REG_GRP + (NUM_VOLTAGES_EVEN_CHIP% VOLTAGES_REG_GRP != 0);
            for (uint8_t creg_grp = 0; creg_grp < num_reg_grps; creg_grp++)
            {
                for (uint8_t cbyte = 0; cbyte < DATA_LEN; cbyte+=2)
                {
                    if(creg_grp*DATA_LEN/2 + cbyte/2 >= NUM_VOLTAGES_EVEN_CHIP) break;  
                    int16_t raw_val = (((uint16_t)adbms->ICs.cell[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.cell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    if (ADBMS_getVoltage(raw_val) < 0.5)
                    {
                        int c_cell = (cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2;
                        adbms->openwire_fault_ = 1;
                        return;
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
                    int16_t raw_val = (((uint16_t)adbms->ICs.cell[creg_grp * (NUM_CHIPS) * DATA_LEN + cic * DATA_LEN + cbyte + 1]) << 8) | adbms->ICs.cell[creg_grp * NUM_CHIPS * DATA_LEN + cic * DATA_LEN + cbyte];
                    if (ADBMS_getVoltage(raw_val) < 0.5)
                    {
                        int c_cell = (cic * NUM_VOLTAGES_ODD_CHIP + (cic + 1)/2) + creg_grp*DATA_LEN/2 + cbyte/2;
                        adbms->openwire_fault_ = 1;
                        return;
                    }
                }
            }
        }
    }

    /// Turn off owc
    adbms->adcv.cont = 1;
    adbms->adcv.ow = 0; // Enable OW on odd-channel 
    ADBMS_Set_ADCV(adbms->adcv, &adbms->ICs.adcv);
    ADBMS_Write_CMD(adbms->ICs.hspi, adbms->ICs.adcv);
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

    // 2950 prints
    printf("adbms2950 vbat: %f\t", adbms->data_2950.vbat);
    printf("adbms2950 i1: %f\t", adbms->data_2950.i1);
    printf("adbms2950 i2: %f\t", adbms->data_2950.i2);
    printf("adbms2950 TS: %f\n", adbms->data_2950.v_TS);

    // print every voltage
    for (int i = 0; i < (NUM_CHIPS-1); i++)
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

    for (int i = 0; i < (NUM_CHIPS-1); i++)
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

    //2950 prints
    len += snprintf(logBuf + len, remaining, "adbms2950 vbat: %f\t", adbms->data_2950.vbat);
    remaining = BUFFER_SIZE - len;

    len += snprintf(logBuf + len, remaining, "adbms2950 i1: %f\t", adbms->data_2950.i1);
    remaining = BUFFER_SIZE - len;

    len += snprintf(logBuf + len, remaining, "adbms2950 i2: %f\t\n", adbms->data_2950.i2);
    remaining = BUFFER_SIZE - len;

    for (int i = 0; i < (NUM_CHIPS-1); i++)
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
