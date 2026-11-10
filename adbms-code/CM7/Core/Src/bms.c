#include "bms.h"

mainboard_ mainboard;

void Bms_Mainboard_Setup(SPI_HandleTypeDef *hspi, FDCAN_HandleTypeDef *hcan)
{
	// initialize handles
	mainboard.hcan = hcan;

	// initialize ad chip;
	ADBMS_Initialize(&mainboard.adbms_raw, hspi);
	// initialize CAN;
	Bms_Initialize_Can(&mainboard);

	// initialize SOC
	Soc_Initialize(&mainboard);

	// initialize the timers: adbms_mainboard_loop, drive_can, data_can
	timer_ t_adbms = Create_Timer(19, bms_mainboard_loop); // bc of timer group delay is +1ms of the duration
	timer_ t_adbms_owc_check = Create_Timer(OWC_PERIOD_MS, adbms_owc_loop);
	timer_ timers[NUM_TIMERS] = {t_adbms, t_adbms_owc_check};
	mainboard.tg = Create_Timer_Group(timers);

	mainboard.start_time = HAL_GetTick();

	//initial states
	mainboard.charging_state = charger_setup;
	mainboard.internal_state = Idle;
	mainboard.imd_status = true; //start healthy because fault is latched (maybe not needed in software)
}


void Tick_Mainboard_Timers()
{
	Tick_Timer_Group(mainboard.tg);
}

// ---------------------------------------------------------------------------
// TEST ONLY (BALANCE_DIAG_TEST): is the balance current commanded, or is it hardware?
//  1. Read back what the 6830 actually holds: DCC bits / DCTO (CFGB), MUTE_ST (CFGA), PWM duty (PWMA/PWMB)
//  2. Send MUTE (datasheet: disables all discharge) and hold it for BALANCE_DIAG_MUTE_MS
//  3. Send UNMUTE and read back again
// While muted, a healthy chip carries ~0 V across the 22 ohm balance resistors.
// ---------------------------------------------------------------------------
static int balance_diag_stage = 0;	// 0 = waiting, 1 = muted, 2 = done

// What the chip MEASURES across each balance switch, compared with what it was told (expected_dcc, bit0 = cell 1).
// C-ADC = cell voltage (tap to tap). S-ADC = SxP to SxN = directly across the internal discharge FET.
// Switch open:   no current through the 2x22 ohm, so S == C.
// Switch closed: S drops to I * R_switch and I = (C - S) / (2 * 22 ohm).
// Returns the number of channels whose measured state does not match expected_dcc (-1 if the S read failed).
static int balance_switch_table(int cic, uint16_t expected_dcc)	// cic = 0-based chip; bit0 of expected_dcc = that chip's cell 1
{
	SPI_HandleTypeDef *hspi = mainboard.adbms_raw.SPI_data.hspi;
	uint8_t *sbuf = mainboard.adbms_raw.SPI_data.spi_dataBuf;
	static adbms_6830_voltage_parsed_ s_meas;
	memset(&s_meas, 0, sizeof(s_meas));
	ADBMS_Owc_Config(&mainboard.adbms_raw, S_Channel, Channel_Off);	// ADSV, CONT = 1, OW = 0 (plain S-ADC measurement; static DCC discharge keeps running)
	HAL_Delay(25);														// S-ADC results update every 8 ms
	ADBMS_WakeUP_ICs();
	ADBMS_Read_Voltages(&mainboard.adbms_raw.read_raw_s, S_Channel_Read, hspi, sbuf);
	ADBMS_6830_Parse_Voltage(&mainboard.adbms_raw.read_raw_s, &s_meas);
	if (s_meas.pec_counts)
	{
		printf("  S-ADC read failed (%s) - switch check skipped\n", s_meas.adc_off ? "ADC cleared/fault value" : "PEC");
		return -1;
	}

	const float r_bal = 2.0f * BALANCE_DIAG_R_OHMS;
	int n_wrong = 0;
	const int first = CHIP_VOLTAGE_OFFSET(cic), n_cells = CHIP_VOLTAGE_OFFSET(cic + 1) - first;
	printf("  IC%d cell   C(cell) V   S(switch) V   I_bal mA   R_switch ohm   commanded   measured\n", cic + 1);
	for (int i = 0; i < n_cells; i++)
	{
		float c = mainboard.adbms_6830.voltage.voltages[first + i];
		float s = s_meas.voltages[first + i];
		float i_ma = (c - s) / r_bal * 1000.0f;
		bool closed = (c - s) > 0.1f;		// > ~2 mA through the 44 ohm path
		bool want = (expected_dcc >> i) & 1;
		if (closed != want) n_wrong++;
		if (closed)
			printf("  %2d     %7.4f      %7.4f      %7.1f      %7.1f       %-6s      CONDUCTING%s\n", i + 1, c, s, i_ma,
			       s / (i_ma / 1000.0f), want ? "CLOSED" : "open", want ? "" : "   <-- MISMATCH");
		else
			printf("  %2d     %7.4f      %7.4f      %7.1f            -       %-6s      open%s\n", i + 1, c, s, i_ma,
			       want ? "CLOSED" : "open", want ? "         <-- MISMATCH" : "");
	}
	printf("  -> IC%d: %d of %d channels match what was commanded\n", cic + 1, n_cells - n_wrong, n_cells);
	return n_wrong;
}

