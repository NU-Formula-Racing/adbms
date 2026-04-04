#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "stm32h7xx_hal.h"
#include "adbms_cmd.h"
#include "bms_system_prams.h"

// TODO: Change this value
#define SPI_TIME_OUT HAL_MAX_DELAY  /* SPI Time out delay */
typedef struct
{
    SPI_HandleTypeDef *hspi;

    uint8_t spi_dataBuf[DATABUF_LEN];

    // Config groups a,b
    uint8_t cfg_a[NUM_CHIPS * DATA_LEN];
    uint8_t cfg_b[NUM_CHIPS * DATA_LEN];

    // AD Commands 
    uint16_t adcv;
    uint16_t adsv;
    uint16_t adax;
    uint16_t adax2;

    // 2950 commands 
    uint16_t adv;

    uint8_t cell[NUM_CHIPS * CELL_REG_GRP * DATA_LEN];
    uint8_t scell[NUM_CHIPS * CELL_REG_GRP * DATA_LEN]; //(ALL OF THESE TAKE UP MORE SPACE THAN NEEDED)
    uint8_t aux[NUM_CHIPS * CELL_REG_GRP * DATA_LEN]; 

    //array to store shunt thermister voltages
    uint8_t shunt_temp[NUM_CHIPS * DATA_LEN];
    

} adbms6830_ICs;

//2950 struct, could change in the future to add more parameters
//depending on what gets measured
typedef struct{

  //pre contactor voltages
  float vbat;

  //current
  float i1;
  float i2;

  //post contactor voltages
  float v_TS;

  //Shunt Thermistor Voltages
  float v_shunt_1;
  float v_shunt_2;

} data_2950;

//this struct is for sending command adv for 2950 which starts v1adc and v2adc
typedef struct
{
  uint8_t       ow      :2;
  uint8_t       vch     :4;
  
}adv_;

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
}cfa_;


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
}cfb_;

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

uint16_t Pec15_Calc(uint8_t len, uint8_t *data);
uint16_t Pec10_Calc(bool isRxCmd, int len, uint8_t *data);

uint16_t Set_UnderOver_Voltage_Threshold(float voltage);
float ADBMS_getVoltage(int data);

void ADBMS_Set_Config_A(cfa_ *cfg_a, uint8_t *cfg_a_tx_buffer, uint8_t num_6830);
void ADBMS_Set_Config_A_2950(cfa2950_* cfg_a2950, uint8_t* cfg_a_tx_buffer, uint8_t chip_position);
void ADBMS_Set_Config_B(cfb_ *cfg_b, uint8_t *cfg_b_tx_buffer);
void ADBMS_Set_ADCV(adcv_ adcv, uint16_t *adcv_cmd_buffer);
void ADBMS_Set_ADSV(adsv_ adsv, uint16_t *adsv_cmd_buffer);
void ADBMS_Set_ADAX(adax_ adax, uint16_t *adax_cmd_buffer);
void ADBMS_Set_ADAX2(adax2_ adax2, uint16_t *adax2_cmd_buffer);
void ADBMS_Set_ADV(adv_ adv, uint16_t* adv_cmd_buffer);

void ADBMS_WakeUP_ICs();
void ADBMS_Write_CMD(SPI_HandleTypeDef *hspi, uint16_t tx_cmd);
void ADBMS_Write_Data(SPI_HandleTypeDef *hspi, uint16_t tx_cmd, uint8_t *data, uint8_t *spi_dataBuf);
bool ADBMS_Read_Data(SPI_HandleTypeDef *hspi, uint16_t tx_cmd, uint8_t *data, uint8_t *spi_dataBuf);
