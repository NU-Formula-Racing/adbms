#ifndef TEST_H
#define TEST_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adbms_interface_v2.h"
#include "adbms_driver_v2.h"

/* driver-layer tests */
void test_set_adcv_basic(void);
void test_set_adsv_basic(void);
void test_set_adv_basic(void);
void test_set_config_a_6830_basic(void);
void test_set_config_b_6830_thresholds(void);

/* interface/default-config tests */
void test_initialize_sets_defaults(void);
void test_6830_config_sets_expected_defaults(void);
void test_2950_config_sets_expected_defaults(void);
void test_joint_config_sets_expected_defaults(void);

/* open-wire interface tests */
void test_owc_c_channel_off(void);
void test_owc_c_channel_even_on(void);
void test_owc_c_channel_odd_on(void);
void test_owc_s_channel_off(void);
void test_owc_s_channel_even_on(void);
void test_owc_s_channel_odd_on(void);

/* read path tests */
void test_read_voltage_pec_flag(void);
void test_read_temps_pec_flag(void);

#endif