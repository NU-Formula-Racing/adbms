#pragma once

#include "adbms_driver_v2.h"
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

    //read failures
    read_failures_                      read_failure;

}adbms_raw_;




//interface functions
void ADBMS_Initialize(adbms_raw_ *adbms);

//read raw values
void ADBMS_Read_Voltage(adbms_raw_ *adbms);
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
void Owc_C_Channel_Read(adbms_raw_* adbms);

//OWC Config and Read S Channel
void Owc_S_Channel_Off(adbms_raw_* adbms);
void Owc_S_Channel_Even_On(adbms_raw_* adbms);
void Owc_S_Channel_Odd_On(adbms_raw_* adbms);
void Owc_S_Channel_Read(adbms_raw_* adbms);