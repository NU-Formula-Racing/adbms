//Responds to ECU commands and manages contactors
#include "control.h"

void control_loop(mainboard_ *mainboard)
{
    if (mainboard->state == Charge)
    {
        charger_control(&mainboard);
    }
    else
    {
        if (update_faults(&mainboard) == true)
        {
            ecu_perepheral(&mainboard);
        }
    }
}


void ecu_perepheral(mainboard_ *mainboard)
{
    if (mainboard->ecu_command == go_to_idle)
    {
		HAL_GPIO_WritePin(GPIOA, Contactor_N_Ctrl_GPIO_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, Contactor_P_Ctrl_GPIO_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, Contactor_Pre_Ctrl_GPIO_Pin, GPIO_PIN_RESET);
        mainboard->state = Idle;
        mainboard->ecu_command = waiting;
	}
	else if (mainboard->ecu_command == go_to_active)
    { 
        if (mainboard->state == Idle)
        {
            HAL_GPIO_WritePin(GPIOA, Contactor_P_Ctrl_GPIO_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOA, Contactor_Pre_Ctrl_GPIO_Pin, GPIO_PIN_SET);
            mainboard->state = Precharge;
            mainboard->ecu_command = waiting;
        }
        else if (mainboard->state == Precharge)
        {
            float inverter_v = mainboard->Inverter_DC_Voltage;
            float total_pack_v = mainboard->adbms.total_v;
            float percent_precharged = inverter_v / total_pack_v;
            if(percent_precharged > INVERTER_VOLTAGE_THRESHOLD)
            {
                HAL_GPIO_WritePin(GPIOA, Contactor_Pre_Ctrl_GPIO_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOA, Contactor_N_Ctrl_GPIO_Pin, GPIO_PIN_SET);
                mainboard->state = Active;
                mainboard->ecu_command = waiting;
            }
            else
            {
                mainboard->ecu_command = waiting;
            }
        }
	}
}

//check for fault and update if needed, return bool 
bool update_faults(mainboard_ *mainboard)
{
    if (mainboard->state == Fault)
    {
        return false;
    }
    if (mainboard->bms_fault == true)
    {
        mainboard->state == Fault;
        HAL_GPIO_WritePin(GPIOA, Contactor_N_Ctrl_GPIO_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, Contactor_P_Ctrl_GPIO_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, Contactor_Pre_Ctrl_GPIO_Pin, GPIO_PIN_RESET);
        return false;
    }
    if (mainboard->external_fault == true)
    {
        mainboard->state = Idle;
        HAL_GPIO_WritePin(GPIOA, Contactor_N_Ctrl_GPIO_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, Contactor_P_Ctrl_GPIO_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, Contactor_Pre_Ctrl_GPIO_Pin, GPIO_PIN_RESET);
        return false;
    }
    return true;
}

void charger_control(mainboard_ *mainboard)
{
    if (mainboard->bms_fault == true)
    {
        mainboard->state = Fault;
        HAL_GPIO_WritePin(GPIOA, Contactor_N_Ctrl_GPIO_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, Contactor_P_Ctrl_GPIO_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, Contactor_Pre_Ctrl_GPIO_Pin, GPIO_PIN_RESET);
    }
    else
    {
        if(!mainboard->charger_messege->fault) //if actively connected to charger
        {
            switch (mainboard->charging_state) 
            {
            case charger_setup:
                HAL_GPIO_WritePin(GPIOA, Contactor_P_Ctrl_GPIO_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOA, Contactor_Pre_Ctrl_GPIO_Pin, GPIO_PIN_SET);
                mainboard->charging_state = charger_precharge;
                break;
            case charger_precharge:
                float percent_precharged = mainboard->charger_voltage / mainboard->adbms.total_v;
                if (percent_precharged > CHARGER_VOLTAGE_THRESHOLD)
                {
                    HAL_GPIO_WritePin(GPIOA, Contactor_Pre_Ctrl_GPIO_Pin, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(GPIOA, Contactor_N_Ctrl_GPIO_Pin, GPIO_PIN_SET);
                    mainboard->charging_state = charger_active;
                }
                break;
            case charger_active:
                break;
            }
        }
    }
}


