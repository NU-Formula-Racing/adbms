#ifndef ADBMS_MAIN_STRUCT_H
#define ADBMS_MAIN_STRUCT_H

//new version of interface inc file
#include "adbms_interface_v2.h"

//new includes
#include "adbms_interface_2950.h"
#include "adbms_interface_6830.h"

#include "virtual_timer.h"
#include "stm32h7xx_it.h"
#include "stm32h7xx_hal.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "bms_system_prams.h"


typedef struct
{
	// AD Chips: changes in this version
    adbms_raw_ adbms_raw;
    adbms_2950_ adbms_2950;
    adbms_6830_ adbms_6830;
	

	// Timer Group
	timer_group_ *tg;
	timer_group_ *tg2;

	// FDCAN handle
	FDCAN_HandleTypeDef *hcan;

	// faults
	bool external_fault;
	bool bms_fault;

	// current
	// float current;
	float current_offset;

    //this is now in 2950 faults
	//bool overcurrent_fault;

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
