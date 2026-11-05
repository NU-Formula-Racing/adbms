#include "adbms_interface.h"


void ADBMS_Initialize(adbms_raw_* adbms_raw, SPI_HandleTypeDef *hspi)
{
    adbms_raw->SPI_data.hspi = hspi;

    //configuration parameter place
    ADBMS_6830_Config(&adbms_raw->command_parameters.parameter_6830,&adbms_raw->command_bit);
    ADBMS_2950_Config(&adbms_raw->command_parameters.parameter_2950,&adbms_raw->command_bit);
    ADBMS_joint_Config(&adbms_raw->command_parameters.parameter_joint,&adbms_raw->command_bit);

    //wakeup IC
    ADBMS_WakeUP_ICs();
    ADBMS_WakeUP_ICs();

    //write data and commands
    // 2950 overcurrent config (CFGB) is only latched on a 0->1 edge of OCEN (CFGA), so:
    // WRCFGB first, then WRCFGA with OCEN = 0, then WRCFGA with OCEN = 1.
    cfa2950_ *cfa2950 = &adbms_raw->command_parameters.parameter_2950.cfa2950;
    ADBMS_Write_Data(adbms_raw->SPI_data.hspi, WRCFGB, adbms_raw->command_bit.cfg_b, adbms_raw->SPI_data.spi_dataBuf);

    cfa2950->ocen = 0;
    ADBMS_Set_Config_A_2950(cfa2950, adbms_raw->command_bit.cfg_a, NUM_2950, POSITION_2950);
    ADBMS_Write_Data(adbms_raw->SPI_data.hspi, WRCFGA, adbms_raw->command_bit.cfg_a, adbms_raw->SPI_data.spi_dataBuf);
    HAL_Delay(1); // > one OCxADC conversion (tOCEN,DLY1)

    // Clear all FLAG latches. CLRFLAG (0x0717): write 1 to clear.
    //  - OC latches survive an MCU reset (the 2950 keeps them while powered).
    //  - Fault bits (OCMM, OCAGD, OCBGD, VDDUV, NOCLK, REFFLT, VREG/VDIG OV/UV, MEDx, VDE, TMODE, OSCFLT)
    //    force OCA/OCB into the fault state (driven LOW) while set. OCMM resets to 1 at power-up,
    //    so without this the OC outputs can never go high. (Datasheet Table 73, note 2)
    // FLAG byte 0: [5]VDRUV [4]OCMM [3]OC3L [2]OCAGD [1]OCAL [0]OC1L
    // FLAG byte 1: [5]VDDUV [4]NOCLK [3]REFFLT [2]OCBGD [1]OCBL [0]OC2L
    // FLAG byte 2/3: read-only counters
    // FLAG byte 4: VREGOV VREGUV VDIGOV VDIGUV SED1 MED1 SED2 MED2
    // FLAG byte 5: VDEL VDE 0 SPIFLT RESET THSD TMODE OSCFLT
#if NUM_2950 > 0
    uint8_t clr_flag[NUM_CHIPS * DATA_LEN] = {0};
    uint8_t *clr_2950 = &clr_flag[(POSITION_2950 * NUM_6830) * DATA_LEN];
    clr_2950[0] = 0x3F;
    clr_2950[1] = 0x3F;
    clr_2950[4] = 0xFF;
    clr_2950[5] = 0xDF;
    ADBMS_Write_Data(adbms_raw->SPI_data.hspi, CLRFLAG, clr_flag, adbms_raw->SPI_data.spi_dataBuf);
#endif

    cfa2950->ocen = 1;
    ADBMS_Set_Config_A_2950(cfa2950, adbms_raw->command_bit.cfg_a, NUM_2950, POSITION_2950);
    ADBMS_Write_Data(adbms_raw->SPI_data.hspi, WRCFGA, adbms_raw->command_bit.cfg_a, adbms_raw->SPI_data.spi_dataBuf);

    // Turn on sensing
    ADBMS_Write_CMD(adbms_raw->SPI_data.hspi, adbms_raw->command_bit.adcv);
    HAL_Delay(1); // ADCs are updated at their conversion rate of 1ms
    ADBMS_Write_CMD(adbms_raw->SPI_data.hspi, adbms_raw->command_bit.adax);
    HAL_Delay(1); 
    ADBMS_Write_CMD(adbms_raw->SPI_data.hspi, adbms_raw->command_bit.adsv);
    HAL_Delay(1);
    ADBMS_Write_CMD(adbms_raw->SPI_data.hspi, adbms_raw->command_bit.adv); //new 2950 command to start V1adc and V2adc
    HAL_Delay(8);
}

