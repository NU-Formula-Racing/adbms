#include "adbms_interface.h"


void ADBMS_Initialize(adbms_raw_* adbms_raw, SPI_HandleTypeDef *hspi)
{
    adbms_raw->SPI_data.hspi = hspi;

    //configuration parameter place
    ADBMS_6830_Config(&adbms_raw->command_parameters.parameter_6830,&adbms_raw->command_bit);
    ADBMS_2950_Config(&adbms_raw->command_parameters.parameter_2950,&adbms_raw->command_bit);
    ADBMS_joint_Config(&adbms_raw->command_parameters.parameter_joint,&adbms_raw->command_bit);

    //wakeup IC
    ADBMS_WakeUP_ICs();
    ADBMS_WakeUP_ICs();

    //write data and commands
    ADBMS_Write_Data(adbms_raw->SPI_data.hspi, WRCFGA, adbms_raw->command_bit.cfg_a, adbms_raw->SPI_data.spi_dataBuf);
    ADBMS_Write_Data(adbms_raw->SPI_data.hspi, WRCFGB, adbms_raw->command_bit.cfg_b, adbms_raw->SPI_data.spi_dataBuf);

    // Turn on sensing
    ADBMS_Write_CMD(adbms_raw->SPI_data.hspi, adbms_raw->command_bit.adcv);
    HAL_Delay(1); // ADCs are updated at their conversion rate of 1ms
    ADBMS_Write_CMD(adbms_raw->SPI_data.hspi, adbms_raw->command_bit.adax);
    HAL_Delay(1); 
    ADBMS_Write_CMD(adbms_raw->SPI_data.hspi, adbms_raw->command_bit.adsv);
    HAL_Delay(1);
    ADBMS_Write_CMD(adbms_raw->SPI_data.hspi, adbms_raw->command_bit.adv); //new 2950 command to start V1adc and V2adc
    HAL_Delay(8);
}

void ADBMS_Read_Voltages(ADBMS_read_raw_* read_raw, voltage_read_type_ type, SPI_HandleTypeDef *hspi,uint8_t *spi_dataBuf)
{

    bool pec = 0;

    ADBMS_WakeUP_ICs();

    switch (type)
    {
        case C_Channel_Read:
            pec |= ADBMS_Read_Data(hspi, RDCVA, (read_raw->read_return + 0 * NUM_CHIPS * DATA_LEN), spi_dataBuf); //read voltages 0-2 for each chip //read current for 2950 chip
            pec |= ADBMS_Read_Data(hspi, RDCVB, (read_raw->read_return + 1 * NUM_CHIPS * DATA_LEN), spi_dataBuf); //read voltages 3-5 for each chip
            pec |= ADBMS_Read_Data(hspi, RDCVC, (read_raw->read_return + 2 * NUM_CHIPS * DATA_LEN), spi_dataBuf); //read voltages 6-8 for each chip
            pec |= ADBMS_Read_Data(hspi, RDCVD, (read_raw->read_return + 3 * NUM_CHIPS * DATA_LEN), spi_dataBuf); //read voltages 9-11 for each chip
            break;

        case S_Channel_Read:
            pec |= ADBMS_Read_Data(hspi, RDSVA, (read_raw->read_return + 0 * NUM_CHIPS * DATA_LEN), spi_dataBuf);
            pec |= ADBMS_Read_Data(hspi, RDSVB, (read_raw->read_return + 1 * NUM_CHIPS * DATA_LEN), spi_dataBuf);
            pec |= ADBMS_Read_Data(hspi, RDSVC, (read_raw->read_return + 2 * NUM_CHIPS * DATA_LEN), spi_dataBuf);
            pec |= ADBMS_Read_Data(hspi, RDSVD, (read_raw->read_return + 3 * NUM_CHIPS * DATA_LEN), spi_dataBuf);
            break;
        
        case AUX_Read: 
            pec |= ADBMS_Read_Data(hspi, RDAUXA, (read_raw->read_return + 0 * NUM_CHIPS * DATA_LEN), spi_dataBuf);
            pec |= ADBMS_Read_Data(hspi, RDAUXB, (read_raw->read_return + 1 * NUM_CHIPS * DATA_LEN), spi_dataBuf);
            pec |= ADBMS_Read_Data(hspi, RDAUXC, (read_raw->read_return + 2 * NUM_CHIPS * DATA_LEN), spi_dataBuf);
            pec |= ADBMS_Read_Data(hspi, RDAUXD, (read_raw->read_return + 3 * NUM_CHIPS * DATA_LEN), spi_dataBuf);
            
    }

    read_raw->read_pec_failure = pec;

}

