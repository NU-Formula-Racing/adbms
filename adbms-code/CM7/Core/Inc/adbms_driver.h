#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "stm32h7xx_hal.h"
#include "adbms_cmd.h"
#include "bms_system_prams.h"


#define SPI_TIME_OUT HAL_MAX_DELAY

//
//all command bits
//
//this struct stores all the parsed bit values of commands
typedef struct
{
    //config A and config B
    uint8_t cfg_a[NUM_CHIPS * DATA_LEN];
    uint8_t cfg_b[NUM_CHIPS * DATA_LEN];

    // 6830 commands that also work for 2950
    uint16_t adcv;
    uint16_t adsv;
    uint16_t adax;
    uint16_t adax2;

    // 2950 specific command
    uint16_t adv;

} config_command_bits_;

//this struct stores all spi related stuff
typedef struct
{
    SPI_HandleTypeDef *hspi;
    uint8_t spi_dataBuf[DATABUF_LEN];

} SPI_data_;

//this struct stores all struct of all the bit values coming back from reads
typedef struct
{
    //this is moved over to the new struct
    //uint8_t cell[NUM_CHIPS * CELL_REG_GRP * DATA_LEN];
    //uint8_t scell[NUM_CHIPS * CELL_REG_GRP * DATA_LEN];

    uint8_t aux[NUM_CHIPS * CELL_REG_GRP * DATA_LEN]; 
    uint8_t shunt_temp[NUM_CHIPS * DATA_LEN];

} raw_read_return_values_;

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
    //we get rid of this, moved into new struct
    //bool read_voltage_pec_failure;

    bool read_temp_pec_failure;
    bool read_open_wire_pec_failure;

} read_failures_;


//
// This section is for commands that have options that can be changed
//

typedef struct
{
    uint8_t       refon   :1;
    uint8_t       cth     :3;
    uint8_t       flag_d  :8;
    uint8_t       soakon  :1;
    uint8_t       owrng   :1;
    uint8_t       owa     :3;
    uint16_t      gpo     :10;
    uint8_t       snap    :1;
    uint8_t       mute_st :1;
    uint8_t       comm_bk :1;
    uint8_t       fc      :3;
}cfa6830_;


typedef struct{
    uint8_t       ocen    :1;
    uint8_t       vs5     :1;
    uint8_t       vs4     :1;
    uint8_t       vs3     :1;
    uint8_t       vs2     :2;
    uint8_t       vs1     :2;
    uint8_t       injtm   :1;
    uint8_t       injecc  :1;
    uint8_t       injts   :1;
    uint8_t       injmon  :2;
    uint8_t       injosc  :2;
    uint8_t       soak    :3;
    uint8_t       vs10    :1;
    uint8_t       vs9     :1;
    uint8_t       vs8     :1;
    uint8_t       vs7     :1;
    uint8_t       vs6     :1;
    uint8_t       gpo6c   :2;
    uint8_t       gpo5c   :1;
    uint8_t       gpo4c   :1;
    uint8_t       gpo3c   :1;
    uint8_t       gpo2c   :1;
    uint8_t       gpo1c   :1;
    uint8_t       spi3w   :1;
    uint8_t       gpio1fe :1;
    uint8_t       gpo6od  :1;
    uint8_t       gpo5od  :1;
    uint8_t       gpo4od  :1;
    uint8_t       gpo3od  :1;
    uint8_t       gpo2od  :1;
    uint8_t       gpo1od  :1;
    uint8_t       vb2mux  :1;
    uint8_t       vb1mux  :1;
    uint8_t       snapst  :1;
    uint8_t       refup   :1;
    uint8_t       commbk  :1;
    uint8_t       acci    :3;

}cfa2950_;

/* For ADBMS6830 config register structure */
typedef struct
{
    uint16_t 	vuv     :12;
    uint16_t 	vov     :12;
    uint8_t 	dtmen   :1;
    uint8_t 	dtrng   :1;
    uint8_t 	dcto    :6;
    uint16_t 	dcc     :16;
}cfb6830_;