// Read back the DCC bits 6830 number cic (0-based) actually holds (bit0 = cell 1). Returns 0xFFFF on PEC failure.
static uint16_t read_back_dcc(int cic)
{
	uint8_t buf[NUM_CHIPS * DATA_LEN];
	pec_return_ pec;
	ADBMS_WakeUP_ICs();
	ADBMS_Read_Data(mainboard.adbms_raw.SPI_data.hspi, RDCFGB, buf, mainboard.adbms_raw.SPI_data.spi_dataBuf, &pec);
	uint8_t *r = &buf[(POSITION_6830 * NUM_2950 + cic) * DATA_LEN];
	return pec.pec_6830 ? 0xFFFF : (uint16_t)(r[4] | (r[5] << 8));
}

static void balance_diag_dump(const char *tag)
{
	uint8_t buf[NUM_CHIPS * DATA_LEN];
	pec_return_ pec;
	SPI_HandleTypeDef *hspi = mainboard.adbms_raw.SPI_data.hspi;
	uint8_t *sbuf = mainboard.adbms_raw.SPI_data.spi_dataBuf;
	const uint8_t slot = POSITION_6830 * NUM_2950;	// position of the (first) 6830 in the read buffer
	uint8_t *r = &buf[slot * DATA_LEN];

	ADBMS_WakeUP_ICs();
	printf("\n[BAL DIAG] %s (t=%lu ms)\n", tag, (unsigned long)HAL_GetTick());

	ADBMS_Read_Data(hspi, RDCFGA, buf, sbuf, &pec);
	printf("  CFGA : %02X %02X %02X %02X %02X %02X  PEC %s  -> MUTE_ST=%d (1 = discharge muted)\n",
	       r[0], r[1], r[2], r[3], r[4], r[5], pec.pec_6830 ? "FAIL" : "ok", (r[5] >> 4) & 1);

	ADBMS_Read_Data(hspi, RDCFGB, buf, sbuf, &pec);
	uint16_t dcc = (uint16_t)(r[4] | (r[5] << 8));	// bit0 = cell 1 ... bit15 = cell 16
	printf("  CFGB : %02X %02X %02X %02X %02X %02X  PEC %s  -> DCC=0x%04X DCTO=%d DTMEN=%d DTRNG=%d\n",
	       r[0], r[1], r[2], r[3], r[4], r[5], pec.pec_6830 ? "FAIL" : "ok",
	       dcc, r[3] & 0x3F, (r[3] >> 7) & 1, (r[3] >> 6) & 1);
	printf("         cells with DCC set:");
	if (!dcc) printf(" none");
	for (int i = 0; i < 16; i++) if (dcc & (1 << i)) printf(" %d", i + 1);
	printf("\n");

	ADBMS_Read_Data(hspi, RDPWM1, buf, sbuf, &pec);	// PWM1..PWM12, one nibble each
	printf("  PWMA : %02X %02X %02X %02X %02X %02X  PEC %s\n", r[0], r[1], r[2], r[3], r[4], r[5], pec.pec_6830 ? "FAIL" : "ok");
	ADBMS_Read_Data(hspi, RDPWM2, buf, sbuf, &pec);	// PWM13..PWM16 in the first 2 bytes
	printf("  PWMB : %02X %02X                PEC %s   (all PWM bytes should be 00)\n", r[0], r[1], pec.pec_6830 ? "FAIL" : "ok");

	for (int cic = 0; cic < NUM_6830; cic++) balance_switch_table(cic, 0);	// everything should be open here
}