//
// Chip Configurations
//
void ADBMS_6830_Config(command_parameters_6830_* parameters,config_command_bits_* command_bits){

    // Set initial configurations
    for (uint8_t cic = 0; cic < (NUM_6830); cic++)
    {
        // Init config A
        parameters->cfa6830[cic].refon = 1;
        parameters->cfa6830[cic].gpo = 0x3FF;  // all gpo tunred on

        // Init config B
        parameters->cfb6830[cic].vuv = Set_UnderOver_Voltage_Threshold(UNDERVOLTAGE);
        parameters->cfb6830[cic].vov = Set_UnderOver_Voltage_Threshold(OVERVOLTAGE);
        
    }

    ADBMS_Set_Config_A_6830(parameters->cfa6830,command_bits->cfg_a, NUM_6830, POSITION_6830); //sets for all 6830
    ADBMS_Set_Config_B_6830(parameters->cfb6830,command_bits->cfg_b, NUM_6830, POSITION_6830); // sets for all 6830
    
}

void ADBMS_2950_Config(command_parameters_2950_* parameters, config_command_bits_* command_bits){

    //
    //cfa configs
    //
    parameters->cfa2950.refup = 1; //refup to 1
    parameters->cfa2950.gpo1c = 1; //for opening the mosfet for vbat
    parameters->cfa2950.gpo1od = 0; //for opening the mosfet for vbat

    ADBMS_Set_Config_A_2950(&parameters->cfa2950,command_bits->cfg_a, NUM_2950,POSITION_2950); //2950 is the last chip on the daisychain

    //
    //no cfb configs for now
    //
    ADBMS_Set_Config_B_2950(&parameters->cfb2950,command_bits->cfg_b, NUM_2950,POSITION_2950); //2950 is the last chip on the daisychain

    
}

void ADBMS_joint_Config(command_parameters_joint_* parameters, config_command_bits_* command_bits){

    //
    //adcv configs
    //
    parameters->adcv.cont = 1; //Init sensing cmd

    ADBMS_Set_ADCV(parameters->adcv, &command_bits->adcv); // set adcv


    //
    //adsv configs
    //
    parameters->adsv.cont = 1; // for 2950 measurements

    ADBMS_Set_ADSV(parameters->adsv, &command_bits->adsv); //set adsv

    //
    //adax configs
    //
    ADBMS_Set_ADAX(parameters->adax,&command_bits->adax); //set adax


    //
    //adv configs
    //
    parameters->adv.ow = 0;  //open_wire source off
    parameters->adv.vch = 0; //vch = 0 (this is complicated, for reference check datasheet table 57/58)

    ADBMS_Set_ADV(parameters->adv, &command_bits->adv); //set adv
}


//
// these are all configuration or reads for open-wire checks
//
// this also repeats quite a lot, i think i can change this too

