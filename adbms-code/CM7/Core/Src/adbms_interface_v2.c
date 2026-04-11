#include "adbms_interface_v2.h"

void ADBMS_Initialize(adbms_raw_ *adbms, SPI_HandleTypeDef *hspi)
{
    adbms->SPI_data.hspi = hspi;

    //configuration parameter place
    ADBMS_6830_Config(adbms->command_parameters->parameter_6830,&adbms->command_bit);
    ADBMS_2950_Config(adbms->command_parameters->parameter_2950,&adbms->command_bit);
    ADBMS_joint_Config(adbms->command_parameters->parameter_joint,&adbms->command_bit);

    //wakeup IC
    ADBMS_WakeUP_ICs();
    ADBMS_WakeUP_ICs();

    //write data and commands
    ADBMS_Write_Data_Command(adbms);
}

void ADBMS_Read_Voltage(adbms_raw_ *adbms){

    //get voltages from ADBMS
    bool pec = 0;
    ADBMS_WakeUP_ICs();

    //reading into raw values struct
    pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVA, (adbms->raw_value.cell + 0 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf); //read voltages 0-2 for each chip //read current for 2950 chip
    pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVB, (adbms->raw_value.cell + 1 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf); //read voltages 3-5 for each chip
    pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVC, (adbms->raw_value.cell + 2 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf); //read voltages 6-8 for each chip
    pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVD, (adbms->raw_value.cell + 3 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf); //read voltages 9-11 for each chip
    //pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVE, (adbms->raw_value.cell + 4 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf); //DONT NEED VOLTAGES OVER 12 for nfr26

    adbms->read_failure.read_voltage_pec_failure = pec;
}

