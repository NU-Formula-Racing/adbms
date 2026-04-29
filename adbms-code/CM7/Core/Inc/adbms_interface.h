#pragma once

#include "adbms_driver.h"
#include "bms_system_prams.h"
#include <float.h>


typedef enum
{
    C_Channel_Read = 0,
    S_Channel_Read

}voltage_read_type_;

typedef struct 
{

    //each is its own raw channel read + own pec failure
    uint8_t c_channel_raw[NUM_CHIPS * CELL_REG_GRP * DATA_LEN];
    uint8_t s_channel_raw[NUM_CHIPS * CELL_REG_GRP * DATA_LEN];
    
    bool voltage_read_pec;

}voltages_raw_;

typedef struct 
{
    //configs
    config_command_bits_                command_bit;
    configurations_command_parameters_  command_parameters;

    //spi configs
    SPI_data_                           SPI_data;

    //read returns
    raw_read_return_values_             raw_value;

    //read failures
    read_failures_                      read_failure;

}adbms_raw_;



//interface functions
void ADBMS_Initialize(adbms_raw_ *adbms, SPI_HandleTypeDef *hspi);

//new read raw values
//repleaces all read_voltages and 
void ADBMS_Read_Raw_Voltage(voltages_raw_* voltages_raw, voltage_read_type_ type, SPI_HandleTypeDef *hspi,uint8_t *spi_dataBuf);;

//replaces by new read raw values function
//void ADBMS_Read_Voltage(adbms_raw_ *adbms);
void ADBMS_Read_Temps(adbms_raw_* adbms);


//chip configurations
void ADBMS_6830_Config(command_parameters_6830_* parameters,config_command_bits_* command_bits);
void ADBMS_2950_Config(command_parameters_2950_* parameters, config_command_bits_* command_bits);
void ADBMS_joint_Config(command_parameters_joint_* parameters, config_command_bits_* command_bits);

//Write Data Command
void ADBMS_Initialize_Write_Data_Command(adbms_raw_* adbms);


//OWC Config and Read C Channel
void Owc_C_Channel_Off(adbms_raw_* adbms);
void Owc_C_Channel_Even_On(adbms_raw_* adbms);
void Owc_C_Channel_Odd_On(adbms_raw_* adbms);

//replaced by new read function
//void Owc_C_Channel_Read(adbms_raw_* adbms);

//OWC Config and Read S Channel
void Owc_S_Channel_Off(adbms_raw_* adbms);
void Owc_S_Channel_Even_On(adbms_raw_* adbms);
void Owc_S_Channel_Odd_On(adbms_raw_* adbms);

//replaced by new read function
//void Owc_S_Channel_Read(adbms_raw_* adbms);