void ADBMS_Read_Voltages(adbms_read_raw_* read_raw, voltage_read_type_ type, SPI_HandleTypeDef *hspi,uint8_t *spi_dataBuf)
{

    switch (type)
    {
        case C_Channel_Read:
            ADBMS_Read_Data(hspi, RDCVA, (read_raw->read_return + 0 * NUM_CHIPS * DATA_LEN), spi_dataBuf,&read_raw->read_pec_failure); //read voltages 0-2 for each chip //read current for 2950 chip
            ADBMS_Read_Data(hspi, RDCVB, (read_raw->read_return + 1 * NUM_CHIPS * DATA_LEN), spi_dataBuf,&read_raw->read_pec_failure); //read voltages 3-5 for each chip
            ADBMS_Read_Data(hspi, RDCVC, (read_raw->read_return + 2 * NUM_CHIPS * DATA_LEN), spi_dataBuf,&read_raw->read_pec_failure); //read voltages 6-8 for each chip
            ADBMS_Read_Data(hspi, RDCVD, (read_raw->read_return + 3 * NUM_CHIPS * DATA_LEN), spi_dataBuf,&read_raw->read_pec_failure); //read voltages 9-11 for each chip
            break;

        case C_Channel_Filtered_Read:
            ADBMS_Read_Data(hspi, RDFCA, (read_raw->read_return + 0 * NUM_CHIPS * DATA_LEN), spi_dataBuf,&read_raw->read_pec_failure); //read filtered voltages 0-2 for each chip
            ADBMS_Read_Data(hspi, RDFCB, (read_raw->read_return + 1 * NUM_CHIPS * DATA_LEN), spi_dataBuf,&read_raw->read_pec_failure); //read filtered voltages 3-5 for each chip
            ADBMS_Read_Data(hspi, RDFCC, (read_raw->read_return + 2 * NUM_CHIPS * DATA_LEN), spi_dataBuf,&read_raw->read_pec_failure); //read filtered voltages 6-8 for each chip
            ADBMS_Read_Data(hspi, RDFCD, (read_raw->read_return + 3 * NUM_CHIPS * DATA_LEN), spi_dataBuf,&read_raw->read_pec_failure); //read filtered voltages 9-11 for each chip
            break;

        case S_Channel_Read:
            ADBMS_Read_Data(hspi, RDSVA, (read_raw->read_return + 0 * NUM_CHIPS * DATA_LEN), spi_dataBuf,&read_raw->read_pec_failure);
            ADBMS_Read_Data(hspi, RDSVB, (read_raw->read_return + 1 * NUM_CHIPS * DATA_LEN), spi_dataBuf,&read_raw->read_pec_failure);
            ADBMS_Read_Data(hspi, RDSVC, (read_raw->read_return + 2 * NUM_CHIPS * DATA_LEN), spi_dataBuf,&read_raw->read_pec_failure);
            ADBMS_Read_Data(hspi, RDSVD, (read_raw->read_return + 3 * NUM_CHIPS * DATA_LEN), spi_dataBuf,&read_raw->read_pec_failure);
            break;
        
        case AUX_Read: 
            ADBMS_Read_Data(hspi, RDAUXA, (read_raw->read_return + 0 * NUM_CHIPS * DATA_LEN), spi_dataBuf,&read_raw->read_pec_failure);
            ADBMS_Read_Data(hspi, RDAUXB, (read_raw->read_return + 1 * NUM_CHIPS * DATA_LEN), spi_dataBuf,&read_raw->read_pec_failure);
            ADBMS_Read_Data(hspi, RDAUXC, (read_raw->read_return + 2 * NUM_CHIPS * DATA_LEN), spi_dataBuf,&read_raw->read_pec_failure);
            ADBMS_Read_Data(hspi, RDAUXD, (read_raw->read_return + 3 * NUM_CHIPS * DATA_LEN), spi_dataBuf,&read_raw->read_pec_failure);
            break;

        case Accumulate_Read_2950:
            ADBMS_Read_Data(hspi, RDACA, (read_raw->read_return + 0 * NUM_CHIPS * DATA_LEN), spi_dataBuf,&read_raw->read_pec_failure);
            ADBMS_Read_Data(hspi, RDACB, (read_raw->read_return + 1 * NUM_CHIPS * DATA_LEN), spi_dataBuf,&read_raw->read_pec_failure);
            break;       
    }

}

