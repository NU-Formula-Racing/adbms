#include "bms_soc.h"
#include <math.h>

/*EKF Parameters*/
#define EKF_R0      0.015f   //internal resistance in ohms
#define EKF_R1      0.005f   //RC resistance in ohms
#define EKF_TAU     20.0f    //RC time constant in seconds

#define Q_SOC       0.00001f //SOC process noise variance
#define Q_VR1       0.001f   //VR1 process noise variance
#define R_MEAS      0.05f    //voltage measurement noise variance

static float Get_OCV(float soc) {
    // TODO: Map SOC to VOC
    return 1.0f; 
}

static float Get_dOCV_dSOC(float soc) {
    // TODO: Slope of SOC to VOC mapping
    return 1.0f; 
}

void Soc_Initialize(mainboard_ *mainboard)
{
    ADBMS_UpdateVoltages(&mainboard->adbms);
    float avg_v = mainboard->adbms.avg_v;
    
    //initialize SOC with OCV
    int vidx1 = fmax(0, ((int)floor((avg_v - 2.5) * 1000.0 / 4.0)));
    int vidx2 = fmin(500, ((int)ceil((avg_v - 2.5) * 1000.0 / 4.0)));
    float v = (v2a_lookup[vidx2] - v2a_lookup[vidx1]) * ((avg_v - 2.5) * 1000.0 - floor((avg_v - 2.5) * 1000.0)) + v2a_lookup[vidx1];
    
    mainboard->soc = fmax(0, 4.5 - v / 1000.0) * 3.0 / (SYSTEM_CAPACITY);
    mainboard->prev_time = HAL_GetTick();

    
    mainboard->vr1 = 0.0f; //open circuit
    
    //init covariance matrix
    mainboard->P[0][0] = 0.5f; //very confident in OCV SOC
    mainboard->P[0][1] = 0.0f; //no relation between variables
    mainboard->P[1][0] = 0.0f;
    mainboard->P[1][1] = 0.5f; //very confident at open circuit
}

void Soc_Update(mainboard_ *mainboard)
{
    float tick = HAL_GetTick();
    float delta_sec = (tick - mainboard->prev_time) / 1000.0f; 
    float delta_hours = delta_sec / 3600.0f;
    mainboard->prev_time = tick;
    float current = mainboard->current; //positive current = discharge

    /*Predict step*/
    float soc_pred = mainboard->soc - (delta_hours * current) / (SYSTEM_CAPACITY);//coulomb counting
    //RC model prediction
    float exp_decay = expf(-delta_sec / EKF_TAU);
    float vr1_pred = (mainboard->vr1 * exp_decay) + (EKF_R1 * (1.0f - exp_decay) * current);

    //covariance prediction A * P * A^T + Q, matrix math unrolled for sumble 2x2, A is linear
    float P00_pred = mainboard->P[0][0] + Q_SOC;
    float P01_pred = mainboard->P[0][1] * exp_decay;
    float P10_pred = mainboard->P[1][0] * exp_decay;
    float P11_pred = (mainboard->P[1][1] * exp_decay * exp_decay) + Q_VR1;

    //  
    float ocv_pred = Get_OCV(soc_pred);
    float docv_dsoc = Get_dOCV_dSOC(soc_pred); //Non-liniar
    float v_term_pred = ocv_pred - vr1_pred - (current * EKF_R0);

    /*Correction step*/  

    // Error covariance time update
    float S = (docv_dsoc * P00_pred * docv_dsoc) - (docv_dsoc * P01_pred) - (P10_pred * docv_dsoc) + P11_pred + R_MEAS; 

    //Kalman gain
    float K0 = (P00_pred * docv_dsoc - P01_pred) / S;
    float K1 = (P10_pred * docv_dsoc - P11_pred) / S;

    //Update States
    float innovation = mainboard->adbms.avg_v - v_term_pred; //innovation of RC voltage
    mainboard->soc = soc_pred + (K0 * innovation);
    mainboard->vr1 = vr1_pred + (K1 * innovation);

    //Update Covariance
    mainboard->P[0][0] = P00_pred - K0 * (docv_dsoc * P00_pred - P10_pred);
    mainboard->P[0][1] = P01_pred - K0 * (docv_dsoc * P01_pred - P11_pred);
    mainboard->P[1][0] = P10_pred - K1 * (docv_dsoc * P00_pred - P10_pred);
    mainboard->P[1][1] = P11_pred - K1 * (docv_dsoc * P01_pred - P11_pred);

    //limit SOC
    mainboard->soc = fmaxf(0.0f, fminf(1.0f, mainboard->soc));

    /*DCIR and max discharge update*/
    int curridx1 = fmin(fmax(0, (int)floor((SYSTEM_CAPACITY * 1000.0 / 3.0 - (mainboard->soc * SYSTEM_CAPACITY)) * 1000.0 / 3.0 / 6.0)), 4500);
    int curridx2 = fmin(fmax(0, (int)ceil((SYSTEM_CAPACITY * 1000.0 / 3.0 - (mainboard->soc * SYSTEM_CAPACITY)) * 1000.0 / 3.0 / 6.0)), 4500);
    mainboard->dcir = (a2r_lookup[curridx2] - a2r_lookup[curridx1]) * (((SYSTEM_CAPACITY * 1000.0 / 3.0 - (mainboard->soc * SYSTEM_CAPACITY)) * 1000.0 / 3.0) - floor((SYSTEM_CAPACITY - (mainboard->soc * SYSTEM_CAPACITY)) * 1000.0 / 3.0)) + a2r_lookup[curridx1];

    float delta_i = (mainboard->adbms.avg_v - UNDERVOLTAGE) / (mainboard->dcir * 1000.0f) * PARALLEL; // convert mili-ohms to ohms
    mainboard->max_discharge_current = fmin(OVERCURRENT, mainboard->max_discharge_current + delta_i);
}