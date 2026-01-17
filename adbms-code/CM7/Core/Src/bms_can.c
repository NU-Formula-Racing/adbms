#include "bms_can.h"

bms_can_ bms_can;

void Bms_Initialize_Can(mainboard_ *mainboard)
{
	// Add mainboard
	bms_can.mainboard = mainboard;

	// assign interrupt line to rx fifo
	HAL_FDCAN_ConfigInterruptLines(adbms_can.mainboard->hcan_drive, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, FDCAN_INTERRUPT_LINE0);

	// activate interrupt line for rx fifo
	HAL_FDCAN_ActivateNotification(adbms_can.mainboard->hcan_drive, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);

	HAL_FDCAN_Start(adbms_can.mainboard->hcan_drive);

	// SOC header initialization
    bms_can.TxHeaderSOC_.Identifier = SOC_ID;
    bms_can.TxHeaderSOC_.IdType = FDCAN_STANDARD_ID;
    bms_can.TxHeaderSOC_.TxFrameType = FDCAN_DATA_FRAME;
    bms_can.TxHeaderSOC_.DataLength = FDCAN_DLC_BYTES_8;
    bms_can.TxHeaderSOC_.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    bms_can.TxHeaderSOC_.BitRateSwitch = FDCAN_BRS_OFF;
    bms_can.TxHeaderSOC_.FDFormat = FDCAN_CLASSIC_CAN;
    bms_can.TxHeaderSOC_.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    bms_can.TxHeaderSOC_.MessageMarker = 0;

	// Faults header initialization
	bms_can.TxHeaderFaults_.Identifier = FAULT_ID;
	bms_can.TxHeaderFaults_.IdType = FDCAN_STANDARD_ID;
	bms_can.TxHeaderFaults_.TxFrameType = FDCAN_DATA_FRAME;
	bms_can.TxHeaderFaults_.DataLength = FDCAN_DLC_BYTES_8;
  	bms_can.TxHeaderFaults_.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
 	bms_can.TxHeaderFaults_.BitRateSwitch = FDCAN_BRS_OFF;
   	bms_can.TxHeaderFaults_.FDFormat = FDCAN_CLASSIC_CAN;
   	bms_can.TxHeaderFaults_.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
 	bms_can.TxHeaderFaults_.MessageMarker = 0;

	// Status header initialization
	bms_can.TxHeaderStatus_.Identifier = STATUS_ID;
    bms_can.TxHeaderStatus_.IdType = FDCAN_STANDARD_ID;
    bms_can.TxHeaderStatus_.TxFrameType = FDCAN_DATA_FRAME;
    bms_can.TxHeaderStatus_.DataLength = FDCAN_DLC_BYTES_8;
    bms_can.TxHeaderStatus_.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    bms_can.TxHeaderStatus_.BitRateSwitch = FDCAN_BRS_OFF;
    bms_can.TxHeaderStatus_.FDFormat = FDCAN_CLASSIC_CAN;
    bms_can.TxHeaderStatus_.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    bms_can.TxHeaderStatus_.MessageMarker = 0;

	// Charger header initialization
	bms_can.TxHeaderCharger_.Identifier = CHARGER_ID;
    bms_can.TxHeaderCharger_.IdType = FDCAN_STANDARD_ID;
    bms_can.TxHeaderCharger_.TxFrameType = FDCAN_DATA_FRAME;
    bms_can.TxHeaderCharger_.DataLength = FDCAN_DLC_BYTES_8;
    bms_can.TxHeaderCharger_.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    bms_can.TxHeaderCharger_.BitRateSwitch = FDCAN_BRS_OFF;
    bms_can.TxHeaderCharger_.FDFormat = FDCAN_CLASSIC_CAN;
    bms_can.TxHeaderCharger_.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    bms_can.TxHeaderCharger_.MessageMarker = 0;

	// Voltages header initialization
    bms_can.TxHeaderVoltages_.Identifier = VOLTAGES_ID;
    bms_can.TxHeaderVoltages_.IdType = FDCAN_STANDARD_ID;
    bms_can.TxHeaderVoltages_.TxFrameType = FDCAN_DATA_FRAME;
    bms_can.TxHeaderVoltages_.DataLength = FDCAN_DLC_BYTES_8;
    bms_can.TxHeaderVoltages_.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    bms_can.TxHeaderVoltages_.BitRateSwitch = FDCAN_BRS_OFF;
    bms_can.TxHeaderVoltages_.FDFormat = FDCAN_CLASSIC_CAN;
    bms_can.TxHeaderVoltages_.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    bms_can.TxHeaderVoltages_.MessageMarker = 0;

	// Temperatures header initialization
	bms_can.TxHeaderTemperatures_.Identifier = TEMPS_ID;
    bms_can.TxHeaderTemperatures_.IdType = FDCAN_STANDARD_ID;
    bms_can.TxHeaderTemperatures_.TxFrameType = FDCAN_DATA_FRAME;
    bms_can.TxHeaderTemperatures_.DataLength = FDCAN_DLC_BYTES_8;
    bms_can.TxHeaderTemperatures_.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    bms_can.TxHeaderTemperatures_.BitRateSwitch = FDCAN_BRS_OFF;
    bms_can.TxHeaderTemperatures_.FDFormat = FDCAN_CLASSIC_CAN;
    bms_can.TxHeaderTemperatures_.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    bms_can.TxHeaderTemperatures_.MessageMarker = 0;

}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
	
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0)
    {
        FDCAN_RxHeaderTypeDef RxHeader;
        uint8_t RxData[8];
        
        // Get the message
        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, RxData) != HAL_OK)
        {
             return;
        }

        // Process Data
    }
}

