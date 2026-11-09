#include "adbms_interface_2950.h"
#include <math.h>


//I DON'T LIKE THIS
//Ask Drake tomorrow
void ADBMS_2950_Calculate_Values(adbms_read_raw_* adbms_2950_read_raw,adbms_2950_* adbms_2950,uint8_t acci)
{
    if (NUM_2950 && !ADBMS_2950_Pec_Failure(adbms_2950_read_raw, adbms_2950)) //pec needs to be false for us to process
    {
        ADBMS_2950_Calculate_Accumulator_Vbat(adbms_2950_read_raw, adbms_2950, acci);
        ADBMS_2950_Calculate_Accumulator_Current(adbms_2950_read_raw, adbms_2950, acci);
        ADBMS_2950_Calculate_Shunt_Temp(adbms_2950_read_raw, adbms_2950);
    }
}


//
//Calculate Values
//
void ADBMS_2950_Calculate_Accumulator_Vbat(adbms_read_raw_* adbms_2950_read_raw, adbms_2950_* adbms_2950, uint8_t acci)
{
    //initialize values
    adbms_2950->raw_data.vbat1_raw = 0;
    adbms_2950->raw_data.vbat2_raw = 0;

    //offset because RDCVB is the second command sent
    int command_offset = NUM_CHIPS * DATA_LEN;
    //offset because 2950 is the last chip on the daisy chain
    int offset = (NUM_6830) * DATA_LEN;

    //take raw bit values from adbms and place into raw_data in adbms_2950
    adbms_2950->raw_data.vbat1_raw = ((int32_t)(adbms_2950_read_raw->read_return[2 + command_offset + offset]) << 16) | ((int32_t)(adbms_2950_read_raw->read_return[1 + command_offset + offset]) << 8) | adbms_2950_read_raw->read_return[0 + command_offset + offset];
    adbms_2950->raw_data.vbat2_raw = ((int32_t)(adbms_2950_read_raw->read_return[5 + command_offset + offset]) << 16) | ((int32_t)(adbms_2950_read_raw->read_return[4 + command_offset + offset]) << 8) | adbms_2950_read_raw->read_return[3 + command_offset + offset];

    //sign extend because it is a 24 bit number but stored int32
    if (adbms_2950->raw_data.vbat1_raw & 0x00800000) {     
        adbms_2950->raw_data.vbat1_raw |= 0xFF000000;      
    }

    if(adbms_2950->raw_data.vbat2_raw & 0x00800000){
        adbms_2950->raw_data.vbat2_raw |= 0xFF000000;
    }

    adbms_2950->data.precontactor_voltage = ADBMS_2950_Transfer_Accumulator_Vbat(adbms_2950->raw_data.vbat1_raw,adbms_2950->raw_data.vbat2_raw,acci);

}


void ADBMS_2950_Calculate_Accumulator_Current(adbms_read_raw_* adbms_2950_read_raw, adbms_2950_* adbms_2950, uint8_t acci)
{
    //initialize values
    adbms_2950->raw_data.i1_raw = 0;
    adbms_2950->raw_data.i2_raw = 0;

    //offset because 2950 is the last chip on the daisy chain
    int offset =  (NUM_6830) * DATA_LEN;

    //getting raw data from cell readings and putting it in adbms_2950 
    adbms_2950->raw_data.i1_raw = ((int32_t)(adbms_2950_read_raw->read_return[2 + offset]) << 16) | ((int32_t)(adbms_2950_read_raw->read_return[1 + offset]) << 8) | adbms_2950_read_raw->read_return[0 + offset];
    adbms_2950->raw_data.i2_raw = ((int32_t)(adbms_2950_read_raw->read_return[5 + offset]) << 16) | ((int32_t)(adbms_2950_read_raw->read_return[4 + offset]) << 8) | adbms_2950_read_raw->read_return[3 + offset];

    //sign extend because it is a 24 bit number but stored int32
    if (adbms_2950->raw_data.i1_raw & 0x00800000) {     
        adbms_2950->raw_data.i1_raw |= 0xFF000000;      
    }

    if(adbms_2950->raw_data.i2_raw & 0x00800000){
        adbms_2950->raw_data.i2_raw |= 0xFF000000;
    }

    //average the two
    adbms_2950->data.current = ADBMS_2950_Transfer_Accumulator_Current(adbms_2950->raw_data.i1_raw,adbms_2950->raw_data.i2_raw,acci);
}


