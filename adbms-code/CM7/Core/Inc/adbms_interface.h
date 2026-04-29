#pragma once

#include "adbms_driver.h"
#include "bms_system_prams.h"
#include <float.h>


typedef struct 
{
    //configs
    config_command_bits_                command_bit;
    configurations_command_parameters_  command_parameters;

    //spi configs
    SPI_data_                           SPI_data;

    //read returns
    raw_read_return_values_             raw_value;
    voltages_raw_                       raw_voltages;

    //read failures
    read_failures_                      read_failure;

}adbms_raw_;

typedef enum
{
    Channel_Odd_On = 0,
    Channel_Even_On,
    Channel_Off

}Owc_Channel_Mode_;


//
//interface functions
//

//initializes configuration and turns on adcs
void ADBMS_Initialize(adbms_raw_ *adbms, SPI_HandleTypeDef *hspi);


//read functions
void ADBMS_Read_Voltages(voltages_raw_* voltages_raw, voltage_read_type_ type, SPI_HandleTypeDef *hspi,uint8_t *spi_dataBuf);;
void ADBMS_Read_Temps(adbms_raw_* adbms);


//chip configurations
void ADBMS_6830_Config(command_parameters_6830_* parameters,config_command_bits_* command_bits);
void ADBMS_2950_Config(command_parameters_2950_* parameters, config_command_bits_* command_bits);
void ADBMS_joint_Config(command_parameters_joint_* parameters, config_command_bits_* command_bits);


//OWC Config Functions
void Owc_C_Channel_Config(adbms_raw_* adbms, Owc_Channel_Mode_ mode);
void Owc_S_Channel_Config(adbms_raw_* adbms, Owc_Channel_Mode_ mode);