void Check_Rx(FDCAN_HandleTypeDef *hfdcan)
{
    if (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan, FDCAN_RX_FIFO0) > 0)
    {
		if (HAL_FDCAN_GetRxMessage(&hfdcan, FDCAN_RX_FIFO0, &bms_can.RxHeader_, bms_can.rxData_) == HAL_OK)
        {
            // ECU Message
            if (bms_can.RxHeader_.Identifier == 0x205)
            {
                // update ecu last msg time
                bms_can.mainboard->ecu_last_msg_time = HAL_GetTick();

                uint8_t ecu_cmd = bms_can.rxData_[0];
                bms_can.mainboard->ECU_Cmd_Close_Contactors = !ecu_cmd; // ecu cmd 0 means close contactors
            }

            // Inverter Message
            if (bms_can.RxHeader_.Identifier == 0x281)
            {
                // update inverter last msg time
                bms_can.mainboard->inverter_last_msg_time = HAL_GetTick();

                uint16_t inverter_raw_voltage = (bms_can.rxData_[4] & 0xFF) | (bms_can.rxData_[5] << 8);
                bms_can.mainboard->Inverter_DC_Voltage = ((float)inverter_raw_voltage) * 0.1;
            }

            // Charger Message
            if (bms_can.RxHeader_.Identifier == 0x18FF50E5)
            {
                // update charger last msg time
                bms_can.mainboard->charger_last_msg_time = HAL_GetTick();

                // big endian
                bms_can.mainboard->charger_status = bms_can.rxData_[4];
                uint16_t charger_raw_voltage = (bms_can.rxData_[0] << 8) | (bms_can.rxData_[1] & 0xFF);
                uint16_t charger_raw_current = (bms_can.rxData_[2] << 8) | (bms_can.rxData_[3] & 0xFF);
                bms_can.mainboard->charger_voltage = ((float)charger_raw_voltage) * 0.1;
                bms_can.mainboard->charger_current = ((float)charger_raw_current) * 0.1;
            }
        }
        else
        {
            printf("CAN RX Error");
		    return;
        }
	}
}

void Drive_Can_Loop()
{
	// printf("Sending Drive CAN\n");

	// update and send soc
	populate_bms_soc(bms_can.txDataSOC_);
	send_can_messages(bms_can.mainboard->hcan_drive, &bms_can.TxHeaderSOC_, bms_can.txDataSOC_, &bms_can.TxMailBox_);

	// update and send faults
	populate_bms_faults(bms_can.txDataFaults_);
	send_can_messages(bms_can.mainboard->hcan_drive, &bms_can.TxHeaderFaults_, bms_can.txDataFaults_, &bms_can.TxMailBox_);

	// update and send status
	populate_bms_status(bms_can.txDataStatus_);
	send_can_messages(bms_can.mainboard->hcan_drive, &bms_can.TxHeaderStatus_, bms_can.txDataStatus_, &bms_can.TxMailBox_);
}

uint8_t send_can_messages(CAN_HandleTypeDef *hcan, CAN_TxHeaderTypeDef *TxHeader, uint8_t *data, uint32_t *TxMailBox)
{
	// send msg
	HAL_StatusTypeDef msg_status = HAL_CAN_AddTxMessage(hcan, TxHeader, data, TxMailBox);

	if (msg_status != HAL_OK)
	{
		// Error handling
//		printf("CAN Message failed\n");
		return 1;
	}
	return 0;
}

void Data_Can_Loop()
{
	// send voltage messages
	bms_can.TxHeaderVoltages_.StdId = 0x153; // set the message id for next iteration
	for(int i = 0; i < NUM_DATA_CAN_VOLTAGE_MSGS; i++) {
		populate_bms_voltages(bms_can.txDataVoltages_, i);
		send_can_messages(bms_can.mainboard->hcan_data, &bms_can.TxHeaderVoltages_, bms_can.txDataVoltages_, &bms_can.TxMailBox_);
		bms_can.TxHeaderVoltages_.StdId++;
	}

	// send temperature messages
	bms_can.TxHeaderTemperatures_.StdId = 0x167; // set the message id for next iteration
	for(int i = 0; i < NUM_DATA_CAN_TEMP_MSGS; i++) {
		populate_bms_temparatures(bms_can.txDataTemperatures_, i);
		send_can_messages(bms_can.mainboard->hcan_data, &bms_can.TxHeaderTemperatures_, bms_can.txDataTemperatures_, &bms_can.TxMailBox_);
		bms_can.TxHeaderTemperatures_.StdId++;
	}
}