//
// Chip Configurations
//
void ADBMS_6830_Config(command_parameters_6830_* parameters,config_command_bits_* command_bits){

    // Set initial configurations
    for (uint8_t cic = 0; cic < (NUM_6830); cic++)
    {
        // Init config A
        parameters->cfa6830[cic].refon = 1;
        parameters->cfa6830[cic].gpo = 0x3FF;  // all gpo tunred on
        parameters->cfa6830[cic].fc = 0b010;   // 45Hz corner freq

        // Init config B
        parameters->cfb6830[cic].vuv = Set_UnderOver_Voltage_Threshold(UNDERVOLTAGE);
        parameters->cfb6830[cic].vov = Set_UnderOver_Voltage_Threshold(OVERVOLTAGE);
        
    }

    ADBMS_Set_Config_A_6830(parameters->cfa6830,command_bits->cfg_a, NUM_6830, POSITION_6830); //sets for all 6830
    ADBMS_Set_Config_B_6830(parameters->cfb6830,command_bits->cfg_b, NUM_6830, POSITION_6830); // sets for all 6830
    
}

void ADBMS_2950_Config(command_parameters_2950_* parameters, config_command_bits_* command_bits){

    //
    //cfa configs
    //
    parameters->cfa2950.refup = 1;    // refup to 1
    parameters->cfa2950.gpo1c = 1;    // for opening the mosfet for vbat
    parameters->cfa2950.gpo1od = 0;   // for opening the mosfet for vbat
    parameters->cfa2950.acci = 0b100; // accumulate 20 1ms samples for 50Hz reading

    ADBMS_Set_Config_A_2950(&parameters->cfa2950,command_bits->cfg_a, NUM_2950,POSITION_2950); //2950 is the last chip on the daisychain

    //
    //cfb configs: overcurrent (datasheet Tables 71/72, "Overcurrent Management")
    //
    // OCxADC LSB = 2.5 mV with gain 2, so the smallest real threshold is 1 LSB = 2.5 mV
    // (OCxTH = 0 means "always asserted", not 0 V). With a 500 uOhm shunt, 2.5 mV = 5 A.
    // OC1 = I1A-I1B, OC2 = I2A-I2B, OC3 = I3A-I3B. OCA/OCB follow a 2-of-3 majority vote of OC1..OC3.
    // (Pin test: see ADBMS_2950_OC_Pin_Test / OC_PIN_TEST in bms_system_prams.h)
#define OC_THRESHOLD_LSB   1   // 1 LSB = 2.5 mV

    parameters->cfb2950.oc1gc  = 1;     // gain 2 -> 2.5 mV/LSB
    parameters->cfb2950.oc2gc  = 1;
    parameters->cfb2950.oc3gc  = 1;
    parameters->cfb2950.oc1th  = OC_THRESHOLD_LSB;   // trip at |I1A-I1B| >= 2.5 mV
    parameters->cfb2950.oc2th  = OC_THRESHOLD_LSB;   // trip at |I2A-I2B| >= 2.5 mV
    parameters->cfb2950.oc3th  = OC_THRESHOLD_LSB;   // trip at |I3A-I3B| >= 2.5 mV
    parameters->cfb2950.ocdgt  = 0b00;  // no deglitch (1oo1)
    parameters->cfb2950.ocdp   = 1;     // short safety interval after OCEN rising edge
    parameters->cfb2950.ocmode = 0b11;  // static mode: latched, cleared only by CLRFLAG
    parameters->cfb2950.ocax   = 0;     // active high
    parameters->cfb2950.ocbx   = 0;     // active high
    parameters->cfb2950.ocod   = 0;     // push-pull: driven low normally, driven to VREG on overcurrent

    ADBMS_Set_Config_B_2950(&parameters->cfb2950,command_bits->cfg_b, NUM_2950,POSITION_2950); //2950 is the last chip on the daisychain

    
}

// Test helper: drive OCA / OCB high individually without an overcurrent event.
// In static mode the pin = OC latch XOR OCxX, so with no overcurrent (latch = 0) setting OCAX/OCBX = 1
// drives that pin high. The datasheet recommends this (static mode + OCAX/OCBX) for pin connectivity tests.
// Uses the datasheet update sequence: WRCFGB, WRCFGA (OCEN = 0), wait, WRCFGA (OCEN = 1).
void ADBMS_2950_OC_Pin_Test(adbms_raw_* adbms_raw, bool oca_high, bool ocb_high)
{
    cfb2950_ *cfb = &adbms_raw->command_parameters.parameter_2950.cfb2950;
    cfa2950_ *cfa = &adbms_raw->command_parameters.parameter_2950.cfa2950;

    cfb->ocax = oca_high ? 1 : 0;
    cfb->ocbx = ocb_high ? 1 : 0;
    ADBMS_Set_Config_B_2950(cfb, adbms_raw->command_bit.cfg_b, NUM_2950, POSITION_2950);

    ADBMS_WakeUP_ICs();
    ADBMS_Write_Data(adbms_raw->SPI_data.hspi, WRCFGB, adbms_raw->command_bit.cfg_b, adbms_raw->SPI_data.spi_dataBuf);

    cfa->ocen = 0;
    ADBMS_Set_Config_A_2950(cfa, adbms_raw->command_bit.cfg_a, NUM_2950, POSITION_2950);
    ADBMS_Write_Data(adbms_raw->SPI_data.hspi, WRCFGA, adbms_raw->command_bit.cfg_a, adbms_raw->SPI_data.spi_dataBuf);
    HAL_Delay(1); // > one OCxADC conversion (tOCEN,DLY1)

    cfa->ocen = 1;
    ADBMS_Set_Config_A_2950(cfa, adbms_raw->command_bit.cfg_a, NUM_2950, POSITION_2950);
    ADBMS_Write_Data(adbms_raw->SPI_data.hspi, WRCFGA, adbms_raw->command_bit.cfg_a, adbms_raw->SPI_data.spi_dataBuf);
}

