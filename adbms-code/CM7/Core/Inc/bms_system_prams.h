#ifndef AD_SYSTEM_PARAMS_H
#define AD_SYSTEM_PARAMS_H

#define NUM_6830  10
#define NUM_2950  1
#define POSITION_2950 1					/*Position of the 2950 chip (0 is beginning of daisy chain and 1 is end of daisy chain) */
#define POSITION_6830 (POSITION_2950 ^ 1) /*Position of the 2950 chip (0 is beginning of daisy chain and 1 is end of daisy chain) */
#define NUM_CHIPS (NUM_6830+NUM_2950)	/* Number of ICs 					*/
#define NUM_VOLTAGES_EVEN_CHIP 12		/* Nmber of Cells in even number ICs starting at 0 ending at 8*/
#define NUM_VOLTAGES_ODD_CHIP  11  	   	/* Nmber of Cells in odd number ICs starting at 1 ending at 9*/
#define NUM_VOLTAGES (NUM_6830 * NUM_VOLTAGES_ODD_CHIP + ((NUM_6830 + 1)/2))
#define NUM_TEMPS_CHIP 10				/* Number of Temps to read per IC 	*/
#define NUM_TEMPS (NUM_6830 * NUM_TEMPS_CHIP)

#define OVERVOLTAGE 4.2					/* Overvoltage Threshold 	*/
#define UNDERVOLTAGE 2.5				/* Undervoltage Threshold 	*/

#define OVERTEMP 60						/* Over Temp Threshold 		*/
#define UNDERTEMP -20					/* Under Temp Threshold 	*/

// FTTI values
#define FTTI_VOLTAGE_TH_1 0.05
#define FTTI_VOLTAGE_TH_1_COUNTS 500
#define FTTI_VOLTAGE_TH_2 0.01
#define FTTI_VOLTAGE_TH_2_COUNTS 50


#define PEC_FAILURE_THRESHOLD 10		/* Number of consecutive PEC failures that can happen before a pec fault occurs*/
#define OWC_VOLTAGE_THRESHOLD 0.5

#define MAX_CHARGER_VOLTAGE 480
#define MAX_CHARGER_CURRENT 4
#define CB_THRESHOLD 0.01					/* Threshold Away From Lowest Cell to Start Cell Balancing when CB is enabled */
#define CB_MIN_V_THRESHOLD 3			/* Cells under this threshold will not be balanced even when CB is enabled */

#define ENABLE_PRINTF_DEBUG_COMMS 0		/* Flag to enable Printf debug comms */
#define ENABLE_USB_COMMS 0				/* Flag to enable USB comms */
#define ENABLE_SD_LOGGING_BIN 0			/* Flag to enable logging to SD Card as binary files */
#define ENABLE_SD_LOGGING_CSV 0			/* Flag to enable logging to SD Card as CSV files (ASCII)*/

#define NUM_CAN_VOLTAGES_PER_MSG 4	 	// will break if this is changed. Based on how DBC is set up
#define NUM_CAN_VOLTAGE_MSGS  (1 + ((NUM_VOLTAGES - 1) / NUM_CAN_VOLTAGES_PER_MSG))
#define NUM_CAN_VOLTAGE_MSG_SENT_PER_ITTR 6
#define NUM_CAN_TEMPS_PER_MSG 8    // will break if this is changed. Based on how DBC is set up
#define NUM_CAN_TEMP_MSGS (1 + ((NUM_TEMPS - 1) / NUM_CAN_TEMPS_PER_MSG))
#define NUM_CAN_TEMP_MSG_SENT_PER_ITTR 1
#define NUM_CAN_OWC_MSGS (1 + ((NUM_VOLTAGES - 1) / 64))

#define NUM_TIMERS 2    // will break if this is changed. This will be replaced by RTOS

#define INVERTER_VOLTAGE_THRESHOLD 0.9 
#define CHARGER_VOLTAGE_THRESHOLD 0.9 

// CAN TIMEOUTS (ms)
#define VCU_CAN_TIMEOUT 5000
#define INVERTER_CAN_TIMEOUT 5000
#define CHARGER_CAN_TIMEOUT 5000

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
#define MIN_6830_ADC_READING ((int16_t)0x8000)	
#define FAULT_6830_ADC_READING ((int16_t)0xFFFF)									  /* From the datasheet as the min mapping for the adcs */

//for SOC calculations
#define CELL_CAPACITY 4.876
#define PARALLEL 3
#define SYSTEM_CAPACITY (CELL_CAPACITY * PARALLEL)

//CAN IDs
#define BMS_Packboard_ID 0x150
#define BMS_Daughterboard_ID 0x151
#define BMS_Status_ID 0x152
#define CHARGER_ID 0x1806E5F4
#define BMS_OWC_C 0x140
#define BMS_OWC_S 0x142
#define VOLTAGES_ID 0x153
#define TEMPS_ID 0x170

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