static void balance_diag_tick()
{
	static uint32_t t_mute = 0, t_last_dump = 0;
	uint32_t now = HAL_GetTick() - mainboard.start_time;
	SPI_HandleTypeDef *hspi = mainboard.adbms_raw.SPI_data.hspi;

	if (balance_diag_stage == 0 && now >= BALANCE_DIAG_START_MS)
	{
		balance_diag_dump("BEFORE MUTE - balancing should read DCC=0 / PWM=0 here");
		ADBMS_WakeUP_ICs();
		ADBMS_Write_CMD(hspi, MUTE);
		balance_diag_dump("AFTER MUTE");
		printf("\n*** DISCHARGE MUTED for %d s: MEASURE THE 22 OHM BALANCE RESISTORS AND PSU CURRENT NOW ***\n"
		       "*** healthy chip: ~0 V across each 22 ohm, PSU ~20 mA (+chip) | still ~1.1 V: FETs not under control ***\n",
		       BALANCE_DIAG_MUTE_MS / 1000);
		t_mute = t_last_dump = now;
		balance_diag_stage = 1;
	}
	else if (balance_diag_stage == 1)
	{
		if (now - t_mute >= BALANCE_DIAG_MUTE_MS)
		{
			ADBMS_WakeUP_ICs();
			ADBMS_Write_CMD(hspi, UNMUTE);
			balance_diag_dump("AFTER UNMUTE (DCC is still 0, so a healthy chip stays at ~0 V)");
			printf("\n*** BALANCE DIAG DONE - normal printing resumes ***\n");
			balance_diag_stage = 2;
		}
		else if (now - t_last_dump >= 5000)
		{
			ADBMS_WakeUP_ICs();
			ADBMS_Write_CMD(hspi, MUTE);	// re-assert in case anything un-muted it
			balance_diag_dump("still muted");
			t_last_dump = now;
		}
	}
}

// ---------------------------------------------------------------------------
// TEST ONLY (CB_TEST): close one balance FET at a time on IC<CB_TEST_IC> so each channel can be checked with a multimeter.
// Waits CB_TEST_START_MS, then closes DIS<CB_TEST_CHANNEL> on that IC (every other channel on every IC open).
// CB_TEST_STEP_MS = 0 -> hold that channel until reflashed; > 0 -> move to the next channel of the same IC every CB_TEST_STEP_MS.
// Expect ~V_cell / (2 x 22 ohm) through the closed channel, ~V_cell/2 across each of its 22 ohm resistors.
// ---------------------------------------------------------------------------
#define CB_TEST_CIC (CB_TEST_IC - 1)	/* 0-based chip under test */
#define CB_TEST_N_CELLS (CHIP_VOLTAGE_OFFSET(CB_TEST_CIC + 1) - CHIP_VOLTAGE_OFFSET(CB_TEST_CIC))
static int cb_test_channel = 0;		// 0 = not started yet, otherwise the 1-based channel (on IC<CB_TEST_IC>) currently closed

