#include "bms_soc.h"

void Soc_Initialize(mainboard_ *mainboard)
{
    //this is a change, this only reads the voltage
    ADBMS_WakeUP_ICs();

    ADBMS_Read_Voltages(&mainboard->adbms_raw.read_raw_c, C_Channel_Read, mainboard->adbms_raw.SPI_data.hspi, mainboard->adbms_raw.SPI_data.spi_dataBuf);

    //now we also need to parse the raw data and put it into the structs
    ADBMS_6830_Parse_Voltage(&mainboard->adbms_raw.read_raw_c, &mainboard->adbms_6830.voltage);
    float avg_v = mainboard->adbms_6830.voltage.avg_v;

    if (avg_v <= 2.5)
    {
        mainboard->soc = 0;
    }
    else if (avg_v >= 4.2)
    {
        mainboard->soc = 1;
    }
    
    int v_idx1 = fmax(0, ((int)floor((avg_v - 2.5) / SOC_QUANTIZATION)));
    float v_idx1_value = 2.5 + (v_idx1 * SOC_QUANTIZATION);

    int v_idx2 = fmin(500, ((int)ceil((avg_v - 2.5) / SOC_QUANTIZATION)));
    float v_idx2_value = 2.5 + (v_idx2 * SOC_QUANTIZATION);

    float ampacity_dis = 0;
    if (v_idx1_value == v_idx1_value)
    {
        ampacity_dis = v2a_lookup[v_idx1];
    }
    else
    {
        ampacity_dis = v2a_lookup[v_idx1] + ((avg_v - v_idx1_value) * (v2a_lookup[v_idx2] - v2a_lookup[v_idx1]))/(v_idx2_value - v_idx1_value);
    }

    mainboard->soc = (CELL_CAPACITY - (ampacity_dis / 1000.0)) / CELL_CAPACITY;
    mainboard->prev_time = HAL_GetTick();
}

void Soc_Update(mainboard_ *mainboard)
{
    float tick = HAL_GetTick();
    float delta = (tick - mainboard->prev_time) / 1000.0f / 3600.0f; // convert to hours
    mainboard->prev_time = tick;

    mainboard->soc = mainboard->soc - (delta * mainboard->adbms_2950.data.current / (SYSTEM_CAPACITY));
}
