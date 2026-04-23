#include "test.h"

static void fill_valid_read_frame(uint8_t *spi_buf, const uint8_t data_bytes[DATA_LEN])
{
    memset(spi_buf, 0, DATABUF_LEN);

    uint8_t *rx_dataBuf = spi_buf + CMD_LEN + PEC_LEN;

    for (int i = 0; i < DATA_LEN; i++)
    {
        rx_dataBuf[i] = data_bytes[i];
    }

    uint16_t pec = Pec10_Calc(true, DATA_LEN, rx_dataBuf);
    rx_dataBuf[DATA_LEN]     = (uint8_t)((pec >> 8) & 0x03);
    rx_dataBuf[DATA_LEN + 1] = (uint8_t)(pec & 0xFF);
}

static void fill_invalid_read_frame(uint8_t *spi_buf, const uint8_t data_bytes[DATA_LEN])
{
    fill_valid_read_frame(spi_buf, data_bytes);

    uint8_t *rx_dataBuf = spi_buf + CMD_LEN + PEC_LEN;
    rx_dataBuf[DATA_LEN + 1] ^= 0x01;   /* corrupt PEC */
}

int main(void)
{
    /* driver-layer tests */
    test_set_adcv_basic();
    test_set_adsv_basic();
    test_set_adv_basic();
    test_set_config_a_6830_basic();
    test_set_config_b_6830_thresholds();

    /* interface/default-config tests */
    test_initialize_sets_defaults();
    test_6830_config_sets_expected_defaults();
    test_2950_config_sets_expected_defaults();
    test_joint_config_sets_expected_defaults();

    /* open-wire interface tests */
    test_owc_c_channel_off();
    test_owc_c_channel_even_on();
    test_owc_c_channel_odd_on();
    test_owc_s_channel_off();
    test_owc_s_channel_even_on();
    test_owc_s_channel_odd_on();

    /* read path tests */
    test_read_voltage_pec_flag();
    test_read_temps_pec_flag();

    printf("All tests passed.\n");
    return 0;
}

/* =========================================================
   Driver-layer tests
   ========================================================= */

void test_set_adcv_basic(void)
{
    adcv_ adcv = {0};
    uint16_t cmd = 0;

    adcv.rd   = 1;
    adcv.cont = 1;
    adcv.dcp  = 0;
    adcv.rstf = 1;
    adcv.ow   = 2;

    ADBMS_Set_ADCV(adcv, &cmd);

    uint16_t expected =
        (0x1 << 9) |
        ((1 & 0x1) << 8) |
        ((1 & 0x1) << 7) |
        (0x3 << 5) |
        ((0 & 0x1) << 4) |
        ((1 & 0x1) << 2) |
        (2 & 0x3);

    assert(cmd == expected);
}

void test_set_adsv_basic(void)
{
    adsv_ adsv = {0};
    uint16_t cmd = 0;

    adsv.cont = 1;
    adsv.dcp  = 1;
    adsv.ow   = 2;

    ADBMS_Set_ADSV(adsv, &cmd);

    uint16_t expected =
        (0x1 << 8) |
        ((1 & 0x1) << 7) |
        (0x3 << 5) |
        ((1 & 0x1) << 4) |
        (0x1 << 3) |
        (2 & 0x3);

    assert(cmd == expected);
}

void test_set_adv_basic(void)
{
    adv_ adv = {0};
    uint16_t cmd = 0;

    adv.ow  = 2;
    adv.vch = 5;

    ADBMS_Set_ADV(adv, &cmd);

    uint16_t expected =
        (0x1 << 10) |
        ((2 & 0x3) << 6) |
        (0x3 << 4) |
        (5 & 0xF);

    assert(cmd == expected);
}

void test_set_config_a_6830_basic(void)
{
    cfa6830_ cfg[1];
    uint8_t out[6];

    memset(cfg, 0, sizeof(cfg));
    memset(out, 0, sizeof(out));

    cfg[0].refon   = 1;
    cfg[0].cth     = 0x05;
    cfg[0].flag_d  = 0xAB;
    cfg[0].soakon  = 1;
    cfg[0].owrng   = 0;
    cfg[0].owa     = 0x03;
    cfg[0].gpo     = 0x03FF;
    cfg[0].snap    = 1;
    cfg[0].mute_st = 0;
    cfg[0].comm_bk = 1;
    cfg[0].fc      = 0x04;

    ADBMS_Set_Config_A_6830(cfg, out, 1);

    assert(out[0] == (uint8_t)(((1 & 0x01) << 7) | (0x05 & 0x07)));
    assert(out[1] == 0xAB);
    assert(out[2] == (uint8_t)(((1 & 0x01) << 7) | ((0 & 0x01) << 6) | ((0x03 & 0x07) << 3)));
    assert(out[3] == 0xFF);
    assert(out[4] == 0x03);
    assert(out[5] == (uint8_t)(((1 & 0x01) << 5) | ((0 & 0x01) << 4) | ((1 & 0x01) << 3) | (0x04 & 0x07)));
}

