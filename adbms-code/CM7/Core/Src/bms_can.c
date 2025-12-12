#include "bms_can.h"

bms_can_ bms_can;


void BMS_Initialize_Can(mainboard_ *mainboard)
{
    // Start CAN (FD)
    bms_can.mainboard = mainboard;
    HAL_FDCAN_Start(bms_can.mainboard->hcan_drive);
    HAL_FDCAN_Start(bms_can.mainboard->hcan_data);

    // Enable notifications (interrupts) for FDCAN FIFO 0 new message
    HAL_FDCAN_ActivateNotification(bms_can.mainboard->hcan_drive, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);

    // SOC header initialization (FDCAN)
    bms_can.TxHeaderSOC_.Identifier   = 0x150;
    bms_can.TxHeaderSOC_.IdType       = FDCAN_STANDARD_ID;
    bms_can.TxHeaderSOC_.TxFrameType  = FDCAN_DATA_FRAME;
    bms_can.TxHeaderSOC_.DataLength   = FDCAN_DLC_BYTES_8;

    // Faults header initialization
    bms_can.TxHeaderFaults_.Identifier  = 0x151;
    bms_can.TxHeaderFaults_.IdType      = FDCAN_STANDARD_ID;
    bms_can.TxHeaderFaults_.TxFrameType = FDCAN_DATA_FRAME;
    bms_can.TxHeaderFaults_.DataLength  = FDCAN_DLC_BYTES_8;

    // Status header initialization
    bms_can.TxHeaderStatus_.Identifier  = 0x152;
    bms_can.TxHeaderStatus_.IdType      = FDCAN_STANDARD_ID;
    bms_can.TxHeaderStatus_.TxFrameType = FDCAN_DATA_FRAME;
    bms_can.TxHeaderStatus_.DataLength  = FDCAN_DLC_BYTES_8;

    // Charger header initialization (extended)
    bms_can.TxHeaderCharger_.Identifier  = 0x1806E5F4;
    bms_can.TxHeaderCharger_.IdType      = FDCAN_EXTENDED_ID;
    bms_can.TxHeaderCharger_.TxFrameType = FDCAN_DATA_FRAME;
    bms_can.TxHeaderCharger_.DataLength  = FDCAN_DLC_BYTES_8;

    // Voltages header initialization
    bms_can.TxHeaderVoltages_.Identifier  = 0x153;
    bms_can.TxHeaderVoltages_.IdType      = FDCAN_STANDARD_ID;
    bms_can.TxHeaderVoltages_.TxFrameType = FDCAN_DATA_FRAME;
    bms_can.TxHeaderVoltages_.DataLength  = FDCAN_DLC_BYTES_8;

    // Temperatures header initialization
    bms_can.TxHeaderTemperatures_.Identifier  = 0x167;
    bms_can.TxHeaderTemperatures_.IdType      = FDCAN_STANDARD_ID;
    bms_can.TxHeaderTemperatures_.TxFrameType = FDCAN_DATA_FRAME;
    bms_can.TxHeaderTemperatures_.DataLength  = FDCAN_DLC_BYTES_8;
}

