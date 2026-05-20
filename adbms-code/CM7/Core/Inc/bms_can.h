#ifndef ADBMS_CAN_H
#define ADBMS_CAN_H

#include "nfr_can_driver.h"
#include "bms_main_struct.h"
#include "stm32h7xx_hal_fdcan.h"

typedef struct
{
    mainboard_ *mainboard;

    FDCAN_RxHeaderTypeDef RxHeader_;
    uint8_t rxData_[8]; 

    // Drive CAN Messages
    FDCAN_TxHeaderTypeDef TxHeaderPackboard_;
    uint8_t txDataPackboard_[8];

    FDCAN_TxHeaderTypeDef TxHeaderDaughterboard_;
    uint8_t txDataDaughterboard_[8];

    FDCAN_TxHeaderTypeDef TxHeaderStatus_;
    uint8_t txDataStatus_[8];

    FDCAN_TxHeaderTypeDef TxHeaderCharger_;
    uint8_t txDataCharger_[8];

    FDCAN_TxHeaderTypeDef TxHeaderOWC_C_;
    uint8_t txDataOWC_C_[8];

    FDCAN_TxHeaderTypeDef TxHeaderOWC_S_;
    uint8_t txDataOWC_S_[8];

    // large messages
    FDCAN_TxHeaderTypeDef TxHeaderVoltages_;
    uint8_t txDataVoltages_[8];
    uint8_t current_voltage_msg;

    FDCAN_TxHeaderTypeDef TxHeaderTemperatures_;
    uint8_t txDataTemperatures_[8];
    uint8_t current_temp_msg;
} bms_can_;

void Bms_Initialize_Can(mainboard_ *mainboard);

/* CAN Loops */
void Can_Loop();
void owc_can_loop();

void populate_bms_packboard(uint8_t *data);
void populate_bms_daughterboard(uint8_t *data);
void populate_bms_status(uint8_t *data);
void populate_bms_voltages(uint8_t *data, int volt_msg_num);
void populate_bms_temparatures(uint8_t *data, int temp_msg_num);
void populate_bms_owc(uint8_t *data, uint8_t owc_msg_num, Owc_Channel_ channel);
void populateCharger_Msg(uint8_t *data);

/* send_can_messages updated to use FDCAN types and buffer index pointer */
uint8_t send_can_messages(FDCAN_HandleTypeDef *hcan, FDCAN_TxHeaderTypeDef *TxHeader, uint8_t *data);


#endif // ADBMS_CAN_H