void test_set_config_b_6830_thresholds(void)
{
    cfb6830_ cfg[1];
    uint8_t out[6];

    memset(cfg, 0, sizeof(cfg));
    memset(out, 0, sizeof(out));

    cfg[0].vuv   = 0x123;
    cfg[0].vov   = 0xABC;
    cfg[0].dtmen = 1;
    cfg[0].dtrng = 0;
    cfg[0].dcto  = 0x15;
    cfg[0].dcc   = 0x55AA;

    ADBMS_Set_Config_B_6830(cfg, out, 1);

    assert(out[0] == 0x23);
    assert(out[1] == (uint8_t)(((0xABC & 0x00F) << 4) | ((0x123 & 0xF00) >> 8)));
    assert(out[2] == (uint8_t)((0xABC & 0xFF0) >> 4));
    assert(out[3] == (uint8_t)(((1 & 0x01) << 7) | ((0 & 0x01) << 6) | (0x15 & 0x3F)));
    assert(out[4] == 0xAA);
    assert(out[5] == 0x55);
}

/* =========================================================
   Interface/default-config tests
   ========================================================= */

void test_initialize_sets_defaults(void)
{
    adbms_raw_ *adbms = calloc(1, sizeof(adbms_raw_));
    assert(adbms != NULL);

    ADBMS_Initialize(adbms);

    assert(adbms->command_parameters.parameter_6830.cfa6830[0].refon == 1);
    assert(adbms->command_parameters.parameter_6830.cfa6830[0].gpo == 0x3FF);

    assert(adbms->command_parameters.parameter_2950.cfa2950.refup == 1);
    assert(adbms->command_parameters.parameter_2950.cfa2950.gpo1c == 1);
    assert(adbms->command_parameters.parameter_2950.cfa2950.gpo1od == 0);

    assert(adbms->command_parameters.parameter_joint.adcv.cont == 1);
    assert(adbms->command_parameters.parameter_joint.adsv.cont == 1);
    assert(adbms->command_parameters.parameter_joint.adv.ow == 0);
    assert(adbms->command_parameters.parameter_joint.adv.vch == 0);

    assert(adbms->command_bit.adcv != 0);
    assert(adbms->command_bit.adsv != 0);
    assert(adbms->command_bit.adv  != 0);

    free(adbms);
}

void test_6830_config_sets_expected_defaults(void)
{
    command_parameters_6830_ params;
    config_command_bits_ bits;

    memset(&params, 0, sizeof(params));
    memset(&bits, 0, sizeof(bits));

    ADBMS_6830_Config(&params, &bits);

    assert(params.cfa6830[0].refon == 1);
    assert(params.cfa6830[0].gpo == 0x3FF);

    assert(params.cfb6830[0].vuv == Set_UnderOver_Voltage_Threshold(UNDERVOLTAGE));
    assert(params.cfb6830[0].vov == Set_UnderOver_Voltage_Threshold(OVERVOLTAGE));

    assert(bits.cfg_a[0] != 0 || bits.cfg_a[1] != 0);
    assert(bits.cfg_b[0] != 0 || bits.cfg_b[1] != 0);
}

void test_2950_config_sets_expected_defaults(void)
{
    command_parameters_2950_ params;
    config_command_bits_ bits;

    memset(&params, 0, sizeof(params));
    memset(&bits, 0, sizeof(bits));

    ADBMS_2950_Config(&params, &bits);

    assert(params.cfa2950.refup == 1);
    assert(params.cfa2950.gpo1c == 1);
    assert(params.cfa2950.gpo1od == 0);

    assert(bits.cfg_a[(NUM_CHIPS - 1) * DATA_LEN + 5] != 0 || bits.cfg_a[(NUM_CHIPS - 1) * DATA_LEN + 3] != 0);
}

void test_joint_config_sets_expected_defaults(void)
{
    command_parameters_joint_ params;
    config_command_bits_ bits;

    memset(&params, 0, sizeof(params));
    memset(&bits, 0, sizeof(bits));

    ADBMS_joint_Config(&params, &bits);

    assert(params.adcv.cont == 1);
    assert(params.adsv.cont == 1);
    assert(params.adv.ow == 0);
    assert(params.adv.vch == 0);

    assert(bits.adcv != 0);
    assert(bits.adsv != 0);
    assert(bits.adv  != 0);
}

/* =========================================================
   Open-wire interface tests
   ========================================================= */