void ADBMS_Owc_Config(adbms_raw_* adbms, Owc_Channel_ channel, Owc_Mode_ mode)
{
    ADBMS_WakeUP_ICs();

    switch(channel)
    {
        
        case C_Channel:
            switch(mode)
            {
                case Channel_Off:
                    adbms->command_parameters.parameter_joint.adcv.cont = 1;
                    adbms->command_parameters.parameter_joint.adcv.ow = 0; //Disable OW
                    break;

                case Channel_Even_On:
                    adbms->command_parameters.parameter_joint.adcv.cont = 1;
                    adbms->command_parameters.parameter_joint.adcv.ow = 1; //Enable Even Channel OW
                    break;
                case Channel_Odd_On:
                    adbms->command_parameters.parameter_joint.adcv.cont = 1;
                    adbms->command_parameters.parameter_joint.adcv.ow = 2; //Enable Odd Channel OW
                    break;
            }
        
            ADBMS_Set_ADCV(adbms->command_parameters.parameter_joint.adcv, &adbms->command_bit.adcv);
            ADBMS_Write_CMD(adbms->SPI_data.hspi,adbms->command_bit.adcv);
            HAL_Delay(1);
            break;
        
            
        case S_Channel:
            switch(mode)
            {
                case Channel_Off:
                adbms->command_parameters.parameter_joint.adsv.cont = 1;
                adbms->command_parameters.parameter_joint.adsv.ow = 0; //Disable OW 
                break;
                case Channel_Even_On:
                    adbms->command_parameters.parameter_joint.adsv.cont = 1;
                    adbms->command_parameters.parameter_joint.adsv.ow = 1; // Enable OW on even-channel 
                    break;
                case Channel_Odd_On:
                    adbms->command_parameters.parameter_joint.adsv.cont = 1;
                    adbms->command_parameters.parameter_joint.adsv.ow = 2; // Enable OW on odd-channel 
                    break;
            
            }
            ADBMS_Set_ADSV(adbms->command_parameters.parameter_joint.adsv, &adbms->command_bit.adsv);
            ADBMS_Write_CMD(adbms->SPI_data.hspi,adbms->command_bit.adsv);
            HAL_Delay(1);
            break;
    }
}





// void Owc_S_Channel_Config(adbms_raw_* adbms, Owc_Channel_Mode_ mode)
// {
//     ADBMS_WakeUP_ICs();

//     switch(mode)
//     {
//         case Channel_Off:
//             adbms->command_parameters.parameter_joint.adsv.cont = 1;
//             adbms->command_parameters.parameter_joint.adsv.ow = 0; //Disable OW 
//             break;
//         case Channel_Even_On:
//             adbms->command_parameters.parameter_joint.adsv.cont = 1;
//             adbms->command_parameters.parameter_joint.adsv.ow = 1; // Enable OW on even-channel 
//             break;
//         case Channel_Odd_On:
//             adbms->command_parameters.parameter_joint.adsv.cont = 1;
//             adbms->command_parameters.parameter_joint.adsv.ow = 2; // Enable OW on odd-channel 
//             break;
//     }

//     ADBMS_Set_ADSV(adbms->command_parameters.parameter_joint.adsv, &adbms->command_bit.adsv);
//     ADBMS_Write_CMD(adbms->SPI_data.hspi,adbms->command_bit.adsv);
//     HAL_Delay(1);
// }


// void Owc_C_Channel_Off(adbms_raw_* adbms)
// {
//     ADBMS_WakeUP_ICs();

//     adbms->command_parameters.parameter_joint.adcv.cont = 1;
//     adbms->command_parameters.parameter_joint.adcv.ow = 0; //Disable OW

//     ADBMS_Set_ADCV(adbms->command_parameters.parameter_joint.adcv, &adbms->command_bit.adcv);

//     ADBMS_Write_CMD(adbms->SPI_data.hspi,adbms->command_bit.adcv);

//     HAL_Delay(1);
// }

// void Owc_C_Channel_Even_On(adbms_raw_* adbms)
// {
//     ADBMS_WakeUP_ICs();

//     adbms->command_parameters.parameter_joint.adcv.cont = 1;
//     adbms->command_parameters.parameter_joint.adcv.ow = 1; //Enable Even Channel OW

//     ADBMS_Set_ADCV(adbms->command_parameters.parameter_joint.adcv, &adbms->command_bit.adcv);

//     ADBMS_Write_CMD(adbms->SPI_data.hspi,adbms->command_bit.adcv);
//     HAL_Delay(8); 
// }

// void Owc_C_Channel_Odd_On(adbms_raw_* adbms)
// {
//     ADBMS_WakeUP_ICs();

//     adbms->command_parameters.parameter_joint.adcv.cont = 1;
//     adbms->command_parameters.parameter_joint.adcv.ow = 2; //Enable Odd Channel OW

//     ADBMS_Set_ADCV(adbms->command_parameters.parameter_joint.adcv, &adbms->command_bit.adcv);

//     ADBMS_Write_CMD(adbms->SPI_data.hspi,adbms->command_bit.adcv);
//     HAL_Delay(8); 
// }