void ADBMS_joint_Config(command_parameters_joint_* parameters, config_command_bits_* command_bits){

    //
    //adcv configs
    //
    parameters->adcv.cont = 1; //Init sensing cmd
    parameters->adcv.rstf = 1; //reset filter

    ADBMS_Set_ADCV(parameters->adcv, &command_bits->adcv); // set adcv


    //
    //adsv configs
    //
    parameters->adsv.cont = 1; // for 2950 measurements

    ADBMS_Set_ADSV(parameters->adsv, &command_bits->adsv); //set adsv

    //
    //adax configs
    //
    ADBMS_Set_ADAX(parameters->adax,&command_bits->adax); //set adax


    //
    //adv configs
    //
    parameters->adv.ow = 0;  //open_wire source off
    parameters->adv.vch = 9; //vch = 9: multi-measurement V1..V8 + VREF2 (~2.4 ms). vch = 0 only converts V1. See ADBMS2950B datasheet Tables 57/58

    ADBMS_Set_ADV(parameters->adv, &command_bits->adv); //set adv
}


//
// these are all configuration or reads for open-wire checks
//
// this also repeats quite a lot, i think i can change this too

void ADBMS_Owc_Config(adbms_raw_* adbms, Owc_Channel_ channel, Owc_Mode_ mode)
{
    ADBMS_WakeUP_ICs();

    switch(channel)
    {
        
        case C_Channel:
            switch(mode)
            {
                case Channel_Off:
                    adbms->command_parameters.parameter_joint.adcv.cont = 1;
                    adbms->command_parameters.parameter_joint.adcv.ow = 0; //Disable OW
                    break;

                case Channel_Even_On:
                    adbms->command_parameters.parameter_joint.adcv.cont = 1;
                    adbms->command_parameters.parameter_joint.adcv.ow = 1; //Enable Even Channel OW
                    break;
                case Channel_Odd_On:
                    adbms->command_parameters.parameter_joint.adcv.cont = 1;
                    adbms->command_parameters.parameter_joint.adcv.ow = 2; //Enable Odd Channel OW
                    break;
            }
        
            ADBMS_Set_ADCV(adbms->command_parameters.parameter_joint.adcv, &adbms->command_bit.adcv);
            ADBMS_Write_CMD(adbms->SPI_data.hspi,adbms->command_bit.adcv);
            HAL_Delay(OWC_SETTLE_MS); // let the OW current pull an open input through the input filter
            ADBMS_WakeUP_ICs();       // isoSPI may have gone idle during the wait
            break;
        
            
        case S_Channel:
            switch(mode)
            {
                case Channel_Off:
                    adbms->command_parameters.parameter_joint.adsv.cont = 1;
                    adbms->command_parameters.parameter_joint.adsv.ow = 0; //Disable OW 
                    break;
                case Channel_Even_On:
                    adbms->command_parameters.parameter_joint.adsv.cont = 1;
                    adbms->command_parameters.parameter_joint.adsv.ow = 1; // Enable OW on even-channel 
                    break;
                case Channel_Odd_On:
                    adbms->command_parameters.parameter_joint.adsv.cont = 1;
                    adbms->command_parameters.parameter_joint.adsv.ow = 2; // Enable OW on odd-channel 
                    break;
            
            }
            ADBMS_Set_ADSV(adbms->command_parameters.parameter_joint.adsv, &adbms->command_bit.adsv);
            ADBMS_Write_CMD(adbms->SPI_data.hspi,adbms->command_bit.adsv);
            HAL_Delay(OWC_SETTLE_MS); // let the OW current pull an open input through the input filter
            ADBMS_WakeUP_ICs();       // isoSPI may have gone idle during the wait
            break;
    }
}