static void cb_test_apply(int channel)
{
	for (int cic = 0; cic < NUM_6830; cic++)
		mainboard.adbms_raw.command_parameters.parameter_6830.cfb6830[cic].dcc =
			(cic == CB_TEST_CIC && channel >= 1 && channel <= CB_TEST_N_CELLS) ? (uint16_t)(1u << (channel - 1)) : 0;
	ADBMS_WakeUP_ICs();
	ADBMS_Set_Config_B_6830(mainboard.adbms_raw.command_parameters.parameter_6830.cfb6830,
	                        mainboard.adbms_raw.command_bit.cfg_b, NUM_6830, POSITION_6830);
	ADBMS_Write_Data(mainboard.adbms_raw.SPI_data.hspi, WRCFGB, mainboard.adbms_raw.command_bit.cfg_b,
	                 mainboard.adbms_raw.SPI_data.spi_dataBuf);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_SET);	// CB indication LED
}

// DCC read back from every IC: the IC under test should hold only its one bit, every other IC 0x0000
static void cb_test_print_readback()
{
	printf("  DCC read back:");
	for (int cic = 0; cic < NUM_6830; cic++)
	{
		uint16_t rb = read_back_dcc(cic);
		printf("  IC%d 0x%04X%s", cic + 1, rb, rb == 0xFFFF ? " (PEC FAIL)" : "");
	}
	printf("   (expected IC%d 0x%04X, others 0x0000)\n", CB_TEST_IC, (unsigned)(1u << (cb_test_channel - 1)));
}

static void cb_test_tick()
{
	static uint32_t t_channel = 0, t_status = 0;
	static bool announced = false;
	uint32_t now = HAL_GetTick() - mainboard.start_time;

	if (cb_test_channel == 0)
	{
		if (!announced)
		{
			printf("\n*** CB TEST: closing IC%d DIS%d in %d s (all other channels open) ***\n", CB_TEST_IC, CB_TEST_CHANNEL, CB_TEST_START_MS / 1000);
			announced = true;
		}
		if (now < CB_TEST_START_MS) return;
		cb_test_channel = CB_TEST_CHANNEL;
		t_channel = now;
		t_status = 0;
		cb_test_apply(cb_test_channel);
		HAL_Delay(5);
		printf("\n*** CB TEST: IC%d DIS%d CLOSED ***\n", CB_TEST_IC, cb_test_channel);
		cb_test_print_readback();
		balance_switch_table(CB_TEST_CIC, (uint16_t)(1u << (cb_test_channel - 1)));
		return;
	}

	// optional auto-step to the next channel on the same IC
	if (CB_TEST_STEP_MS > 0 && now - t_channel >= CB_TEST_STEP_MS)
	{
		cb_test_channel = (cb_test_channel % CB_TEST_N_CELLS) + 1;
		t_channel = now;
		cb_test_apply(cb_test_channel);
		HAL_Delay(5);
		printf("\n*** CB TEST: now IC%d DIS%d CLOSED - next channel in %d s ***\n", CB_TEST_IC, cb_test_channel, CB_TEST_STEP_MS / 1000);
		cb_test_print_readback();
		balance_switch_table(CB_TEST_CIC, (uint16_t)(1u << (cb_test_channel - 1)));
		t_status = now;
		return;
	}

	// every 2 s: re-assert the DCC bit (anything that rewrote CFGB would otherwise open it) and print a status line
	if (now - t_status >= 2000)
	{
		cb_test_apply(cb_test_channel);
		float v = mainboard.adbms_6830.voltage.voltages[CHIP_VOLTAGE_OFFSET(CB_TEST_CIC) + cb_test_channel - 1];
		uint16_t rb = read_back_dcc(CB_TEST_CIC);
		printf("CB TEST t=%lus: IC%d DIS%d closed | IC%d DCC read back 0x%04X | cell %d = %.4f V | expect ~%.1f mA, ~%.3f V across each 22 ohm\n",
		       (unsigned long)(now / 1000), CB_TEST_IC, cb_test_channel, CB_TEST_IC, rb, cb_test_channel, v,
		       v / (2.0f * BALANCE_DIAG_R_OHMS) * 1000.0f, v / 2.0f);
		t_status = now;
	}
}

