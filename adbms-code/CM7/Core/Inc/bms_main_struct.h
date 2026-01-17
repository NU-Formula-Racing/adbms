#ifndef ADBMS_MAIN_STRUCT_H
#define ADBMS_MAIN_STRUCT_H

#include "adbms_interface.h"
#include "current_driver.h"
#include "virtual_timer.h"
#include "stm32f4xx_it.h"
#include "stm32f4xx_hal.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "main.h"
#include "control.h"


typedef struct
{
	// AD Chips
	adbms_ adbms;

	// Timer Group
	timer_group_ *tg;
	timer_group_ *tg2;

	// handles
	ADC_HandleTypeDef *hadc;
	CAN_HandleTypeDef *hcan_drive;
	CAN_HandleTypeDef *hcan_data;

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
	enum ECU_commands ecu_command;

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
	bool timeout_fault;
	messege_condition ecu_messege;
	messege_condition inverter_messege;
	messege_condition charger_messege;

	bool comms_6822_state;
	uint32_t start_time;
	enum bms_states state;
	enum Charging_states charging_state;
	
} mainboard_;

typedef struct
{
	float last_msg_time;
	bool fault;
}message_condition;


#endif // ADBMS_MAIN_STRUCT_H