//new include header
#include "bms_can.h"

bms_can_ bms_can;

void Bms_Initialize_Can(mainboard_ *mainboard)
{
	// Add mainboard
	bms_can.mainboard = mainboard;
	
	// assign interrupt line to rx fifo
	HAL_FDCAN_ConfigInterruptLines(bms_can.mainboard->hcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, FDCAN_INTERRUPT_LINE0);

	// activate interrupt line for rx fifo
	HAL_FDCAN_ActivateNotification(bms_can.mainboard->hcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);

	HAL_FDCAN_Start(bms_can.mainboard->hcan);

	// Packboard header initialization
    bms_can.TxHeaderPackboard_.Identifier = BMS_Packboard_ID;
    bms_can.TxHeaderPackboard_.IdType = FDCAN_STANDARD_ID;
    bms_can.TxHeaderPackboard_.TxFrameType = FDCAN_DATA_FRAME;
    bms_can.TxHeaderPackboard_.DataLength = FDCAN_DLC_BYTES_8;
    bms_can.TxHeaderPackboard_.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    bms_can.TxHeaderPackboard_.BitRateSwitch = FDCAN_BRS_OFF;
    bms_can.TxHeaderPackboard_.FDFormat = FDCAN_CLASSIC_CAN;
    bms_can.TxHeaderPackboard_.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    bms_can.TxHeaderPackboard_.MessageMarker = 0;

	// Daughterboard header initialization
	bms_can.TxHeaderDaughterboard_.Identifier = BMS_Daughterboard_ID;
	bms_can.TxHeaderDaughterboard_.IdType = FDCAN_STANDARD_ID;
	bms_can.TxHeaderDaughterboard_.TxFrameType = FDCAN_DATA_FRAME;
	bms_can.TxHeaderDaughterboard_.DataLength = FDCAN_DLC_BYTES_8;
  	bms_can.TxHeaderDaughterboard_.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
 	bms_can.TxHeaderDaughterboard_.BitRateSwitch = FDCAN_BRS_OFF;
   	bms_can.TxHeaderDaughterboard_.FDFormat = FDCAN_CLASSIC_CAN;
   	bms_can.TxHeaderDaughterboard_.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
 	bms_can.TxHeaderDaughterboard_.MessageMarker = 0;

	// Status header initialization
	bms_can.TxHeaderStatus_.Identifier = BMS_Status_ID;
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
    bms_can.TxHeaderCharger_.IdType = FDCAN_EXTENDED_ID;
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

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hcan, uint32_t RxFifo0ITs)
{
	
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0)
    {

       // Get the message
       if (HAL_FDCAN_GetRxMessage(hcan, FDCAN_RX_FIFO0, &bms_can.RxHeader_, bms_can.rxData_) == HAL_OK)
       {
           // VCU Message
           if (bms_can.RxHeader_.Identifier == 0x205)
           {
               // update vcu last msg time
               bms_can.mainboard->vcu_last_msg_time = HAL_GetTick();

               bms_can.mainboard->vcu_state_request = bms_can.rxData_[0];
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
			   if (bms_can.mainboard->internal_state == Idle)
			   {
					bms_can.mainboard->internal_state = Charging;
			   }
           }
       }
       else
       {
           printf("CAN RX Error");
		    return;
       }
    }
}


