#pragma once

#include "adbms_driver_v2.h"
#include "thermistor_driver.h"
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

}adbms_;


//for all command paramters
typedef struct
{
    command_parameters_6830_    parameter_6830;
    command_parameters_2950_    parameter_2950;
    command_parameters_joint_   parameter_joint;

}configurations_command_parameters_;

typedef struct
{
    //6830 configuration commands
    cfa6830_    cfa6830[NUM_6830];
    cfb6830_    cfb6830[NUM_6830];

}command_parameters_6830_;

typedef struct
{
    //2950 configuration commands
    cfa2950_    cfa2950;
    cfb2950_    cfb2950;

}command_parameters_2950_;

typedef struct
{
    //joint configuration commands
    adcv_       adcv;
    adsv_       adsv;
    adax_       adax;
    adax2_      adax2_;
    adv_        adv;
}command_parameters_joint_;



//interface functions
void ADBMS_Initialize(adbms_ *adbms, SPI_HandleTypeDef *hspi);

//read raw values
void ADBMS_Read_Voltage(adbms_ *adbms);
void ADBMS_Read_Temps(adbms_* adbms);


//chip configurations
void ADBMS_6830_Config(command_parameters_6830_* parameters,config_command_bits_* command_bits);
void ADBMS_2950_Config(command_parameters_2950_* parameters, config_command_bits_* command_bits);
void ADBMS_joint_Config(command_parameters_joint_* parameters, config_command_bits_* command_bits);

//Write Data Command
void ADBMS_Initialize_Write_Data_Command(adbms_ * adbms);
