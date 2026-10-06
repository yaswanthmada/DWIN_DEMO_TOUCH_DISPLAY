/*
 * SYSTICK.c
 *
 *  Created on: Aug 6, 2026
 *      Author: MadaYaswanth
 */


#include"SYSTICK.h"
#include"RCC.h"
/*******************************************************************************
 * Function Name : Systick_Timer_Init
 * Description   : Configures and enables the ARM Cortex-M SysTick timer to
 *                 generate a periodic interrupt every 1 millisecond based on
 *                 the current AHB bus clock frequency.
 * Scope         : Static (Private to this file)
 * Parameters    : None
 * Return Value  : bool (true = Initialization successful, false = Reload value exceeds 24-bit max)
 ******************************************************************************/
static bool Systick_Timer_Init(void)
{
    uint32_t Reload_Val;
    Reload_Val = (Get_AHB_freq() / 1000U) - 1U;
    if (Reload_Val > SysTick_LOAD_RELOAD_Msk)
    {
        return false;
    }
    SysTick->CTRL = 0;
    SysTick->LOAD = Reload_Val;
    SysTick->VAL = 0;
    SysTick->CTRL =
        SysTick_CTRL_CLKSOURCE_Msk |
        SysTick_CTRL_TICKINT_Msk   |
        SysTick_CTRL_ENABLE_Msk;
    return true;
}
uint32_t Get_Tick_Ms(void)
{
    return Get_Ms_Ticks;
}

bool Core_Timer_Init()
{
	return Systick_Timer_Init();
}