void Can_Loop()
{
	// update and send packboard
	populate_bms_packboard(bms_can.txDataPackboard_);
	send_can_messages(bms_can.mainboard->hcan, &bms_can.TxHeaderPackboard_, bms_can.txDataPackboard_);

	// update and send daughterboard
	populate_bms_daughterboard(bms_can.txDataDaughterboard_);
	send_can_messages(bms_can.mainboard->hcan, &bms_can.TxHeaderDaughterboard_, bms_can.txDataDaughterboard_);

	// update and send status
	populate_bms_status(bms_can.txDataStatus_);
	send_can_messages(bms_can.mainboard->hcan, &bms_can.TxHeaderStatus_, bms_can.txDataStatus_);

	// populate and send voltage messages
	uint8_t num_voltge_msgs_sent = NUM_CAN_VOLTAGE_MSG_SENT_PER_ITTR;
	if (bms_can.mainboard->internal_state == Fault) num_voltge_msgs_sent = NUM_CAN_VOLTAGE_MSGS;
	
	for(int i = 0; i < num_voltge_msgs_sent; i++) 
	{
		populate_bms_voltages(bms_can.txDataVoltages_, bms_can.current_voltage_msg);
		send_can_messages(bms_can.mainboard->hcan, &bms_can.TxHeaderVoltages_, bms_can.txDataVoltages_);
		bms_can.TxHeaderVoltages_.Identifier++;
		bms_can.current_voltage_msg++;

		if (bms_can.current_voltage_msg >= NUM_CAN_VOLTAGE_MSGS)
		{
			bms_can.TxHeaderVoltages_.Identifier = VOLTAGES_ID;
			bms_can.current_voltage_msg = 0;
		}
	}


	// populate and send temperature messages
	uint8_t num_temp_msgs_sent = NUM_CAN_TEMP_MSG_SENT_PER_ITTR;
	if (bms_can.mainboard->internal_state == Fault) num_temp_msgs_sent = NUM_CAN_TEMP_MSGS;
	
	for(int i = 0; i < num_temp_msgs_sent; i++) {
		populate_bms_temparatures(bms_can.txDataTemperatures_, bms_can.current_temp_msg);
		send_can_messages(bms_can.mainboard->hcan, &bms_can.TxHeaderTemperatures_, bms_can.txDataTemperatures_);
		bms_can.TxHeaderTemperatures_.Identifier++;
		bms_can.current_temp_msg++;

		if (bms_can.current_temp_msg >= NUM_CAN_TEMP_MSGS)
		{
			bms_can.TxHeaderTemperatures_.Identifier = TEMPS_ID;
			bms_can.current_temp_msg = 0;
		}
	}

	if (bms_can.mainboard->internal_state == Charging)
	{
		populateCharger_Msg(bms_can.txDataCharger_);
		send_can_messages(bms_can.mainboard->hcan, &bms_can.TxHeaderCharger_, bms_can.txDataCharger_);
	}
}

uint8_t send_can_messages(FDCAN_HandleTypeDef *hcan, FDCAN_TxHeaderTypeDef *TxHeader, uint8_t *data)
{	
	// send msg
	HAL_StatusTypeDef msg_status = HAL_FDCAN_AddMessageToTxFifoQ(hcan, TxHeader, data);

	if (msg_status != HAL_OK)
	{
		// Error handling
		//printf("CAN Message failed\n");
		return 1;
	}
	return 0;
}

void populate_bms_packboard(uint8_t *data)
{
	RawCanSignal signals[2];
	populateRawMessage(&signals[0], bms_can.mainboard->adbms_2950.data.current, 32, 0.0001, -100);	// battery current
	populateRawMessage(&signals[1], bms_can.mainboard->adbms_2950.data.precontactor_voltage, 32, 0.0001, 0);// max regen current
	encodeSignals(data, 2, signals[0], signals[1]);
}

void populate_bms_daughterboard(uint8_t *data)
{
	RawCanSignal signals[4];
	populateRawMessage(&signals[0], bms_can.mainboard->adbms_6830.voltage.total_v, 16, 0.01, 0);   		 // total voltage
	populateRawMessage(&signals[1], bms_can.mainboard->adbms_6830.voltage.max_v, 16, 0.00015, -3.4152);	 // max cell voltage
	populateRawMessage(&signals[2], bms_can.mainboard->adbms_6830.voltage.min_v, 16, 0.00015, -3.4152);	 // min cell voltage
	populateRawMessage(&signals[3], bms_can.mainboard->adbms_6830.temperature.max_temp, 16, 0.01, -20);   	 // max cell temp
	encodeSignals(data, 4, signals[0], signals[1], signals[2], signals[3]);

}