// void Owc_S_Channel_Off(adbms_raw_* adbms)
// {
//     ADBMS_WakeUP_ICs();

//     adbms->command_parameters.parameter_joint.adsv.cont = 1;
//     adbms->command_parameters.parameter_joint.adsv.ow = 0; //Disable OW 

//     ADBMS_Set_ADSV(adbms->command_parameters.parameter_joint.adsv, &adbms->command_bit.adsv);

//     ADBMS_Write_CMD(adbms->SPI_data.hspi,adbms->command_bit.adsv);

//     HAL_Delay(1);
// }

// void Owc_S_Channel_Even_On(adbms_raw_* adbms)
// {
//     // check openwire fault
//     ADBMS_WakeUP_ICs();

//     adbms->command_parameters.parameter_joint.adsv.cont = 1;
//     adbms->command_parameters.parameter_joint.adsv.ow = 1; // Enable OW on even-channel 

//     ADBMS_Set_ADSV(adbms->command_parameters.parameter_joint.adsv, &adbms->command_bit.adsv);

//     ADBMS_Write_CMD(adbms->SPI_data.hspi,adbms->command_bit.adsv);
//     HAL_Delay(8);
// }

// void Owc_S_Channel_Odd_On(adbms_raw_* adbms)
// {
//     // check openwire fault
//     ADBMS_WakeUP_ICs();

//     //same with here
//     //cell_Balance_Off(adbms);  // need to turn off cell balancing to check for OWC

//     /// OWC ODD Check
//     adbms->command_parameters.parameter_joint.adsv.cont = 1;
//     adbms->command_parameters.parameter_joint.adsv.ow = 2; // Enable OW on odd-channel 

//     ADBMS_Set_ADSV(adbms->command_parameters.parameter_joint.adsv, &adbms->command_bit.adsv);

//     ADBMS_Write_CMD(adbms->SPI_data.hspi,adbms->command_bit.adsv);
//     HAL_Delay(8);
// }


//
//all retired read functions
//


// void ADBMS_Read_Voltage(adbms_raw_ *adbms){

//     //get voltages from ADBMS
//     bool pec = 0;

//     ADBMS_WakeUP_ICs();

//     //reading into raw values struct

//     pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVA, (adbms->raw_value.cell + 0 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf); //read voltages 0-2 for each chip //read current for 2950 chip
//     pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVB, (adbms->raw_value.cell + 1 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf); //read voltages 3-5 for each chip
//     pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVC, (adbms->raw_value.cell + 2 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf); //read voltages 6-8 for each chip
//     pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVD, (adbms->raw_value.cell + 3 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf); //read voltages 9-11 for each chip
//     //pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVE, (adbms->raw_value.cell + 4 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf); //DONT NEED VOLTAGES OVER 12 for nfr26

//     adbms->read_failure.read_voltage_pec_failure = pec;
// }


// void Owc_C_Channel_Read(adbms_raw_* adbms)
// {
//     bool pec = 0;
//     ADBMS_WakeUP_ICs();

//     pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVA, (adbms->raw_value.scell + 0 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);
//     pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVB, (adbms->raw_value.scell + 1 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);
//     pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVC, (adbms->raw_value.scell + 2 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);
//     pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVD, (adbms->raw_value.scell + 3 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);
//     //pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDCVE, (adbms->raw_value.scell + 4 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf); // probably don't need this

//     adbms->read_failure.read_open_wire_pec_failure = pec;

// }


// void Owc_S_Channel_Read(adbms_raw_* adbms)
// {
//     bool pec = 0;
//     ADBMS_WakeUP_ICs();

//     pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDSVA, (adbms->raw_value.scell + 0 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);
//     pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDSVB, (adbms->raw_value.scell + 1 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);
//     pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDSVC, (adbms->raw_value.scell + 2 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);
//     pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDSVD, (adbms->raw_value.scell + 3 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf);
//     //pec |= ADBMS_Read_Data(adbms->SPI_data.hspi, RDSVE, (adbms->raw_value.scell + 4 * NUM_CHIPS * DATA_LEN), adbms->SPI_data.spi_dataBuf); // probably don't need this

//     adbms->read_failure.read_open_wire_pec_failure = pec;

// }