void ADBMS_2950_Calculate_Shunt_Temp(adbms_read_raw_* adbms_2950_read_raw, adbms_2950_* adbms_2950)
{

    //initilaize values
    adbms_2950->raw_data.v7 = 0;
    adbms_2950->raw_data.v9 = 0;

    //offset because 2950 is the last chip ont he daisy chain
    int offset = (NUM_6830) * DATA_LEN;

    //getting raw data 
    adbms_2950->raw_data.v7 = (int16_t)(adbms_2950_read_raw->read_return[1+offset] << 8) | (int16_t)(adbms_2950_read_raw->read_return[0+offset]);
    adbms_2950->raw_data.v9 = (int16_t)(adbms_2950_read_raw->read_return[5+offset] << 8) | (int16_t)(adbms_2950_read_raw->read_return[4+offset]);

    adbms_2950->data.pack_temperature_1 = ADBMS_2950_Transfer_Shunt_Temp(adbms_2950->raw_data.v7);
    adbms_2950->data.pack_temperature_2 = ADBMS_2950_Transfer_Shunt_Temp(adbms_2950->raw_data.v9);

}



//
//Transfer Functions
//

float ADBMS_2950_Transfer_Accumulator_Vbat(int32_t vbat1_raw, int32_t vbat2_raw, uint8_t acci)
{
    //page 50 of datasheet
    float accn = (float) 4 * acci + 4;

    //transfer to real value
    float vbat1 = (float)(vbat1_raw / (accn * 0.00414937759)); // (15000/3,600,000 + 15000)
    float vbat2 = (float) vbat2_raw / accn;

    float vbat = (vbat1-vbat2) * 0.0001; //100 microolms

    return vbat - 0.06; // measured DC offset
}


float ADBMS_2950_Transfer_Accumulator_Current(int32_t current_1_raw, int32_t current_2_raw, uint8_t acci)
{

    //page 50 of datasheet
    float accn = (float) 4 * acci + 4;



    // old shunt transfer function
    // float current1 = (float) -current_1_raw/ (94.0 * accn);
    // float current2 = (float) current_2_raw/ (94.0 * accn);

    // new shunt transfer function
    float current1 = (float) -current_1_raw/ (512 * accn);
    float current2 = (float) current_2_raw/ (512 * accn);


    return (current1 + current2)/2;
}

float ADBMS_2950_Transfer_Shunt_Temp(int16_t voltage){

//write this later :)

}

bool ADBMS_2950_Pec_Failure(adbms_read_raw_* adbms_2950_read_raw, adbms_2950_* adbms_2950)
{

    if (adbms_2950_read_raw->read_pec_failure.pec_2950){
        adbms_2950->warnings.pec_warning_count += 1;
        return true; // pec true we don't parse this data, 
    }

    adbms_2950->warnings.pec_warning_count = 0;
    return false;
}


void Update_2950_InternalFault(adbms_2950_* adbms_2950)
{
    //latching 2950 warnings
    adbms_2950->warnings.pec_warning = adbms_2950->warnings.pec_warning || (adbms_2950->warnings.pec_warning_count > PEC_FAILURE_THRESHOLD);

}

 

//
//Prints 
//
void ADBMS_2950_Print_Vals(adbms_2950_* adbms_2950)
{
    // 2950 prints
    printf("ADBMS 2950 Data\n\n");
    printf("adbms2950 Precontactor voltage: %f\n", adbms_2950->data.precontactor_voltage);
    printf("adbms2950 Current: %f\n\n", adbms_2950->data.current);

    printf("\n ADBMS 2950 Warnings \n\n");
    printf("adbms2950 Pec Warning: %d\n", adbms_2950->warnings.pec_warning);
}