// ADBMS loop that gets ticked
void bms_mainboard_loop()
{
	update_values();
	check_faults();
	Control_Loop(&mainboard);
	Can_Loop();
	
	// Test: once, OC_PIN_TEST_DELAY_MS after boot, drive the selected OCA/OCB pin(s) high
	static bool oc_pin_test_done = false;
	if (OC_PIN_TEST && !oc_pin_test_done && (HAL_GetTick() - mainboard.start_time) >= OC_PIN_TEST_DELAY_MS)
	{
		ADBMS_2950_OC_Pin_Test(&mainboard.adbms_raw, OC_PIN_TEST_OCA, OC_PIN_TEST_OCB);
		oc_pin_test_done = true;
		if (ENABLE_PRINTF_DEBUG_COMMS) printf("\n*** OC pin test: OCA -> %s, OCB -> %s ***\n",
		                                      OC_PIN_TEST_OCA ? "HIGH" : "LOW", OC_PIN_TEST_OCB ? "HIGH" : "LOW");
	}

	if (BALANCE_DIAG_TEST) balance_diag_tick();
	if (CB_TEST) cb_test_tick();

	// keep the terminal readable while the balance diag is muting / the CB test is running
	if(ENABLE_PRINTF_DEBUG_COMMS && balance_diag_stage != 1 && !CB_TEST) send_data_over_printf();

	if(mainboard.internal_state == Fault) HAL_Delay(10);	// delay loop to send values slower in fault
}

// Last open-wire check result, for the periodic printout
static uint32_t owc_last_time  = 0;
static int      owc_last_c_pec = 0;
static int      owc_last_s_pec = 0;
static bool     owc_has_run    = false;

// Seprate loop that gets ticked to run OWC
void adbms_owc_loop()
{
	if (!ENABLE_OWC || NUM_6830 == 0) return;
	if (balance_diag_stage == 1) return;	// don't disturb the balance diag window
	if (CB_TEST) return;					// OWC turns balancing off, which would open the FET under test

	ADBMS_WakeUP_ICs();

	cell_Balance_Off(&mainboard.adbms_raw);

	// Clear the per-channel results so they show this run only.
	// (openwire_voltage_fault and bms_fault stay latched as before.)
	adbms_6830_faults_ *f = &mainboard.adbms_6830.faults;
	memset(f->owc_c_faults, 0, sizeof(f->owc_c_faults));
	memset(f->owc_s_faults, 0, sizeof(f->owc_s_faults));

	Owc_c_channel_update(&mainboard.adbms_raw, &mainboard.adbms_6830);
	owc_last_c_pec = mainboard.adbms_6830.voltage_owc.pec_counts;
	Owc_s_channel_update(&mainboard.adbms_raw, &mainboard.adbms_6830);
	owc_last_s_pec = mainboard.adbms_6830.voltage_owc.pec_counts;
	owc_last_time  = HAL_GetTick();
	owc_has_run    = true;

	if (ENABLE_PRINTF_DEBUG_COMMS)
	{
		printf("\n=== Open wire check ===\n");
		print_owc_results();
	}

	owc_can_loop();
}

