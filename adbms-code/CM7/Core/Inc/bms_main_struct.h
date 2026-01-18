#ifndef ADBMS_MAIN_STRUCT_H
#define ADBMS_MAIN_STRUCT_H

#include "adbms_interface.h"
#include "virtual_timer.h"
#include "stm32h7xx_it.h"
#include "stm32h7xx_hal.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "bms_system_prams.h"


typedef struct
{
	// AD Chips
	adbms_ adbms;

	// Timer Group
	timer_group_ *tg;
	timer_group_ *tg2;

	// FDCAN handle
	FDCAN_HandleTypeDef *hcan;

	// faults
	bool external_fault;
	bool bms_fault;

	// current
	float current;
	float current_offset;
	bool overcurrent_fault;

	// external values
	bool shutdown_present;
	bool imd_status;
	float Inverter_DC_Voltage;
	bool ecu_close_contactors;

	//Last ECU command was valid
	bool ecu_valid_command;
	
	// chrager
	bool charger_status;
	float charger_voltage;
	float charger_current;

	// soc
	float soc;
	float dcir;
	float max_discharge_current;
	float prev_time;

	// timeouts
	float ecu_last_msg_time;
	float inverter_last_msg_time;
	float charger_last_msg_time;

	bool ecu_timeout;
	bool inverter_timeout;
	bool charger_timeout;

	bool timeout_fault;

	bool comms_6822_state;
	uint32_t start_time;
	enum bms_states state;
	enum Charging_states charging_state;
	
} mainboard_;


#endif // ADBMS_MAIN_STRUCT_H
