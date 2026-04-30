//minimum changes to this new version, mostly the exact same

//Manages contactors
#include "bms_control.h"

void Control_Loop(mainboard_ *mainboard)
{
   if (mainboard->internal_state == Charging)
   {
       charger_control(mainboard);
   }
   else
   {
       if (check_fault_status(mainboard) == true)
       {
           in_car(mainboard);
       }
   }
}


void in_car(mainboard_ *mainboard)
{
    switch (mainboard->vcu_state_request) {
    case car_idle:
        HAL_GPIO_WritePin(GPIOB, CONTACTOR_N_CTRL_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, CONTACTOR_P_CTRL_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, CONTACTOR_PRE_CTRL_Pin, GPIO_PIN_RESET);
        mainboard->internal_state = Idle;
        break;
    case car_precharge:
        if (mainboard->internal_state == Idle)
        {
            HAL_GPIO_WritePin(GPIOB, CONTACTOR_P_CTRL_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOB, CONTACTOR_N_CTRL_Pin, GPIO_PIN_SET);
		    HAL_GPIO_WritePin(GPIOB, CONTACTOR_PRE_CTRL_Pin, GPIO_PIN_SET);
            mainboard->internal_state = Precharge;

        }
        else if (mainboard->internal_state == Precharge)
        {
            float inverter_v = mainboard->Inverter_DC_Voltage;
            //new change to 6830
            float total_pack_v = mainboard->adbms_6830.voltage.total_v;
            float percent_precharged = inverter_v / total_pack_v;
            if(percent_precharged > INVERTER_VOLTAGE_THRESHOLD)
            {
                HAL_GPIO_WritePin(GPIOB, CONTACTOR_PRE_CTRL_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOB, CONTACTOR_P_CTRL_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOB, CONTACTOR_N_CTRL_Pin, GPIO_PIN_SET);
                mainboard->internal_state = Active;
            }
        }
        break;
    case car_neutral:
        //VCU in netutral after BMS is in Active
        break;
    case car_drive:
        // BMS Active
        break;
    case car_fault:
        HAL_GPIO_WritePin(GPIOB, CONTACTOR_N_CTRL_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, CONTACTOR_P_CTRL_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, CONTACTOR_PRE_CTRL_Pin, GPIO_PIN_RESET);
        mainboard->internal_state = Idle;
        break;
    }
}

//check for fault and update if needed, return bool
bool check_fault_status(mainboard_ *mainboard)
{
   if ((mainboard->internal_state == Fault) || mainboard->bms_fault || !mainboard->imd_status)
   {
        mainboard->internal_state = Fault;
        HAL_GPIO_WritePin(GPIOB, CONTACTOR_N_CTRL_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, CONTACTOR_P_CTRL_Pin, GPIO_PIN_RESET);
	    HAL_GPIO_WritePin(GPIOB, CONTACTOR_PRE_CTRL_Pin, GPIO_PIN_RESET);

        return false;
   }
   else if (mainboard->external_fault)
   {
        mainboard->internal_state = Idle;
        HAL_GPIO_WritePin(GPIOB, CONTACTOR_N_CTRL_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, CONTACTOR_P_CTRL_Pin, GPIO_PIN_RESET);
	    HAL_GPIO_WritePin(GPIOB, CONTACTOR_PRE_CTRL_Pin, GPIO_PIN_RESET);
        return false;
   }
   else
   {
        return true;
   }
   
}


void charger_control(mainboard_ *mainboard)
{
    if ((mainboard->internal_state == Fault) || mainboard->bms_fault || !mainboard->imd_status)
    {
        mainboard->internal_state = Fault;
        HAL_GPIO_WritePin(GPIOB, CONTACTOR_P_CTRL_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, CONTACTOR_PRE_CTRL_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, CONTACTOR_N_CTRL_Pin, GPIO_PIN_RESET);
    }
    else
    {
        if(!mainboard->charger_timeout) //if actively connected to charger
        {
            switch (mainboard->charging_state)
            {
                case charger_setup:
                    HAL_GPIO_WritePin(GPIOB, CONTACTOR_P_CTRL_Pin, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(GPIOB, CONTACTOR_N_CTRL_Pin, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(GPIOB, CONTACTOR_PRE_CTRL_Pin, GPIO_PIN_SET);
                    mainboard->charging_state = charger_precharge;
                    break;
                case charger_precharge:
                    float percent_precharged = mainboard->charger_voltage / mainboard->adbms_6830.voltage.total_v;
                    if (percent_precharged > CHARGER_VOLTAGE_THRESHOLD)
                    {
                        HAL_GPIO_WritePin(GPIOB, CONTACTOR_PRE_CTRL_Pin, GPIO_PIN_RESET);
                        HAL_GPIO_WritePin(GPIOB, CONTACTOR_P_CTRL_Pin, GPIO_PIN_SET);
                        HAL_GPIO_WritePin(GPIOB, CONTACTOR_N_CTRL_Pin, GPIO_PIN_SET);
                        mainboard->charging_state = charger_active;
                    }
                    break;
                case charger_active:
                    break;
            }
        }
        else
        {
            mainboard->internal_state = Idle;
            mainboard->charging_state = charger_setup;
            HAL_GPIO_WritePin(GPIOB, CONTACTOR_P_CTRL_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOB, CONTACTOR_PRE_CTRL_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOB, CONTACTOR_N_CTRL_Pin, GPIO_PIN_RESET);
      }
    }
}