// Print the result of the most recent open-wire check (OW voltage < OWC_VOLTAGE_THRESHOLD = open).
// owc_c_faults / owc_s_faults are cleared at the start of each run, so they hold the last run only.
void print_owc_results()
{
	if (!ENABLE_OWC || NUM_6830 == 0) return;
	if (!owc_has_run)
	{
		printf("Last OWC: not run yet (first check at ~%d ms)\n", OWC_PERIOD_MS);
		return;
	}

	adbms_6830_faults_ *f = &mainboard.adbms_6830.faults;
	int n_c = 0, n_s = 0;

	printf("Last OWC (t=%lu ms, %lu ms ago)  C open:", (unsigned long)owc_last_time, (unsigned long)(HAL_GetTick() - owc_last_time));
	for (int i = 0; i < NUM_VOLTAGES; i++) if (f->owc_c_faults[i])
	{
		int cic = ADBMS_6830_Cell_Chip(i);
		printf(" IC%d:C%d", cic + 1, i - CHIP_VOLTAGE_OFFSET(cic) + 1); n_c++;
	}
	printf("%s%s   S open:", n_c ? "" : " none", owc_last_c_pec ? " (PEC FAIL - not updated)" : "");
	for (int i = 0; i < NUM_VOLTAGES; i++) if (f->owc_s_faults[i])
	{
		int cic = ADBMS_6830_Cell_Chip(i);
		printf(" IC%d:S%d", cic + 1, i - CHIP_VOLTAGE_OFFSET(cic) + 1); n_s++;
	}
	printf("%s%s\n", n_s ? "" : " none", owc_last_s_pec ? " (PEC FAIL - not updated)" : "");
}


#if NUM_2950 > 0
// Read back the 2950's CFGA and re-run ADBMS_Initialize if the chip lost its config
// (power-up after the mainboard, or a reset). Same check as the isoSPI bench test:
// OCEN (byte 0 bit 7) and REFUP/ACCI (byte 5 bits 4,2:0) must match what we wrote.
// No re-init on a PEC failure: that just means no packboard is answering.
static void Check_2950_Config(void)
{
	uint8_t buf[NUM_CHIPS * DATA_LEN];
	pec_return_ pec;
	const unsigned slot = (POSITION_2950 * NUM_6830) * DATA_LEN;
	const uint8_t *expected = &mainboard.adbms_raw.command_bit.cfg_a[slot];

	ADBMS_Read_Data(mainboard.adbms_raw.SPI_data.hspi, RDCFGA, buf, mainboard.adbms_raw.SPI_data.spi_dataBuf, &pec);
	if (pec.pec_2950) return;

	const bool config_ok = ((buf[slot + 0] ^ expected[0]) & 0x80U) == 0U
	                    && ((buf[slot + 5] ^ expected[5]) & 0x17U) == 0U;
	if (!config_ok)
	{
		if (ENABLE_PRINTF_DEBUG_COMMS) printf("\n*** ADBMS2950 config lost (packboard reset/plugged in) - reconfiguring ***\n");
		ADBMS_Initialize(&mainboard.adbms_raw, mainboard.adbms_raw.SPI_data.hspi);
		ADBMS_WakeUP_ICs();
	}
}
#endif