void ADBMS_Read_Temps(adbms_raw_* adbms){

    //get temps from ADBMS
    bool pec = 0;
    ADBMS_WakeUP_ICs();

    pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDAUXA, (adbms->raw_value.aux + 0 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDAUXB, (adbms->raw_value.aux + 1 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDAUXC, (adbms->raw_value.aux + 2 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDAUXD, (adbms->raw_value.aux + 3 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);

    adbms->read_failure.read_temp_pec_failure = pec;

    //need to start new poll for conversion before next read (no continuous mode)
    ADBMS_Write_CMD(adbms->SPI_data.hspi,adbms->command_bit.adax);
}

//
// Chip Configurations
//
void ADBMS_6830_Config(command_parameters_6830_* parameters,config_command_bits_* command_bits){

    // Set initial configurations
    for (uint8_t cic = 0; cic < (NUM_6830-1); cic++)
    {
        // Init config A
        parameters->cfa6830[cic].refon = 1;
        parameters->cfa6830[cic].gpo = 0x3FF;  // all gpo tunred on

        // Init config B
        parameters->cfb6830[cic].vuv = Set_UnderOver_Voltage_Threshold(UNDERVOLTAGE);
        parameters->cfb6830[cic].vov = Set_UnderOver_Voltage_Threshold(OVERVOLTAGE);
    }

    ADBMS_Set_Config_A_6830(&parameters->cfa6830,command_bits->cfg_a, NUM_6830); //sets for all 6830
    ADBMS_Set_Config_B_6830(&parameters->cfb6830,command_bits->cfg_b, NUM_6830); // sets for all 6830
    
}

void ADBMS_2950_Config(command_parameters_2950_* parameters, config_command_bits_* command_bits){

    //
    //cfa configs
    //
    parameters->cfa2950.refup = 1; //refup to 1
    parameters->cfa2950.gpo1c = 1; //for opening the mosfet for vbat
    parameters->cfa2950.gpo1od = 0; //for opening the mosfet for vbat

    ADBMS_Set_Config_A_2950(&parameters->cfa2950,command_bits->cfg_a, NUM_CHIPS); //2950 is the last chip on the daisychain

    //
    //no cfb configs for now
    //
    ADBMS_Set_Config_B_2950(&parameters->cfb2950,command_bits->cfg_b, NUM_CHIPS); //2950 is the last chip on the daisychain

    
}

void ADBMS_joint_Config(command_parameters_joint_* parameters, config_command_bits_* command_bits){

    //
    //adcv configs
    //
    parameters->adcv.cont = 1; //Init sensing cmd

    ADBMS_Set_ADCV(parameters->adcv, command_bits->adcv); // set adcv


    //
    //adsv configs
    //
    parameters->adsv.cont = 1; // for 2950 measurements

    ADBMS_Set_ADSV(parameters->adsv,command_bits->adsv); //set adsv

    //
    //adv configs
    //
    parameters->adv.ow = 0;  //open_wire source off
    parameters->adv.vch = 0; //vch = 0 (this is complicated, for reference check datasheet table 57/58)

    ADBMS_Set_ADV(parameters->adv,command_bits->adv); //set adv
}

//
//moved all previous write data and command in initialize over
//
void ADBMS_Initialize_Write_Data_Command(adbms_raw_* adbms){
    
    //Write data and commands
    ADBMS_Write_Data(adbms->SPI_data.hspi, WRCFGA, adbms->command_bit.cfg_a, adbms->SPI_data.spi_dataBuf);
    ADBMS_Write_Data(adbms->SPI_data.hspi, WRCFGB, adbms->command_bit.cfg_b, adbms->SPI_data.spi_dataBuf);

    // Turn on sensing
    ADBMS_Write_CMD(adbms->SPI_data.hspi, adbms->command_bit.adcv);
    HAL_Delay(1); // ADCs are updated at their conversion rate of 1ms
    ADBMS_Write_CMD(adbms->SPI_data.hspi, adbms->command_bit.adax);
    HAL_Delay(8); 
    ADBMS_Write_CMD(adbms->SPI_data.hspi, adbms->command_bit.adsv);
    HAL_Delay(8);
    ADBMS_Write_CMD(adbms->SPI_data.hspi, adbms->command_bit.adv); //new 2950 command to start V1adc and V2adc
    HAL_Delay(8);
}


//
// these are all configuration or reads for open-wire checks
//

void cell_Balance_On(adbms_raw_* adbms)
{
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

    ADBMS_Set_Config_B_6830(&parameters.cfb6830,&command_bits.cfg_b, NUM_6830);
    ADBMS_Write_Data(adbms->SPI_data.hspi, WRCFGB, adbms->command_bit.cfg_b, adbms->SPI_data.spi_dataBuf);
    
}

void Owc_C_Channel_Off(adbms_raw_* adbms)
{
    ADBMS_WakeUP_ICs();

    adbms->command_parameters.parameter_joint.adcv.cont = 1;
    adbms->command_parameters.parameter_joint.adcv.ow = 0; //Disable OW

    ADBMS_Set_ADCV(adbms->command_parameters.parameter_joint.adcv, &adbms->command_bit.adcv);
    ADBMS_Write_CMD(adbms->SPI_data.hspi,adbms->command_bit.adcv);

    HAL_Delay(1);
}

void Owc_C_Channel_Even_On(adbms_raw_* adbms)
{
    ADBMS_WakeUP_ICs();

    adbms->command_parameters.parameter_joint.adcv.cont = 1;
    adbms->command_parameters.parameter_joint.adcv.ow = 1; //Enable Even Channel OW

    ADBMS_Set_ADCV(adbms->command_parameters.parameter_joint.adcv, &adbms->command_bit.adcv);
    ADBMS_Write_CMD(adbms->SPI_data.hspi,adbms->command_bit.adcv);
    HAL_Delay(8); 
}

void Owc_C_Channel_Odd_On(adbms_raw_* adbms)
{
    ADBMS_WakeUP_ICs();

    adbms->command_parameters.parameter_joint.adcv.cont = 1;
    adbms->command_parameters.parameter_joint.adcv.ow = 2; //Enable Odd Channel OW

    ADBMS_Set_ADCV(adbms->command_parameters.parameter_joint.adcv, &adbms->command_bit.adcv);
    ADBMS_Write_CMD(adbms->SPI_data.hspi,adbms->command_bit.adcv);
    HAL_Delay(8); 
}

void Owc_C_Channel_Read(adbms_raw_* adbms)
{
    bool pec = 0;
    ADBMS_WakeUP_ICs();

    pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVA, (adbms->raw_value.scell + 0 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVB, (adbms->raw_value.scell + 1 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVC, (adbms->raw_value.scell + 2 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVD, (adbms->raw_value.scell + 3 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);
    //pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVE, (adbms->raw_value.scell + 4 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf); // probably don't need this

    adbms->read_failure.read_open_wire_pec_failure = pec;

}


void Owc_S_Channel_Off(adbms_raw_* adbms)
{
    ADBMS_WakeUP_ICs();

    // not quite sure if we should turn it back on right now, or if i just turn it on in BMS
    // cell_Balance_Off(adbms); 

    adbms->command_parameters.parameter_joint.adsv.cont = 1;
    adbms->command_parameters.parameter_joint.adsv.ow = 0; //Disable OW 

    ADBMS_Set_ADSV(adbms->command_parameters.parameter_joint.adsv, &adbms->command_bit.adsv);
    ADBMS_Write_CMD(adbms->SPI_data.hspi,adbms->command_bit.adsv);

    HAL_Delay(1);
}

void Owc_S_Channel_Even_On(adbms_raw_* adbms)
{
    // check openwire fault
    ADBMS_WakeUP_ICs();
    cell_Balance_Off(adbms);  // need to turn off cell balancing to check for OWC

    adbms->command_parameters.parameter_joint.adsv.cont = 1;
    adbms->command_parameters.parameter_joint.adsv.ow = 1; // Enable OW on even-channel 

    ADBMS_Set_ADSV(adbms->command_parameters.parameter_joint.adsv, &adbms->command_bit.adsv);
    ADBMS_Write_CMD(adbms->SPI_data.hspi,adbms->command_bit.adsv);

    HAL_Delay(8);
}

void Owc_S_Channel_Odd_On(adbms_raw_* adbms)
{
    // check openwire fault
    ADBMS_WakeUP_ICs();
    cell_Balance_Off(adbms);  // need to turn off cell balancing to check for OWC

    /// OWC ODD Check
    adbms->command_parameters.parameter_joint.adsv.cont = 1;
    adbms->command_parameters.parameter_joint.adsv.ow = 2; // Enable OW on odd-channel 

    ADBMS_Set_ADSV(adbms->command_parameters.parameter_joint.adsv, &adbms->command_bit.adsv);
    ADBMS_Write_CMD(adbms->SPI_data.hspi,adbms->command_bit.adsv);

    HAL_Delay(8);
}

void Owc_S_Channel_Read(adbms_raw_* adbms)
{
    bool pec = 0;
    ADBMS_WakeUP_ICs();

    pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDSVA, (adbms->raw_value.scell + 0 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDSVB, (adbms->raw_value.scell + 1 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDSVC, (adbms->raw_value.scell + 2 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);
    pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDSVD, (adbms->raw_value.scell + 3 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);
    //pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDSVE, (adbms->raw_value.scell + 4 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf); // probably don't need this

    adbms->read_failure.read_open_wire_pec_failure = pec;

}