typedef struct
{
    uint8_t     oc1th       :7;
    uint8_t     oc2th       :7;
    uint8_t     oc3th       :7;
    uint8_t     ocdp        :1;
    uint8_t     ocdgt       :2;
    uint8_t     ocbx        :1; 
    uint8_t     ocax        :1;
    uint8_t     ocmode      :2;
    uint8_t     oc3gc       :1;
    uint8_t     oc2gc       :1;
    uint8_t     oc1gc       :1;
    uint8_t     ocod        :1;
    uint8_t     gpio4c      :1;
    uint8_t     gpio3c      :1;
    uint8_t     gpio2c      :1;
    uint8_t     gpio1c      :1;
    uint8_t     gpio2eoc    :1;
    uint8_t     diagsel     :3;

}cfb2950_;

typedef struct
{
    uint8_t rd    :1;
    uint8_t cont  :1;
    uint8_t dcp   :1;
    uint8_t rstf  :1;
    uint8_t ow    :2;
}adcv_;

typedef struct
{
    uint8_t cont  :1;
    uint8_t dcp   :1;
    uint8_t ow    :2;
}adsv_;

typedef struct
{
    uint8_t ow    :1;
    uint8_t pup   :1;
    uint8_t ch    :5;
}adax_;

typedef struct
{
    uint8_t ch    :4;
}adax2_;

//this struct is for sending command adv for 2950 which starts v1adc and v2adc
typedef struct
{
    uint8_t       ow      :2;
    uint8_t       vch     :4;
  
}adv_;


//
// all command parameters
//
// this is moved down a level so it can be accessed in the lower interface

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

//for all command paramters
typedef struct
{
    command_parameters_6830_    parameter_6830;
    command_parameters_2950_    parameter_2950;
    command_parameters_joint_   parameter_joint;

}configurations_command_parameters_;


//driver functions
uint16_t Pec15_Calc(uint8_t len, uint8_t *data);
uint16_t Pec10_Calc(bool isRxCmd, int len, uint8_t *data);
uint16_t Set_UnderOver_Voltage_Threshold(float voltage);


//6830 configuration functions
void ADBMS_Set_Config_A_6830(cfa6830_ *cfg_a, uint8_t *cfg_a_tx_buffer, uint8_t num_6830, uint8_t position);
void ADBMS_Set_Config_B_6830(cfb6830_ *cfg_b, uint8_t *cfg_b_tx_buffer, uint8_t num_6830, uint8_t position);


//2950 configuration functions
void ADBMS_Set_Config_A_2950(cfa2950_* cfg_a2950, uint8_t* cfg_a_tx_buffer, uint8_t num_2950, uint8_t position);
void ADBMS_Set_Config_B_2950(cfb2950_* cfg_b2950, uint8_t* cfg_b_tx_buffer, uint8_t num_2950, uint8_t position);

//joint configuration functions
void ADBMS_Set_ADCV(adcv_ adcv, uint16_t *adcv_cmd_buffer);
void ADBMS_Set_ADSV(adsv_ adsv, uint16_t *adsv_cmd_buffer);
void ADBMS_Set_ADAX(adax_ adax, uint16_t *adax_cmd_buffer);
void ADBMS_Set_ADAX2(adax2_ adax2, uint16_t *adax2_cmd_buffer);
void ADBMS_Set_ADV(adv_ adv, uint16_t* adv_cmd_buffer);

//read write function
void ADBMS_WakeUP_ICs();
void ADBMS_Write_CMD(SPI_HandleTypeDef *hspi, uint16_t tx_cmd);
void ADBMS_Write_Data(SPI_HandleTypeDef *hspi, uint16_t tx_cmd, uint8_t *data, uint8_t *spi_dataBuf);
bool ADBMS_Read_Data(SPI_HandleTypeDef *hspi, uint16_t tx_cmd, uint8_t *data, uint8_t *spi_dataBuf);