void HAL_FDCAN_RxFifo0MsgPendingCallback(FDCAN_HandleTypeDef *hfdcan)
{
    // get message
    HAL_StatusTypeDef can_rx_status = HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &bms_can.RxHeader_, bms_can.rxData_);

    // error check
    if (can_rx_status != HAL_OK)
    {
        printf("FDCAN RX Error");
        return;
    }

    // Process the data

    // ECU Message (standard ID 0x205)
    if ((bms_can.RxHeader_.IdType == FDCAN_STANDARD_ID) && (bms_can.RxHeader_.Identifier == 0x205))
    {
        // update ecu last msg time
        bms_can.mainboard->ecu_messege->last_msg_time = HAL_GetTick();

        uint8_t ecu_data = bms_can.rxData_[0];
        if (ecu_data == 0) {
            bms_can.mainboard->ecu_command = go_to_idle;
        }
        else if (ecu_data == 1) {
            bms_can.mainboard->ecu_command = go_to_active;
        }
    }

    // Inverter Message (standard ID 0x281)
    if ((bms_can.RxHeader_.IdType == FDCAN_STANDARD_ID) && (bms_can.RxHeader_.Identifier == 0x281))
    {
        // update inverter last msg time
        bms_can.mainboard->inverter_messege->last_msg_time  = HAL_GetTick();

        uint16_t inverter_raw_voltage = (bms_can.rxData_[4] & 0xFF) | (bms_can.rxData_[5] << 8);
        bms_can.mainboard->Inverter_DC_Voltage = ((float)inverter_raw_voltage) * 0.1;
    }

    // Charger Message (extended ID 0x18FF50E5)
    if ((bms_can.RxHeader_.IdType == FDCAN_EXTENDED_ID) && (bms_can.RxHeader_.Identifier == 0x18FF50E5))
    {
        // update charger last msg time
        bms_can.mainboard->charger_messege->last_msg_time = HAL_GetTick();

        //got charger messege, means we are connected to charger
        bms_can.mainboard->state = Charge;

        // big endian
        bms_can.mainboard->charger_status = bms_can.rxData_[4];
        uint16_t charger_raw_voltage = (bms_can.rxData_[0] << 8) | (bms_can.rxData_[1] & 0xFF);
        uint16_t charger_raw_current = (bms_can.rxData_[2] << 8) | (bms_can.rxData_[3] & 0xFF);
        bms_can.mainboard->charger_voltage = ((float)charger_raw_voltage) * 0.1;
        bms_can.mainboard->charger_current = ((float)charger_raw_current) * 0.1;
    }
}

uint8_t send_can_messages(FDCAN_HandleTypeDef *hfdcan, FDCAN_TxHeaderTypeDef *TxHeader, uint8_t *data, uint32_t *TxMailBox)
{
    // For FD CAN we use the FIFOQ API. The TxMailBox parameter is kept for compatibility but not used by HAL_FDCAN_AddMessageToTxFifoQ.
    HAL_StatusTypeDef msg_status = HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, TxHeader, data);

    if (msg_status != HAL_OK)
    {
        // Error handling
//		printf("FDCAN Message failed\n");
        return 1;
    }
    return 0;
}


void drive_can_loop()
{
    // update and send general info
    populateBMS_SOC(bms_can.txDataSOC_);
    send_can_messages(bms_can.mainboard->hcan_drive, &bms_can.TxHeaderSOC_, bms_can.txDataSOC_, &bms_can.TxMailBox_);

    // update and send faults
    populateBMS_Faults(bms_can.txDataFaults_);
    send_can_messages(bms_can.mainboard->hcan_drive, &bms_can.TxHeaderFaults_, bms_can.txDataFaults_, &bms_can.TxMailBox_);

    // update and send in depth data
    populateBMS_Status(bms_can.txDataStatus_);
    send_can_messages(bms_can.mainboard->hcan_drive, &bms_can.TxHeaderStatus_, bms_can.txDataStatus_, &bms_can.TxMailBox_);
}


// 	// send voltage messages
// 	bms_can.TxHeaderVoltages_.Identifier = 0x153; // set the message id for next iteration
// 	for(int i = 0; i < NUM_DATA_CAN_VOLTAGE_MSGS; i++) {
// 		populateBMS_VoltageMessages(bms_can.txDataVoltages_, i);
// 		send_can_messages(bms_can.mainboard->hcan_data, &bms_can.TxHeaderVoltages_, bms_can.txDataVoltages_, &bms_can.TxMailBox_);
// 		bms_can.TxHeaderVoltages_.Identifier++;
// 	}

// 	// send temperature messages
// 	bms_can.TxHeaderTemperatures_.Identifier = 0x167; // set the message id for next iteration
// 	for(int i = 0; i < NUM_DATA_CAN_TEMP_MSGS; i++) {
// 		populateBMS_TemperatureMessages(bms_can.txDataTemperatures_, i);
// 		send_can_messages(bms_can.mainboard->hcan_data, &bms_can.TxHeaderTemperatures_, bms_can.txDataTemperatures_, &bms_can.TxMailBox_);
// 		bms_can.TxHeaderTemperatures_.Identifier++;
// 	}


void populateBMS_SOC(uint8_t *data)
{
    RawCanSignal signals[5];
    populateRawMessage(&signals[0], 0, 12, 0.1, 0);									  // max discharge current
    populateRawMessage(&signals[1], 0, 12, 0.1, 0);									  // max regen current
    populateRawMessage(&signals[2], bms_can.mainboard->adbms.total_v, 16, 0.01, 0);   // battery voltage
    populateRawMessage(&signals[3], bms_can.mainboard->adbms.avg_temp, 8, 1, -40);    // battery temp
    populateRawMessage(&signals[4], bms_can.mainboard->current, 16, 0.01, 0);		  // battery current
    encodeSignals(data, 5, signals[0], signals[1], signals[2], signals[3], signals[4]);
}

