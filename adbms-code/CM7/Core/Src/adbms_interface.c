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
    ADBMS_Write_Data(adbms_raw->SPI_data.hspi, WRCFGA, adbms_raw->command_bit.cfg_a, adbms_raw->SPI_data.spi_dataBuf);
    ADBMS_Write_Data(adbms_raw->SPI_data.hspi, WRCFGB, adbms_raw->command_bit.cfg_b, adbms_raw->SPI_data.spi_dataBuf);

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
        {
            pec_return_ current_pec; // keep a PEC failure on RDACA from being overwritten by the RDACB read
            ADBMS_Read_Data(hspi, RDACA, (read_raw->read_return + 0 * NUM_CHIPS * DATA_LEN), spi_dataBuf, &current_pec);
            ADBMS_Read_Data(hspi, RDACB, (read_raw->read_return + 1 * NUM_CHIPS * DATA_LEN), spi_dataBuf,&read_raw->read_pec_failure);
            read_raw->read_pec_failure.pec_2950 |= current_pec.pec_2950;
            read_raw->read_pec_failure.pec_6830 |= current_pec.pec_6830;
            break;
        }       
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
    //no cfb configs for now
    //
    ADBMS_Set_Config_B_2950(&parameters->cfb2950,command_bits->cfg_b, NUM_2950,POSITION_2950); //2950 is the last chip on the daisychain

    
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
    parameters->adv.vch = 0; //vch = 0 (this is complicated, for reference check datasheet table 57/58)

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
            HAL_Delay(1);
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
            HAL_Delay(1);
            break;
    }
}

//
// isoSPI ring manager (reversible isoSPI + COMM_BK, see ADBMS2950B "Configurable isoSPI Break")
//
#define RING_BREAK_CONFIRM_LOOPS 3      // forward tail must fail this many loops in a row before isolating
#define RING_PROBE_MS            2000U  // while isolated, clear COMM_BK this often to check for a repair

// Set COMM_BK on the chips in chip_mask (forward chip index) and clear it on all others, then write CFGA.
static void ring_set_comm_bk(adbms_raw_ *adbms_raw, uint32_t chip_mask)
{
    for (uint8_t i = 0; i < NUM_6830; i++)
    {
        adbms_raw->command_parameters.parameter_6830.cfa6830[i].comm_bk = (chip_mask >> (POSITION_6830 * NUM_2950 + i)) & 1U;
    }
#if NUM_2950 > 0
    adbms_raw->command_parameters.parameter_2950.cfa2950.commbk = (chip_mask >> (POSITION_2950 * NUM_6830)) & 1U;
#endif
    ADBMS_Set_Config_A_6830(adbms_raw->command_parameters.parameter_6830.cfa6830, adbms_raw->command_bit.cfg_a, NUM_6830, POSITION_6830);
    ADBMS_Set_Config_A_2950(&adbms_raw->command_parameters.parameter_2950.cfa2950, adbms_raw->command_bit.cfg_a, NUM_2950, POSITION_2950);
    ADBMS_WakeUP_ICs();
    ADBMS_Write_Data(adbms_raw->SPI_data.hspi, WRCFGA, adbms_raw->command_bit.cfg_a, adbms_raw->SPI_data.spi_dataBuf);
}

void ADBMS_Ring_Manage(adbms_raw_ *adbms_raw)
{
    static uint8_t break_loops = 0;
    static uint32_t isolated_since = 0;
    const uint32_t all_chips = (NUM_CHIPS >= 32) ? 0xFFFFFFFFUL : ((1UL << NUM_CHIPS) - 1UL);

    // reverse-path health check (rate limited inside)
    ADBMS_Ring_Check(adbms_raw->SPI_data.hspi);
    if (!adbms_ring_status.reverse_enabled) return;

    if (adbms_ring_status.isolated)
    {
        // probe: clear COMM_BK everywhere; if the break is still there it is re-isolated within a few loops
        if (HAL_GetTick() - isolated_since >= RING_PROBE_MS)
        {
            ring_set_comm_bk(adbms_raw, 0);
            adbms_ring_status.isolated = false;
            break_loops = 0;
        }
        return;
    }

    // A break shows up as a contiguous tail of chips failing on the forward path (chip k onwards).
    const uint32_t fwd = adbms_ring_status.fwd_fail_mask;
    uint8_t k = 0;
    while (k < NUM_CHIPS && !(fwd & (1UL << k))) k++;
    const uint32_t tail = all_chips & ~((1UL << k) - 1UL);
    if (fwd == 0 || fwd != tail)
    {
        break_loops = 0;
        return;
    }
    if (++break_loops < RING_BREAK_CONFIRM_LOOPS) return;

    // Stop chip k-1 (last one reachable forward) and chip k (last one reachable in reverse)
    // from driving the broken segment.
    uint32_t mask = 1UL << k;
    if (k > 0) mask |= 1UL << (k - 1);
    ring_set_comm_bk(adbms_raw, mask);
    adbms_ring_status.isolated = true;
    adbms_ring_status.break_before_chip = k;
    isolated_since = HAL_GetTick();
}