void populate_bms_soc(uint8_t *data)
{
	RawCanSignal signals[5];
	populateRawMessage(&signals[0], 0, 12, 0.1, 0);									  // max discharge current
	populateRawMessage(&signals[1], 0, 12, 0.1, 0);									  // max regen current
	populateRawMessage(&signals[2], bms_can.mainboard->adbms.total_v, 16, 0.01, 0);   // battery voltage
	populateRawMessage(&signals[3], bms_can.mainboard->adbms.avg_temp, 8, 1, -40);    // battery temp
	populateRawMessage(&signals[4], bms_can.mainboard->current, 16, 0.01, 0);		  // battery current
	encodeSignals(data, 5, signals[0], signals[1], signals[2], signals[3], signals[4]);
}

void populate_bms_faults(uint8_t *data)
{
	RawCanSignal signals[8];
	populateRawMessage(&signals[0], bms_can.mainboard->bms_fault, 1, 1, 0);																  	// fault summary
	populateRawMessage(&signals[1], bms_can.mainboard->adbms.undervoltage_fault_, 1, 1, 0);												  	// undervoltage fault
	populateRawMessage(&signals[2], bms_can.mainboard->adbms.overvoltage_fault_, 1, 1, 0);												   	// overvoltage fault
	populateRawMessage(&signals[3], bms_can.mainboard->adbms.undertemperature_fault_, 1, 1, 0);												// undertemp fault
	populateRawMessage(&signals[4], bms_can.mainboard->adbms.overtemperature_fault_, 1, 1, 0);											 	// overemp fault
	populateRawMessage(&signals[5], bms_can.mainboard->overcurrent_fault, 1, 1, 0);														 	// overcurrent fault
	populateRawMessage(&signals[6], bms_can.mainboard->external_fault, 1, 1, 0);													   			// external fault
	populateRawMessage(&signals[7], (bms_can.mainboard->adbms.openwire_fault_ || bms_can.mainboard->adbms.openwire_temp_fault_), 1, 1, 0);	// open wire fault
	encodeSignals(data, 8, signals[0], signals[1], signals[2], signals[3], signals[4], signals[5], signals[6], signals[7]);
}

void populate_bms_status(uint8_t *data)
{
	RawCanSignal signals[7];

	populateRawMessage(&signals[0], 0, 8, 1, 0);		 // BMS State
	populateRawMessage(&signals[1], bms_can.mainboard->imd_status, 8, 1, 0);		 // IMD State
	populateRawMessage(&signals[2], bms_can.mainboard->adbms.max_temp, 8, 1, -40);   // max cell temp
	populateRawMessage(&signals[3], bms_can.mainboard->adbms.min_temp, 8, 1, -40);   // min cell temp
	populateRawMessage(&signals[4], bms_can.mainboard->adbms.max_v, 8, 0.012, 2);	 // max cell voltage
	populateRawMessage(&signals[5], bms_can.mainboard->adbms.min_v, 8, 0.012, 2);	 // min cell voltage
	populateRawMessage(&signals[6], 0, 8, 0.5, 0);									 // BMS SOC
	encodeSignals(data, 7, signals[0], signals[1], signals[2], signals[3], signals[4], signals[5], signals[6]);
}


void populate_bms_voltages(uint8_t *data, int volt_msg_num)
{
	RawCanSignal signals[8];
	for(int i = 0; i < NUM_DATA_CAN_VOLTAGES_PER_MSG; i++){
		populateRawMessage(&signals[i], bms_can.mainboard->adbms.voltages[volt_msg_num * NUM_DATA_CAN_VOLTAGES_PER_MSG + i], 8, 0.012, 2);
	}
	populateRawMessage(&signals[7], 0, 8, 0.004, 0);	// OCV msg that is legacy from BQ code and only included for backwards compatibility
	// num_per_msg + 1 because includes the added OCV msg
	encodeSignals(data, NUM_DATA_CAN_VOLTAGES_PER_MSG+1, signals[0], signals[1], signals[2], signals[3], signals[4], signals[5], signals[6], signals[7]);
}

void populate_bms_temparatures(uint8_t *data, int temp_num)
{
	RawCanSignal signals[8];
	for(int i = 0; i < NUM_DATA_CAN_TEMPS_PER_MSG; i++){
		populateRawMessage(&signals[i], bms_can.mainboard->adbms.temperatures[temp_num * NUM_DATA_CAN_TEMPS_PER_MSG + i], 8, 1, -40);
	}
	encodeSignals(data, NUM_DATA_CAN_TEMPS_PER_MSG, signals[0], signals[1], signals[2], signals[3], signals[4], signals[5], signals[6], signals[7]);
}