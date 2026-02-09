#ifndef AD_SYSTEM_PARAMS_H
#define AD_SYSTEM_PARAMS_H

// Note for NFR25 we precharged across the negative contactor. 
// This might change in future years, if so change which contactors are closed accoringly in the FSM 

#define NUM_CHIPS 2				    /* Number of ICs 					*/
#define NUM_VOLTAGES_EVEN_CHIP 12		/* Nmber of Cells in even number ICs starting at 0 ending at 8*/
#define NUM_VOLTAGES_ODD_CHIP  11  	   	/* Nmber of Cells in odd number ICs starting at 1 ending at 9*/
#define NUM_TEMPS_CHIP 10				/* Number of Temps to read per IC 	*/

#define OVERVOLTAGE 4.2					/* Overvoltage Threshold 	*/
#define UNDERVOLTAGE 2.5				/* Undervoltage Threshold 	*/
#define OVERTEMP 90						/* Over Temp Threshold 		*/
#define UNDERTEMP -40					/* Under Temp Threshold 	*/
#define OVERCURRENT 135					/* Over Current Threshold 	*/

#define PEC_FAILURE_THRESHOLD 10		/* Number of consecutive PEC failures that can happen before a pec fault occurs*/

#define CB_THRESHOLD 0.01				/* Threshold Away From Lowest Cell to Start Cell Balancing when CB is enabled */
#define CB_MIN_V_THRESHOLD 3			/* Cells under this threshold will not be balanced even when CB is enabled */

#define ENABLE_PRINTF_DEBUG_COMMS 1		/* Flag to enable Printf debug comms */
#define ENABLE_USB_COMMS 1				/* Flag to enable USB comms */
#define ENABLE_SD_LOGGING_BIN 0			/* Flag to enable logging to SD Card as binary files */
#define ENABLE_SD_LOGGING_CSV 1			/* Flag to enable logging to SD Card as CSV files (ASCII)*/


#define NUM_CURRENT_OFFSET_CYCLES 10	/* Numbeer of Cycles to get ADC Offset */

#define NUM_DATA_CAN_VOLTAGES_PER_MSG 7 // will break if this is changed. Based on how data can and DBC are set up
#define NUM_DATA_CAN_VOLTAGE_MSGS (NUM_CHIPS * NUM_VOLTAGES_ODD_CHIP + ((NUM_CHIPS + 1)/2)) / NUM_DATA_CAN_VOLTAGES_PER_MSG
#define NUM_DATA_CAN_TEMPS_PER_MSG 8    // will break if this is changed. Based on how data can and DBC are set up
#define NUM_DATA_CAN_TEMP_MSGS (NUM_CHIPS * NUM_TEMPS_CHIP) / NUM_DATA_CAN_TEMPS_PER_MSG

#define MAX_CHARGER_VOLTAGE 568
#define MAX_CHARGER_CURRENT 3

// BMS IC Parameters
#define CELLS 	16														  /* Bms ic number of Cells                */
#define CELL_REG_GRP 6													  /*Number of total voltage register groups (A-F)*/
#define AUX		12														  /* Bms ic number of Aux             	   */
#define AUX_GPIO 10
#define AUX_REG_GRP 4
#define VOLTAGES_REG_GRP 3												  /* Number of voltages a Register Group can represent*/
#define CMD_LEN  2                                                        /* Number of CMD Bytes                   */
#define DATA_LEN 6                                                        /* Number of Data Bytes in Register Groups*/
#define PEC_LEN  2                                                        /* Number of PEC Bytes                   */
#define DATABUF_LEN (CMD_LEN + PEC_LEN) + (DATA_LEN + PEC_LEN)*NUM_CHIPS  /* CMD Msg + PEC and (DATA + PEC) per IC */

#define NUM_TIMERS 2    // will break if this is changed. This will be replaced by RTOS
#define NUM_CURRENT_OFFSET_CYCLES 10

#define INVERTER_VOLTAGE_THRESHOLD 0.9 
#define CHARGER_VOLTAGE_THRESHOLD 0.75 

// CAN TIMEOUTS (ms)
#define VCU_CAN_TIMEOUT 5000
#define INVERTER_CAN_TIMEOUT 5000
#define CHARGER_CAN_TIMEOUT 5000

//for SOC calculations
#define CELL_CAPACITY 4.5
#define PARALLEL 3
#define SYSTEM_CAPACITY (CELL_CAPACITY * PARALLEL)

//CAN IDs
#define SOC_ID 0x150
#define FAULT_ID 0x151
#define STATUS_ID 0x152
#define CHARGER_ID 0x1806E5F4
#define VOLTAGES_ID 0x153
#define TEMPS_ID 0x167

// ENUMERATES
enum bms_states
{
	Idle = 0,
	Precharge = 1,
	Active = 2,
	Charging = 3, 
	Fault = 4
};

enum vcu_states
{
	car_idle = 0,
	car_precharge = 1,
	car_neutral = 2,
	car_drive = 3,
	car_fault = 4
};

enum charging_states
{
	charger_setup = 0,
	charger_precharge = 1,
	charger_active = 2,
};

#endif // AD_SYSTEM_PARAMS_H