void populate_bms_status(uint8_t *data)
{
	RawCanSignal signals[17];

	// states faults summary
	populateRawMessage(&signals[0], bms_can.mainboard->soc, 16, 0.00002, 0);		// SOC
	populateRawMessage(&signals[1], bms_can.mainboard->internal_state, 8, 1, 0);	// BMS state
	populateRawMessage(&signals[2], bms_can.mainboard->imd_status, 1, 1, 0);		// IMD status
	populateRawMessage(&signals[3], bms_can.mainboard->bms_fault, 1, 1, 0);			// InternalFault_Summary

    // faults
	populateRawMessage(&signals[4], bms_can.mainboard->adbms_6830.faults.undervoltage_fault, 1, 1, 0);		// undervoltage fault
	populateRawMessage(&signals[5], bms_can.mainboard->adbms_6830.faults.overvoltage_fault, 1, 1, 0);		// overvoltage fault
	populateRawMessage(&signals[6], bms_can.mainboard->adbms_6830.faults.undertemperature_fault, 1, 1, 0);	// undertemp fault
	populateRawMessage(&signals[7], bms_can.mainboard->adbms_6830.faults.overtemperature_fault, 1, 1, 0);	// overemp fault
	populateRawMessage(&signals[8], bms_can.mainboard->adbms_6830.faults.openwire_voltage_fault, 1, 1, 0);	// open wire fault
	populateRawMessage(&signals[9], bms_can.mainboard->adbms_6830.faults.openwire_temp_fault, 1, 1, 0);		// open wire temp fault
	populateRawMessage(&signals[10], bms_can.mainboard->adbms_6830.faults.pec_fault, 1, 1, 0);				// Pec Fault

	// external warnings
	populateRawMessage(&signals[11], !bms_can.mainboard->shutdown_present, 1, 1, 0);	// shutdown open
	populateRawMessage(&signals[12], bms_can.mainboard->vcu_timeout, 1, 1, 0);			// vcu timeout
	populateRawMessage(&signals[13], bms_can.mainboard->inverter_timeout, 1, 1, 0);	// inverter timeout
	populateRawMessage(&signals[14], bms_can.mainboard->charger_timeout, 1, 1, 0);		// charger timeout

	// 2950 warnings
	populateRawMessage(&signals[15], 0, 1, 1, 0);		// 2950 pec warning

	// total pec failures
	populateRawMessage(&signals[16], bms_can.mainboard->adbms_6830.faults.total_pec_failures, 16, 1, 0);

	encodeSignals(data, 17, signals[0], signals[1], signals[2], signals[3], signals[4], signals[5], signals[6], signals[7], signals[8], signals[9], signals[10], signals[11], signals[12], signals[13], signals[14], signals[15], signals[16]);
}


void populate_bms_voltages(uint8_t *data, int volt_msg_num)
{
	RawCanSignal signals[NUM_CAN_VOLTAGES_PER_MSG];
	for(int i = 0; i < NUM_CAN_VOLTAGES_PER_MSG; i++){
		if ((volt_msg_num * NUM_CAN_VOLTAGES_PER_MSG + i) >= NUM_VOLTAGES)
		{
			populateRawMessage(&signals[i], 0, 16, 0, 0);	// out of voltage range so send 0s
		}
		else
		{	
			populateRawMessage(&signals[i], bms_can.mainboard->adbms_6830.voltage.voltages[volt_msg_num * NUM_CAN_VOLTAGES_PER_MSG + i], 16, 0.00015, -3.4152);
		}
	}
	encodeSignals(data, NUM_CAN_VOLTAGES_PER_MSG, signals[0], signals[1], signals[2], signals[3]);
}

void populate_bms_temparatures(uint8_t *data, int temp_msg_num)
{
	RawCanSignal signals[NUM_CAN_TEMPS_PER_MSG];
	for(int i = 0; i < NUM_CAN_TEMPS_PER_MSG; i++){
		if ((temp_msg_num * NUM_CAN_TEMPS_PER_MSG + i) >= NUM_TEMPS)
		{
			populateRawMessage(&signals[i], 0, 8, 0, 0);	// out of temp range so send 0s
		}
		else
		{	
			populateRawMessage(&signals[i], bms_can.mainboard->adbms_6830.temperature.temperatures[temp_msg_num * NUM_CAN_TEMPS_PER_MSG + i], 8, 0.5, -20);
		}
	}
	encodeSignals(data, NUM_CAN_TEMPS_PER_MSG, signals[0], signals[1], signals[2], signals[3], signals[4], signals[5], signals[6], signals[7]);
}

void populateCharger_Msg(uint8_t *data)
{
	uint16_t voltage = MAX_CHARGER_VOLTAGE * 10;
	uint16_t current = MAX_CHARGER_CURRENT * 10;
	data[0] = (voltage >> 8) & 0xFF;
	data[1] = voltage & 0xFF;
	data[2] = (current >> 8) & 0xFF;
	data[3] = current & 0xFF;
	if (bms_can.mainboard->internal_state == Charging && bms_can.mainboard->charging_state == charger_active )
	{
		data[4] = 0;
	}
	else
	{
		data[4] = 1;
	}
	 
}