//
// Functional test: raw ADC dump
//
// LSB sizes (volts per count), ADBMS2950B datasheet Tables 44 and 51:
//   I1 / I2 ADC      : 1 uV   (I1 = I1A - I1B)
//   VB1ADC, VxA      : 100 uV (V1ADC results)
//   VB2ADC, VxB      : -85 uV (V2ADC results, inverted gain)
#define LSB_I_V    0.000001f
#define LSB_VB1_V  0.0001f
#define LSB_VB2_V  (-0.000085f)
#define LSB_V_V    0.0001f
#define LSB_VB_V   LSB_VB1_V   // accumulator print below
#define LSB_V2ADC  (-0.000085f)

typedef struct
{
    const char *label;
    uint8_t     byte;    // byte offset in the 6-byte register group
    uint8_t     width;   // 2 = int16, 3 = int24
    float       lsb;     // volts per count
} raw_field_;

typedef struct
{
    const char *name;
    uint16_t    cmd;
    uint8_t     n_fields;
    raw_field_  fields[3];
} raw_group_;

// 2950 result register groups read fresh each loop.
// Accumulator groups (RDACA/RDACB = I/VB ACC) are NOT re-read here because reading clears them;
// they are printed from the buffer already fetched in update_values().
// Command codes and byte layouts from ADBMS2950B datasheet Tables 43/45 (I, VB) and 49/52 (V).
// V1..V8 come from V1ADC (RDV1x), V9/V10 only from V2ADC (RDV2C). Requires ADV with VCH = 9.
static const raw_group_ raw_groups_2950[] __attribute__((unused)) = {
    { "RDI   ", 0x0004, 2, { { "I1A-I1B", 0, 3, LSB_I_V   }, { "I2A-I2B", 3, 3, LSB_I_V   } } },
    { "RDVB  ", 0x0006, 2, { { "VB1",     2, 2, LSB_VB1_V }, { "VB2",     4, 2, LSB_VB2_V } } },
    { "RDV1A ", 0x000A, 3, { { "V1",      0, 2, LSB_V_V   }, { "V2",      2, 2, LSB_V_V   }, { "V3", 4, 2, LSB_V_V } } },
    { "RDV1B ", 0x0009, 3, { { "V4",      0, 2, LSB_V_V   }, { "V5",      2, 2, LSB_V_V   }, { "V6", 4, 2, LSB_V_V } } },
    { "RDV1C ", 0x0003, 2, { { "V7",      0, 2, LSB_V_V   }, { "V8",      2, 2, LSB_V_V   } } },
    { "RDV2C ", 0x0005, 2, { { "V9",      0, 2, LSB_V2ADC }, { "V10",     2, 2, LSB_V2ADC } } },
};

static int32_t decode_signed(const uint8_t *b, uint8_t width)
{
    if (width == 2) return (int16_t)((b[1] << 8) | b[0]);

    int32_t v = ((int32_t)b[2] << 16) | ((int32_t)b[1] << 8) | b[0];
    if (v & 0x00800000) v |= 0xFF000000;
    return v;
}

// divisor = 1 for instantaneous registers, accn for accumulators (gives per-sample average)
__attribute__((unused)) static void print_raw_group(const raw_group_ *grp, const uint8_t *d, bool pec_fail, float divisor)
{
    for (int i = 0; i < grp->n_fields; i++)
    {
        const raw_field_ *f = &grp->fields[i];
        float volts = (float)decode_signed(&d[f->byte], f->width) / divisor * f->lsb;
        printf("%-14s %11.6f V%s\n", f->label, volts, pec_fail ? "  (PEC FAIL)" : "");
    }
}

//
// Packboard thermistors on V7 / V9 / V10:
//   VREF1P25 --[10k pull-up]--+-- Vx pin
//                             |
//                           [NTC]  TDK NTCG 10k, J-class (B25/85 = 3435 K)
//                             |
//                            SGND
//
#define THERM_VREF_V      1.25f      // VREF1P25 nominal
#define THERM_PULLUP_OHM  10000.0f
#define THERM_R25_OHM     10000.0f
#define THERM_B_K         3435.0f    // NTCG..J..103 parts. Use 3950 for NTCG..BH103 parts
#define THERM_T25_K       298.15f

