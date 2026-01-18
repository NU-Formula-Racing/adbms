#include "virtual_timer.h"

// Creates a new timer
timer_ Create_Timer(uint32_t duration_ms, Callback cb)
{
	uint32_t tickstart = HAL_GetTick();
	timer_ timer = {tickstart, duration_ms, cb};
	return timer;
}

timer_group_ *Create_Timer_Group(timer_ timers[NUM_TIMERS])
{
	// Allocate the timer group structure
	timer_group_ *tg = (timer_group_ *)malloc(sizeof(timer_group_));
	if (!tg)
	{
		return NULL;
	}

	memcpy(tg->timers, timers, sizeof(timer_) * NUM_TIMERS);
	return tg;
}

// Ticks a timer group
void Tick_Timer_Group(timer_group_ *tg)
{
	for (uint8_t i = 0; i < NUM_TIMERS; i++)
	{
		// HAL_GetTick is in ms
		// will error with a HAL_GetTick overflow
		if ((HAL_GetTick() - tg->timers[i].tickstart) > tg->timers[i].durations_ms)
		{
			tg->timers[i].tickstart = HAL_GetTick();
			tg->timers[i].cb();
		}
	}
}

void Tick_Charger_Timer(timer_group_ *tg)
{
	if ((HAL_GetTick() - tg->timers[0].tickstart) > tg->timers[0].durations_ms)
	{
		tg->timers[0].tickstart = HAL_GetTick();
		tg->timers[0].cb();
	}
}
