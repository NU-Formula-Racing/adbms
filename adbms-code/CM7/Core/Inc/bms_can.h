#ifndef ADBMS_CAN_H
#define ADBMS_CAN_H

#include "nfr_can_driver.h"
#include "bms_main_struct.h"
#include "stm32h7xx_hal_fdcan.h"

typedef struct
{
    mainboard_ *mainboard;

    uint32_t TxMailBox_; //FDCAN uses buffer indices?

    FDCAN_RxHeaderTypeDef RxHeader_;
    uint8_t rxData_[8]; //CAN FD supports up to 64 bytes

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

void HAL_FDCAN_RxFifo0MsgPendingCallback(FDCAN_HandleTypeDef *hfdcan);

void BMS_Initialize_Can(mainboard_ *mainboard);

/* send_can_messages updated to use FDCAN types and buffer index pointer */
uint8_t send_can_messages(FDCAN_HandleTypeDef *hfdcan, FDCAN_TxHeaderTypeDef *TxHeader, uint8_t *data, uint32_t *TxBufferIndex);

/* CAN Loops */
void drive_can_loop();
/*void data_can_loop();*/

void populateBMS_SOC(uint8_t *data);
void populateBMS_Faults(uint8_t *data);
void populateBMS_Status(uint8_t *data);
void populateBMS_VoltageMessages(uint8_t *data, int volt_msg_num);
void populateBMS_TemperatureMessages(uint8_t *data, int temp_num);

#endif // ADBMS_CAN_H
