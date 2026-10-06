/*
 * SYSTICK.h
 *
 *  Created on: Aug 6, 2026
 *      Author: MadaYaswanth
 */

#ifndef INC_SYSTICK_H_
#define INC_SYSTICK_H_

#include"main.h"
/*******************************************************************************
 * Function Name : Core_Timer_Init
 * Description   : Public wrapper function to initialize the core SysTick timer
 *                 for system millisecond timekeeping.
 * Scope         : Global / Public
 * Parameters    : None
 * Return Value  : bool (true = SysTick initialized, false = Configuration failed)
 ******************************************************************************/
bool Core_Timer_Init();
/*******************************************************************************
 * Function Name : Get_Tick_Ms
 * Description   : Retrieves the global millisecond counter value incremented by
 *                 the SysTick interrupt service routine.
 * Scope         : Global / Public
 * Parameters    : None
 * Return Value  : uint32_t (Current system uptime tick count in milliseconds)
 ******************************************************************************/

uint32_t Get_Tick_Ms(void);
#endif /* INC_SYSTICK_H_ */