// Returns temperature in degC, or NAN if the voltage is outside the divider range (open / short)
float ADBMS_2950_Thermistor_Temp(float v)
{
    if (v <= 0.0f || v >= THERM_VREF_V) return NAN;

    float r_ntc = THERM_PULLUP_OHM * v / (THERM_VREF_V - v);
    float inv_t = 1.0f / THERM_T25_K + logf(r_ntc / THERM_R25_OHM) / THERM_B_K;
    return 1.0f / inv_t - 273.15f;
}

__attribute__((unused)) static void print_thermistor(const char *name, float v, bool pec_fail)
{
    float t = ADBMS_2950_Thermistor_Temp(v);
    if (isnan(t))
        printf("%s  %9.4f V   ----- degC (%s)%s\n", name, v, v <= 0.0f ? "short to GND?" : "open / no thermistor?", pec_fail ? "  (PEC FAIL)" : "");
    else
        printf("%s  %9.4f V  %6.2f degC%s\n", name, v, t, pec_fail ? "  (PEC FAIL)" : "");
}

void ADBMS_2950_Print_Raw_ADC(adbms_raw_* adbms_raw)
{
    const int offset = NUM_6830 * DATA_LEN;  // 2950 slot in the daisy chain read-back
    uint8_t buf[NUM_CHIPS * DATA_LEN];
    pec_return_ pec;

    ADBMS_WakeUP_ICs();
    printf("\n--- ADBMS2950 ADC (t=%lu ms) ---\n", (unsigned long)HAL_GetTick());

#if PRINT_ALL_2950_GROUPS
    for (unsigned g = 0; g < sizeof(raw_groups_2950) / sizeof(raw_groups_2950[0]); g++)
    {
        const raw_group_ *grp = &raw_groups_2950[g];
        if (PRINT_2950_VBAT && grp->cmd == 0x0006) continue; // Printed in the VBAT section below.
        ADBMS_Read_Data(adbms_raw->SPI_data.hspi, grp->cmd, buf, adbms_raw->SPI_data.spi_dataBuf, &pec);
        print_raw_group(grp, &buf[offset], pec.pec_2950, 1.0f);
        if (PRINT_2950_V1V2 && grp->cmd == 0x000A)
        {
            float difference = (decode_signed(&buf[offset], 2) - decode_signed(&buf[offset + 2], 2)) * LSB_V_V;
            printf("V1-V2          %11.6f V%s\n", difference, pec.pec_2950 ? "  (PEC FAIL)" : "");
        }
    }
#endif

#if PRINT_2950_V1V2 && !PRINT_ALL_2950_GROUPS
    // RDV1A (0x000A): V1A bytes 0-1, V2A bytes 2-3, 100 uV/LSB (V1ADC). Needs ADV VCH = 9.
    // VS1 = VS2 = 00 (default): each measured vs SGND; difference computed here.
    ADBMS_Read_Data(adbms_raw->SPI_data.hspi, 0x000A, buf, adbms_raw->SPI_data.spi_dataBuf, &pec);
    float v1 = (float)decode_signed(&buf[offset + 0], 2) * LSB_V_V;
    float v2 = (float)decode_signed(&buf[offset + 2], 2) * LSB_V_V;
    const char *v_flag = pec.pec_2950 ? "  (PEC FAIL)" : "";
    printf("V1             %11.6f V%s\n", v1, v_flag);
    printf("V2             %11.6f V%s\n", v2, v_flag);
    printf("V1-V2          %11.6f V%s\n", v1 - v2, v_flag);
#endif

#if PRINT_2950_VBAT
    // RDVB (0x0006): VB1 in bytes 2-3 (100 uV/LSB), VB2 in bytes 4-5 (-85 uV/LSB, inverted gain).
    // VB1MUX = VB2MUX = 0 (default): VB1 = VBAT1 vs SGND, VB2 = VBAT2 vs SGND; difference computed here.
    ADBMS_Read_Data(adbms_raw->SPI_data.hspi, 0x0006, buf, adbms_raw->SPI_data.spi_dataBuf, &pec);
    float vbat1 = (float)decode_signed(&buf[offset + 2], 2) * LSB_VB1_V;
    float vbat2 = (float)decode_signed(&buf[offset + 4], 2) * LSB_VB2_V;
    const char *vb_flag = pec.pec_2950 ? "  (PEC FAIL)" : "";
    printf("VBAT1          %11.6f V%s\n", vbat1, vb_flag);
    printf("VBAT2          %11.6f V%s\n", vbat2, vb_flag);
    printf("VBAT1-VBAT2    %11.6f V%s\n", vbat1 - vbat2, vb_flag);

    // GPO1 drives the VBAT sense FET.
    // RDCFGA (0x0002): byte 3 bit 0 = GPO1C, byte 4 bit 0 = GPO1OD (configured)
    // RDSTAT (0x0034): byte 3 bit 2 = GPO1H, byte 4 bit 4 = GPO1L (actual pin level)
    ADBMS_Read_Data(adbms_raw->SPI_data.hspi, RDCFGA, buf, adbms_raw->SPI_data.spi_dataBuf, &pec);
    uint8_t gpo1c  = buf[offset + 3] & 0x01;
    uint8_t gpo1od = buf[offset + 4] & 0x01;
    bool gpo_cfg_pec = pec.pec_2950;
    ADBMS_Read_Data(adbms_raw->SPI_data.hspi, 0x0034, buf, adbms_raw->SPI_data.spi_dataBuf, &pec);
    uint8_t gpo1h = (buf[offset + 3] >> 2) & 0x01;
    uint8_t gpo1l = (buf[offset + 4] >> 4) & 0x01;
    const char *gpo1_pin = (gpo1h && gpo1l) ? "HIGH" : (!gpo1h && !gpo1l) ? "LOW" : "MID (between thresholds)";
    printf("GPO1 cfg       GPO1C=%u GPO1OD=%u (%s)%s\n", gpo1c, gpo1od,
           gpo1od ? (gpo1c ? "open-drain, hi-Z" : "open-drain, pull low") : (gpo1c ? "push-pull, drive high" : "push-pull, drive low"),
           gpo_cfg_pec ? "  (PEC FAIL)" : "");
    printf("GPO1 pin       %s (GPO1H=%u GPO1L=%u)%s\n", gpo1_pin, gpo1h, gpo1l, pec.pec_2950 ? "  (PEC FAIL)" : "");
#endif

#if PRINT_2950_THERMISTORS
    // ADV VCH = 9 converts V7 (V1ADC, VCH6) and V9/V10 (V2ADC, VCH6/VCH7).
    // VS7 = VS9 = VS10 = 0 (default): each measured vs SGND.
    // RDV1C (0x0003): V7A bytes 0-1, V8A bytes 2-3, 100 uV/LSB
    ADBMS_Read_Data(adbms_raw->SPI_data.hspi, 0x0003, buf, adbms_raw->SPI_data.spi_dataBuf, &pec);
    float v7 = (float)decode_signed(&buf[offset + 0], 2) * LSB_V_V;
    bool v7_pec = pec.pec_2950;

    // RDV2C (0x0005): V9B bytes 0-1, V10B bytes 2-3, -85 uV/LSB (V2ADC inverted gain)
    ADBMS_Read_Data(adbms_raw->SPI_data.hspi, 0x0005, buf, adbms_raw->SPI_data.spi_dataBuf, &pec);
    float v9  = (float)decode_signed(&buf[offset + 0], 2) * LSB_V2ADC;
    float v10 = (float)decode_signed(&buf[offset + 2], 2) * LSB_V2ADC;

    print_thermistor("V7 ", v7,  v7_pec);
    print_thermistor("V9 ", v9,  pec.pec_2950);
    print_thermistor("V10", v10, pec.pec_2950);

#endif

#if PRINT_2950_OC
    // Overcurrent status
    // RDOC (0x000B):   byte0 OC1R, byte1 OC2R, byte2 OC3R (signed 8-bit, x OCxLSB)
    // RDFLAG (0x0032): byte0 bit0 OC1L, bit1 OCAL, bit3 OC3L; byte1 bit0 OC2L, bit1 OCBL (latches)
    // RDSTAT (0x0034): byte0 bit0 OCAP, bit1 OCBP (actual OCA/OCB pin state)
    ADBMS_Read_Data(adbms_raw->SPI_data.hspi, 0x000B, buf, adbms_raw->SPI_data.spi_dataBuf, &pec);
    float oc1 = (int8_t)buf[offset + 0] * 0.0025f;   // OC1GC = 1 -> 2.5 mV
    float oc2 = (int8_t)buf[offset + 1] * 0.0025f;   // OC2GC = 1 -> 2.5 mV
    float oc3 = (int8_t)buf[offset + 2] * 0.0025f;   // OC3GC = 1 -> 2.5 mV
    bool oc_pec = pec.pec_2950;

    ADBMS_Read_Data(adbms_raw->SPI_data.hspi, 0x0032, buf, adbms_raw->SPI_data.spi_dataBuf, &pec);
    uint8_t oc1l = buf[offset + 0] & 0x01, ocal = (buf[offset + 0] >> 1) & 0x01, oc3l = (buf[offset + 0] >> 3) & 0x01;
    uint8_t oc2l = buf[offset + 1] & 0x01, ocbl = (buf[offset + 1] >> 1) & 0x01;
    uint8_t ocagd = (buf[offset + 0] >> 2) & 0x01, ocbgd = (buf[offset + 1] >> 2) & 0x01;   // pin state != intended output
    uint8_t ocmm  = (buf[offset + 0] >> 4) & 0x01;
    uint8_t flag0 = buf[offset + 0], flag1 = buf[offset + 1], flag4 = buf[offset + 4], flag5 = buf[offset + 5];
    oc_pec |= pec.pec_2950;

    // Config actually held by the chip
    // RDCFGA (0x0002): byte0 bit7 OCEN
    // RDCFGB (0x0026): byte0..2 OCxTH, byte3 bit3 OCDP bits1:0 OCDGT,
    //                  byte4 bit7 OCBX bit6 OCAX bits5:4 OCMODE bit3..1 OC3GC..OC1GC bit0 OCOD
    ADBMS_Read_Data(adbms_raw->SPI_data.hspi, RDCFGA, buf, adbms_raw->SPI_data.spi_dataBuf, &pec);
    uint8_t ocen = (buf[offset + 0] >> 7) & 0x01;
    bool cfg_pec = pec.pec_2950;
    ADBMS_Read_Data(adbms_raw->SPI_data.hspi, RDCFGB, buf, adbms_raw->SPI_data.spi_dataBuf, &pec);
    cfg_pec |= pec.pec_2950;
    uint8_t b4 = buf[offset + 4];
    printf("OC cfg  OCEN=%u OCMODE=%u%u OCOD=%u OCAX=%u OCBX=%u TH=%u/%u/%u GC=%u%u%u DGT=%u DP=%u%s\n",
           ocen, (b4 >> 5) & 1, (b4 >> 4) & 1, b4 & 1, (b4 >> 6) & 1, (b4 >> 7) & 1,
           buf[offset + 0] & 0x7F, buf[offset + 1] & 0x7F, buf[offset + 2] & 0x7F,
           (b4 >> 3) & 1, (b4 >> 2) & 1, (b4 >> 1) & 1, buf[offset + 3] & 0x03, (buf[offset + 3] >> 3) & 1,
           cfg_pec ? "  (PEC FAIL)" : "");
    printf("OC diag OCAGD=%u OCBGD=%u OCMM=%u   (GD=1: pin state differs from intended output)\n", ocagd, ocbgd, ocmm);
    // Any of these set -> OCA/OCB forced to fault state (low)
    printf("Faults  VDDUV=%u NOCLK=%u REFFLT=%u VREGOV=%u VREGUV=%u VDIGOV=%u VDIGUV=%u MED1=%u MED2=%u VDE=%u TMODE=%u OSCFLT=%u THSD=%u\n",
           (flag1 >> 5) & 1, (flag1 >> 4) & 1, (flag1 >> 3) & 1,
           (flag4 >> 7) & 1, (flag4 >> 6) & 1, (flag4 >> 5) & 1, (flag4 >> 4) & 1, (flag4 >> 2) & 1, (flag4 >> 0) & 1,
           (flag5 >> 6) & 1, (flag5 >> 1) & 1, (flag5 >> 0) & 1, (flag5 >> 2) & 1);
    (void)flag0;

    ADBMS_Read_Data(adbms_raw->SPI_data.hspi, 0x0034, buf, adbms_raw->SPI_data.spi_dataBuf, &pec);
    uint8_t ocap = buf[offset + 0] & 0x01, ocbp = (buf[offset + 0] >> 1) & 0x01;
    oc_pec |= pec.pec_2950;

    printf("OC1 (I1A-I1B) %8.4f V  latch=%u\n", oc1, oc1l);
    printf("OC2 (I2A-I2B) %8.4f V  latch=%u\n", oc2, oc2l);
    printf("OC3 (I3A-I3B) %8.4f V  latch=%u\n", oc3, oc3l);
    printf("OCA pin %s (latch=%u)   OCB pin %s (latch=%u)%s\n",
           ocap ? "HIGH" : "LOW", ocal, ocbp ? "HIGH" : "LOW", ocbl, oc_pec ? "  (PEC FAIL)" : "");
#endif

    // ADV is single-shot: retrigger so the V registers are fresh on the next loop
    ADBMS_Write_CMD(adbms_raw->SPI_data.hspi, adbms_raw->command_bit.adv);
}