void update_values()
{
	//First Read both Voltages and Temps raw values in lower level interface
	ADBMS_WakeUP_ICs();
	
	// turn cb off if charging to read true voltage
	if (mainboard.internal_state == Charging) 
	{
		cell_Balance_Off(&mainboard.adbms_raw);
		ADBMS_Write_CMD(mainboard.adbms_raw.SPI_data.hspi, mainboard.adbms_raw.command_bit.adcv); // clear adc filter
    	HAL_Delay(1); // ADCs are updated at their conversion rate of 1ms
	}

	
#if NUM_6830 > 0
	// 6830-only reads/commands. Skipped with a packboard-only chain so 6830 opcodes
	// (RDFCx, RDAUXx, ADAX) are never sent to the 2950.
	//read voltages
	ADBMS_Read_Voltages(&mainboard.adbms_raw.read_raw_c, C_Channel_Filtered_Read, mainboard.adbms_raw.SPI_data.hspi, mainboard.adbms_raw.SPI_data.spi_dataBuf);
	//Parse 6830 voltage
	ADBMS_6830_Parse_Voltage(&mainboard.adbms_raw.read_raw_c, &mainboard.adbms_6830.voltage);

	// turn cb back on if charging
	if (mainboard.internal_state == Charging) cell_Balance_On(&mainboard.adbms_raw,&mainboard.adbms_6830);


	//read temp
	ADBMS_Read_Voltages(&mainboard.adbms_raw.read_raw_aux, AUX_Read, mainboard.adbms_raw.SPI_data.hspi, mainboard.adbms_raw.SPI_data.spi_dataBuf);
	//RESTART ADAX
	ADBMS_Write_CMD(mainboard.adbms_raw.SPI_data.hspi, mainboard.adbms_raw.command_bit.adax);
	//parse 6830 temp
	ADBMS_6830_Parse_Temperature(&mainboard.adbms_raw.read_raw_aux, &mainboard.adbms_6830.temperature);
#endif

#if NUM_2950 > 0
	// If the packboard was powered/plugged in after boot, or reset, its config is back at
	// defaults (OCEN/REFUP/ACCI cleared) and the accumulators read garbage. Re-init if so.
	Check_2950_Config();

	// read filtered voltages
	ADBMS_Read_Voltages(&mainboard.adbms_raw.read_raw_2950_filtered, Accumulate_Read_2950, mainboard.adbms_raw.SPI_data.hspi, mainboard.adbms_raw.SPI_data.spi_dataBuf);
    // parse 2950 data
    ADBMS_2950_Calculate_Values(&mainboard.adbms_raw.read_raw_2950_filtered, &mainboard.adbms_2950, mainboard.adbms_raw.command_parameters.parameter_2950.cfa2950.acci);
#endif

    //Update Faults
#if NUM_6830 > 0
    Update_6830_InternalFault(&mainboard.adbms_6830);
#endif
	Update_2950_InternalFault(&mainboard.adbms_2950);


	// restart adcs if needed
    if (NUM_6830 > 0 && mainboard.adbms_6830.voltage.adc_off)
    {
		ADBMS_Initialize(&mainboard.adbms_raw, mainboard.adbms_raw.SPI_data.hspi);
    }

	// update STM32 Pin values
    mainboard.shutdown_present = HAL_GPIO_ReadPin(GPIOD, Shutdown_Contactors_Pin); 	   		  // shutdown status
    mainboard.imd_status = mainboard.imd_status && HAL_GPIO_ReadPin(GPIOD, IMD_STATUS_IN_Pin); // IMD_Status (software latched)
    mainboard.comms_6822_state = HAL_GPIO_ReadPin(GPIOD, AD6822_State_Pin);	 //Currently Unused  		   		  // 6822_State

	//soc
	Soc_Update(&mainboard);

	// delay to allow higher duty cycle on cb
	if (mainboard.internal_state == Charging) HAL_Delay(32);
}

