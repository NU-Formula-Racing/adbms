//Responds to ECU commands and manages contactors
#include "bms_control.h"

void Control_Loop(mainboard_ *mainboard)
{
   if (mainboard->state == Charging)
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
   if (mainboard->ecu_precharge)
   {
         if (mainboard->state == Idle)
       {
            HAL_GPIO_WritePin(GPIOB, CONTACTOR_P_CTRL_Pin, GPIO_PIN_SET);
		    HAL_GPIO_WritePin(GPIOB, CONTACTOR_PRE_CTRL_Pin, GPIO_PIN_SET);
            mainboard->state = Precharge;
       }
	}
	else if (mainboard->ecu_neutral)
    {
        if (mainboard->state == Precharge)
       {
           float inverter_v = mainboard->Inverter_DC_Voltage;
           float total_pack_v = mainboard->adbms.total_v;
           float percent_precharged = inverter_v / total_pack_v;
           if(percent_precharged > INVERTER_VOLTAGE_THRESHOLD)
           {
               HAL_GPIO_WritePin(GPIOB, CONTACTOR_PRE_CTRL_Pin, GPIO_PIN_RESET);
               HAL_GPIO_WritePin(GPIOB, CONTACTOR_N_CTRL_Pin, GPIO_PIN_SET);
               mainboard->state = Neutral;
           }
       }
	}
    else
    {
        HAL_GPIO_WritePin(GPIOB, CONTACTOR_N_CTRL_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, CONTACTOR_P_CTRL_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, CONTACTOR_PRE_CTRL_Pin, GPIO_PIN_RESET);
        mainboard->state = Idle;
    }
}

//check for fault and update if needed, return bool
bool check_fault_status(mainboard_ *mainboard)
{
   if ((mainboard->state == Fault) || mainboard->bms_fault || mainboard->external_fault)
   {
       mainboard->state = Fault;
       HAL_GPIO_WritePin(GPIOB, CONTACTOR_P_CTRL_Pin, GPIO_PIN_RESET);
       HAL_GPIO_WritePin(GPIOB, CONTACTOR_PRE_CTRL_Pin, GPIO_PIN_RESET);
       HAL_GPIO_WritePin(GPIOB, CONTACTOR_N_CTRL_Pin, GPIO_PIN_RESET);

       return false;
   }
   else
   {
        return true;
   }
   
}

void charger_control(mainboard_ *mainboard)
{
   if (mainboard->bms_fault == true)
   {
       mainboard->state = Fault;
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
                    HAL_GPIO_WritePin(GPIOB, CONTACTOR_P_CTRL_Pin, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(GPIOB, CONTACTOR_PRE_CTRL_Pin, GPIO_PIN_SET);
                    mainboard->charging_state = charger_precharge;
                    break;
                case charger_precharge:
                    float percent_precharged = mainboard->charger_voltage / mainboard->adbms.total_v;
                    if (percent_precharged > CHARGER_VOLTAGE_THRESHOLD)
                    {
                        
                        HAL_GPIO_WritePin(GPIOB, CONTACTOR_PRE_CTRL_Pin, GPIO_PIN_RESET);
                        HAL_GPIO_WritePin(GPIOB, CONTACTOR_N_CTRL_Pin, GPIO_PIN_SET);
                        HAL_GPIO_WritePin(GPIOB, CONTACTOR_P_CTRL_Pin, GPIO_PIN_SET);
                        mainboard->charging_state = charger_active;
                    }
                    break;
                case charger_active:
                    break;
            }
        }
    }
}