void test_owc_c_channel_off(void)
{
    adbms_raw_ *adbms = calloc(1, sizeof(adbms_raw_));
    assert(adbms != NULL);

    Owc_C_Channel_Off(adbms);

    assert(adbms->command_parameters.parameter_joint.adcv.cont == 1);
    assert(adbms->command_parameters.parameter_joint.adcv.ow == 0);

    uint16_t expected = 0;
    ADBMS_Set_ADCV(adbms->command_parameters.parameter_joint.adcv, &expected);

    assert(adbms->command_bit.adcv == expected);

    free(adbms);
}

void test_owc_c_channel_even_on(void)
{
    adbms_raw_ *adbms = calloc(1, sizeof(adbms_raw_));
    assert(adbms != NULL);

    Owc_C_Channel_Even_On(adbms);

    assert(adbms->command_parameters.parameter_joint.adcv.cont == 1);
    assert(adbms->command_parameters.parameter_joint.adcv.ow == 1);

    uint16_t expected = 0;
    ADBMS_Set_ADCV(adbms->command_parameters.parameter_joint.adcv, &expected);

    assert(adbms->command_bit.adcv == expected);

    free(adbms);
}

void test_owc_c_channel_odd_on(void)
{
    adbms_raw_ *adbms = calloc(1, sizeof(adbms_raw_));
    assert(adbms != NULL);

    Owc_C_Channel_Odd_On(adbms);

    assert(adbms->command_parameters.parameter_joint.adcv.cont == 1);
    assert(adbms->command_parameters.parameter_joint.adcv.ow == 2);

    uint16_t expected = 0;
    ADBMS_Set_ADCV(adbms->command_parameters.parameter_joint.adcv, &expected);

    assert(adbms->command_bit.adcv == expected);

    free(adbms);
}

void test_owc_s_channel_off(void)
{
    adbms_raw_ *adbms = calloc(1, sizeof(adbms_raw_));
    assert(adbms != NULL);

    Owc_S_Channel_Off(adbms);

    assert(adbms->command_parameters.parameter_joint.adsv.cont == 1);
    assert(adbms->command_parameters.parameter_joint.adsv.ow == 0);

    uint16_t expected = 0;
    ADBMS_Set_ADSV(adbms->command_parameters.parameter_joint.adsv, &expected);

    assert(adbms->command_bit.adsv == expected);

    free(adbms);
}

void test_owc_s_channel_even_on(void)
{
    adbms_raw_ *adbms = calloc(1, sizeof(adbms_raw_));
    assert(adbms != NULL);

    Owc_S_Channel_Even_On(adbms);

    assert(adbms->command_parameters.parameter_joint.adsv.cont == 1);
    assert(adbms->command_parameters.parameter_joint.adsv.ow == 1);

    uint16_t expected = 0;
    ADBMS_Set_ADSV(adbms->command_parameters.parameter_joint.adsv, &expected);

    assert(adbms->command_bit.adsv == expected);

    free(adbms);
}

void test_owc_s_channel_odd_on(void)
{
    adbms_raw_ *adbms = calloc(1, sizeof(adbms_raw_));
    assert(adbms != NULL);

    Owc_S_Channel_Odd_On(adbms);

    assert(adbms->command_parameters.parameter_joint.adsv.cont == 1);
    assert(adbms->command_parameters.parameter_joint.adsv.ow == 2);

    uint16_t expected = 0;
    ADBMS_Set_ADSV(adbms->command_parameters.parameter_joint.adsv, &expected);

    assert(adbms->command_bit.adsv == expected);

    free(adbms);
}

/* =========================================================
   Read path tests
   ========================================================= */

void test_read_voltage_pec_flag(void)
{
    adbms_raw_ *adbms = calloc(1, sizeof(adbms_raw_));
    assert(adbms != NULL);

    uint8_t sample_data[DATA_LEN] = {1, 2, 3, 4, 5, 6};

    fill_valid_read_frame(adbms->SPI_data.spi_dataBuf, sample_data);
    ADBMS_Read_Voltage(adbms);
    assert(adbms->read_failure.read_voltage_pec_failure == false);

    fill_invalid_read_frame(adbms->SPI_data.spi_dataBuf, sample_data);
    ADBMS_Read_Voltage(adbms);
    assert(adbms->read_failure.read_voltage_pec_failure == true);

    free(adbms);
}

void test_read_temps_pec_flag(void)
{
    adbms_raw_ *adbms = calloc(1, sizeof(adbms_raw_));
    assert(adbms != NULL);

    adbms->command_bit.adax = 0x1234;

    uint8_t sample_data[DATA_LEN] = {10, 20, 30, 40, 50, 60};

    fill_valid_read_frame(adbms->SPI_data.spi_dataBuf, sample_data);
    ADBMS_Read_Temps(adbms);
    assert(adbms->read_failure.read_temp_pec_failure == false);

    fill_invalid_read_frame(adbms->SPI_data.spi_dataBuf, sample_data);
    ADBMS_Read_Temps(adbms);
    assert(adbms->read_failure.read_temp_pec_failure == true);

    free(adbms);
}