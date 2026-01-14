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
    FDCAN_TxHeaderTypeDef TxHeaderSOC_;
    uint8_t txDataSOC_[8];

    FDCAN_TxHeaderTypeDef TxHeaderFaults_;
    uint8_t txDataFaults_[8];

    FDCAN_TxHeaderTypeDef TxHeaderStatus_;
    uint8_t txDataStatus_[8];

    FDCAN_TxHeaderTypeDef TxHeaderCharger_;
    uint8_t txDataCharger_[8];

    // data can messages
    FDCAN_TxHeaderTypeDef TxHeaderVoltages_;
    uint8_t txDataVoltages_[8];

    FDCAN_TxHeaderTypeDef TxHeaderTemperatures_;
    uint8_t txDataTemperatures_[8];
} bms_can_;

void Bms_Initialize_Can(mainboard_ *mainboard);

void Check_Rx(FDCAN_HandleTypeDef *hfdcan);

/* CAN Loops */
void Drive_Can_Loop();
void Data_Can_Loop();

void populate_bms_soc(uint8_t *data);
void populate_bms_faults(uint8_t *data);
void populate_bms_status(uint8_t *data);
void populate_bms_voltages(uint8_t *data, int volt_msg_num);
void populate_bms_temparatures(uint8_t *data, int temp_num);

/* send_can_messages updated to use FDCAN types and buffer index pointer */
uint8_t send_can_messages(FDCAN_HandleTypeDef *hfdcan, FDCAN_TxHeaderTypeDef *TxHeader, uint8_t *data, uint32_t *TxBufferIndex);


#endif // ADBMS_CAN_H
