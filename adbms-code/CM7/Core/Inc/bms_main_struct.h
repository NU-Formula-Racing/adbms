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
	bool imd_status; //high is healthy
	float Inverter_DC_Voltage;
	
	// chrager
	uint8_t charger_status;
	float charger_voltage;
	float charger_current;

	// soc
	float soc;
	float dcir;
	float max_discharge_current;
	float prev_time;
	float vr1;          //voltage across RC
	float P[2][2];      //covariance Matrix

	// timeouts
	float vcu_last_msg_time;
	float inverter_last_msg_time;
	float charger_last_msg_time;

	bool vcu_timeout;
	bool inverter_timeout;
	bool charger_timeout;

	bool timeout_fault;

	bool comms_6822_state;
	uint32_t start_time;
	
	enum bms_states internal_state;
	enum vcu_states vcu_state_request;
	enum charging_states charging_state;
	
} mainboard_;


#endif // ADBMS_MAIN_STRUCT_H
