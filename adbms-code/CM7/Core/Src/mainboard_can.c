#include "mainboard_can.h"

FDCAN_RxHeaderTypeDef test_header_rx;
uint8_t rxData_[8];

FDCAN_TxHeaderTypeDef test_header_tx;
uint8_t txData[8];

//test_header_tx.ExtId = 0x1806E5F4;
//test_header_tx.IDE = CAN_ID_EXT;
//test_header_tx.RTR = CAN_RTR_DATA;
//test_header_tx.DLC = 8;
