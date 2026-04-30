#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stm32h7xx_hal.h"
#include "bms_system_prams.h"

typedef void (*Callback)();

typedef struct
{
    uint32_t tickstart;
    uint32_t durations_ms;
    Callback cb;
} timer_;

typedef struct
{
    timer_ timers[NUM_TIMERS]; // Array of timers
} timer_group_;

timer_ Create_Timer(uint32_t duration_ms, Callback cb);

timer_group_ *Create_Timer_Group(timer_ timers[NUM_TIMERS]);

void Tick_Timer_Group(timer_group_ *tg);

void Tick_Charger_Timer(timer_group_ *tg);