// Prints the I1A - I1B differential voltage (I1 ADC is 1uV/LSB, accumulated over accn samples)
void ADBMS_2950_Print_I1_Diff(adbms_2950_* adbms_2950, uint8_t acci)
{
    float accn = (float) 4 * acci + 4;
    float i1_diff_uv = (float) adbms_2950->raw_data.i1_raw / accn;
    printf("I1A-I1B: %f uV\n", i1_diff_uv);
}



//Previous transfer functions
// float ADBMS_2950_Transfer_Current(int32_t data)
// {
//   float current;
//   //the actual measured resistance is closer to 94 microolms instead of 100
//   //its measured in micro volts
//   current = (float) data / 94.0;
//   return current;
// }


// float ADBMS_2950_Transfer_Vbat(int32_t vbat1_raw, int32_t vbat2_raw){

//     //fix this transfer function

//     float vbat_final = 0.0;

//     //transfer to real value and store
//     float vbat1 = (float)(vbat1_raw /  0.00414937759); // (15000/3,600,000 + 15000)
//     float vbat2 = (float) vbat2_raw;

//     vbat_final = (vbat1-vbat2) * 0.0001; //100 microolms
    
//     return vbat_final;
// }

//
// Post Contactor Voltage No longe exists
//

//void ADBMS_2950_Calculate_Post_Voltage(adbms_raw_* adbms_raw, adbms_2950_* adbms_2950)
// {