void populateBMS_Faults(uint8_t *data)
{
    RawCanSignal signals[8];
    populateRawMessage(&signals[0], bms_can.mainboard->bms_fault, 1, 1, 0);																  	// fault summary
    populateRawMessage(&signals[1], bms_can.mainboard->adbms.undervoltage_fault_, 1, 1, 0);												  	// undervoltage fault
    populateRawMessage(&signals[2], bms_can.mainboard->adbms.overvoltage_fault_, 1, 1, 0);												   	// overvoltage fault
    populateRawMessage(&signals[3], bms_can.mainboard->adbms.undertemperature_fault_, 1, 1, 0);												// undertemp fault
    populateRawMessage(&signals[4], bms_can.mainboard->adbms.overtemperature_fault_, 1, 1, 0);											 	// overemp fault
    populateRawMessage(&signals[5], bms_can.mainboard->overcurrent_fault, 1, 1, 0);														 	// overcurrent fault
    populateRawMessage(&signals[6], bms_can.mainboard->external_fault, 1, 1, 0);												   			// external fault
    populateRawMessage(&signals[7], (bms_can.mainboard->adbms.openwire_fault_ || bms_can.mainboard->adbms.openwire_temp_fault_), 1, 1, 0);	// open wire fault
    encodeSignals(data, 8, signals[0], signals[1], signals[2], signals[3], signals[4], signals[5], signals[6], signals[7]);
}

void populateBMS_Status(uint8_t *data)
{
    RawCanSignal signals[7];

    populateRawMessage(&signals[0], bms_can.mainboard->state, 8, 1, 0);		 		 // BMS State
    populateRawMessage(&signals[1], bms_can.mainboard->imd_status, 8, 1, 0);		 // IMD State
    populateRawMessage(&signals[2], bms_can.mainboard->adbms.max_temp, 8, 1, -40);   // max cell temp
    populateRawMessage(&signals[3], bms_can.mainboard->adbms.min_temp, 8, 1, -40);   // min cell temp
    populateRawMessage(&signals[4], bms_can.mainboard->adbms.max_v, 8, 0.012, 2);	 // max cell voltage
    populateRawMessage(&signals[5], bms_can.mainboard->adbms.min_v, 8, 0.012, 2);	 // min cell voltage
    populateRawMessage(&signals[6], 0, 8, 0.5, 0);									 // BMS SOC
    encodeSignals(data, 7, signals[0], signals[1], signals[2], signals[3], signals[4], signals[5], signals[6]);
}


void populateBMS_VoltageMessages(uint8_t *data, int volt_msg_num)
{
    RawCanSignal signals[8];
    for(int i = 0; i < NUM_DATA_CAN_VOLTAGES_PER_MSG; i++){
        populateRawMessage(&signals[i], bms_can.mainboard->adbms.voltages[volt_msg_num * NUM_DATA_CAN_VOLTAGES_PER_MSG + i], 8, 0.012, 2);
    }
    populateRawMessage(&signals[7], 0, 8, 0.004, 0);	// OCV msg that is legacy from BQ code and only included for backwards compatibility
    // num_per_msg + 1 because includes the added OCV msg
    encodeSignals(data, NUM_DATA_CAN_VOLTAGES_PER_MSG+1, signals[0], signals[1], signals[2], signals[3], signals[4], signals[5], signals[6], signals[7]);
}

void populateBMS_TemperatureMessages(uint8_t *data, int temp_num)
{
    RawCanSignal signals[8];
    for(int i = 0; i < NUM_DATA_CAN_TEMPS_PER_MSG; i++){
        populateRawMessage(&signals[i], bms_can.mainboard->adbms.temperatures[temp_num * NUM_DATA_CAN_TEMPS_PER_MSG + i], 8, 1, -40);
    }
    encodeSignals(data, NUM_DATA_CAN_TEMPS_PER_MSG, signals[0], signals[1], signals[2], signals[3], signals[4], signals[5], signals[6], signals[7]);
}
