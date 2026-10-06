/*
 * RCC.h
 *
 *  Created on: Aug 3, 2026
 *      Author: MadaYaswanth
 */

#ifndef INC_RCC_H_
#define INC_RCC_H_


#include"main.h"

#define CLOCK_8MHZ   8000000U
#define CLOCK_36MHZ  36000000U
/*******************************************************************************
 * Function Name : Stm32f103_System_Clock_Init
 * Description   : Public entry wrapper to configure system clock tree for target
 *                 STM32F103 operating frequency (72 MHz).
 * Scope         : Global / Public
 * Parameters    : None
 * Return Value  : None
 ******************************************************************************/
void Stm32f103_System_Clock_Init(void);
/*******************************************************************************
 * Function Name : Get_SYSCLK_freq
 * Description   : Calculates and returns current core System Clock (SYSCLK)
 *                 frequency in Hertz based on active source (HSI, HSE, or PLL).
 * Scope         : Global / Public
 * Parameters    : None
 * Return Value  : uint32_t (SYSCLK frequency in Hz)
 ******************************************************************************/
uint32_t Get_SYSCLK_freq(void);
/*******************************************************************************
 * Function Name : Get_AHB_freq
 * Description   : Calculates and returns current Advanced High-Performance Bus
 *                 (AHB/HCLK) frequency in Hertz by applying the HPRE prescaler.
 * Scope         : Global / Public
 * Parameters    : None
 * Return Value  : uint32_t (AHB bus frequency in Hz)
 ******************************************************************************/
uint32_t Get_AHB_freq(void) ;
/*******************************************************************************
 * Function Name : Get_APB1_freq
 * Description   : Calculates and returns current Low-Speed APB1 bus (PCLK1)
 *                 frequency in Hertz by applying the PPRE1 prescaler.
 * Scope         : Global / Public
 * Parameters    : None
 * Return Value  : uint32_t (APB1 bus frequency in Hz)
 ******************************************************************************/
uint32_t Get_APB1_freq(void);
/*******************************************************************************
 * Function Name : Get_APB2_freq
 * Description   : Calculates and returns current High-Speed APB2 bus (PCLK2)
 *                 frequency in Hertz by applying the PPRE2 prescaler.
 * Scope         : Global / Public
 * Parameters    : None
 * Return Value  : uint32_t (APB2 bus frequency in Hz)
 ******************************************************************************/
uint32_t Get_APB2_freq(void) ;



#endif /* INC_RCC_H_ */