void check_faults()
{
	// raise fault flag if any fault is true
	// faults are latching
	mainboard.bms_fault = mainboard.bms_fault
							|| mainboard.adbms_6830.faults.overvoltage_fault
							|| mainboard.adbms_6830.faults.undervoltage_fault
							|| mainboard.adbms_6830.faults.overtemperature_fault
							|| mainboard.adbms_6830.faults.undertemperature_fault
							|| mainboard.adbms_6830.faults.openwire_voltage_fault
							|| mainboard.adbms_6830.faults.openwire_temp_fault
							|| mainboard.adbms_6830.faults.pec_fault;

	// Preserve PD7's startup LOW while the hold-low override is enabled.
#if !PD7_HOLD_LOW_ENABLED
	// write BMS_Status - healthy is high
	HAL_GPIO_WritePin(BMS_STATUS_OUT_GPIO_Port, BMS_STATUS_OUT_Pin, !mainboard.bms_fault);
#endif

	// set external faults
	// timeouts
	if (mainboard.internal_state == Charging) //in charger
	{
		// In charge states only check for charger timeout
		float charger_dt = HAL_GetTick() - mainboard.charger_last_msg_time;
		mainboard.charger_timeout = charger_dt > CHARGER_CAN_TIMEOUT;

		mainboard.timeout_fault = mainboard.charger_timeout;
		
	}
	else //in car
	{
		//In non-charge states check for inverter and vcu timeouts
		float vcu_dt = HAL_GetTick() - mainboard.vcu_last_msg_time;
		mainboard.vcu_timeout = vcu_dt > VCU_CAN_TIMEOUT;

		float inverter_dt = HAL_GetTick() - mainboard.inverter_last_msg_time;
		mainboard.inverter_timeout = inverter_dt > INVERTER_CAN_TIMEOUT;

		mainboard.timeout_fault = mainboard.vcu_timeout || mainboard.inverter_timeout;
	}

	mainboard.external_fault = !mainboard.shutdown_present || mainboard.timeout_fault;

	// Turns on external LED if external fault
	HAL_GPIO_WritePin(GPIOE, GPIO_LED_1_Pin, mainboard.external_fault);

}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) //TSSI Callback
{
	/* STM light test: keep both TSSI outputs high. */
	(void)htim;
	HAL_GPIO_WritePin(GPIOB, TSSI_G_Pin|TSSI_R_Pin, GPIO_PIN_SET);
}

void send_data_over_printf()
{
	// AD Prints for both chips
//    ADBMS_6830_Print_Vals(&mainboard.adbms_6830);
//	ADBMS_2950_Print_Vals(&mainboard.adbms_2950);
//	ADBMS_2950_Print_I1_Diff(&mainboard.adbms_2950, mainboard.adbms_raw.command_parameters.parameter_2950.cfa2950.acci);
	if (NUM_6830 > 0)
	{
		ADBMS_6830_Print_Voltages(&mainboard.adbms_6830);
		if (ENABLE_TEMP_PRINT) ADBMS_6830_Print_Temps(&mainboard.adbms_6830);
		print_owc_results();
	}
#if NUM_2950 > 0
	{
		// The raw dump does ~8 SPI reads and a lot of text; at the 20 ms loop rate it would
		// flood USB and slow CAN. Print the packboard data every PACKBOARD_PRINT_PERIOD_MS.
		static uint32_t last_print = 0;
		if (HAL_GetTick() - last_print >= PACKBOARD_PRINT_PERIOD_MS)
		{
			last_print = HAL_GetTick();
			printf("\n=== t=%lu ms | state=%d bms_fault=%d ext_fault=%d | 2950 PEC fails in a row=%lu (latched warn=%d) ===\n",
			       (unsigned long)(last_print - mainboard.start_time), (int)mainboard.internal_state,
			       (int)mainboard.bms_fault, (int)mainboard.external_fault,
			       (unsigned long)mainboard.adbms_2950.warnings.pec_warning_count,
			       (int)mainboard.adbms_2950.warnings.pec_warning);
			ADBMS_2950_Print_Vals(&mainboard.adbms_2950);
			ADBMS_2950_Print_Raw_ADC(&mainboard.adbms_raw);
		}
	}
#endif


	// Mainboard Prints
//	printf("Time: %d\n", (int)(HAL_GetTick() - mainboard.start_time));
//	printf("BMS fault: %d\n", mainboard.bms_fault);
//	printf("External fault: %d\n", mainboard.external_fault);
//	printf("BMS State: %d\n", mainboard.internal_state);
//	printf("Charging State: %d\n", mainboard.charging_state);
//	printf("SOC: %f\n", mainboard.soc);
	
	// TODO Add more prints as needed
}