//     //initialize values
//     adbms_2950->raw_data.v1_raw = 0;
//     adbms_2950->raw_data.v2_raw = 0;

//     //RDCVD is the 4th command that we send that get puts into cell
//     int command_offset = (DATA_LEN * NUM_CHIPS) * 3;
//     //offset because 2950 is the lsat chip on the daisy chain
//     int offset = (NUM_6830) * DATA_LEN;

//     //getting raw data from cell readings
//     adbms_2950->raw_data.v1_raw = (int16_t)(adbms_raw->raw_voltages.c_channel_raw[3+offset+command_offset] << 8) | (int16_t)(adbms_raw->raw_voltages.c_channel_raw[2+offset+command_offset]);
//     adbms_2950->raw_data.v2_raw = (int16_t)(adbms_raw->raw_voltages.c_channel_raw[5+offset+command_offset] << 8) | (int16_t)(adbms_raw->raw_voltages.c_channel_raw[1+offset+command_offset]);

//     adbms_2950->data.postcontactor_voltage = ADBMS_2950_Transfer_Post_Voltage(adbms_2950->raw_data.v1_raw, adbms_2950->raw_data.v2_raw);
// }

// float ADBMS_2950_Transfer_Post_Voltage(int16_t v1_raw, int16_t v2_raw){

//     float v_TS = 0.0;

//     float v1 = (float) (v1_raw / 0.0041938); // (15000/3,600,000 + 15000)
//     float v2 = (float) (v2_raw);

//     v_TS = (v1-v2) * 0.0001; //100 microolms
    
//     return v_TS;
// }
