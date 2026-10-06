/*
 * RCC.c
 *
 *  Created on: Aug 3, 2026
 *      Author: MadaYaswanth
 */

#include"main.h"
/*******************************************************************************
 * Function Name : system_clock_init_to_72MHZ
 * Description   : Initializes system clock to 72 MHz using external crystal (HSE)
 *                 and PLL x9, configures Flash latency to 2 wait states with
 *                 prefetch buffer enabled, and sets APB1 prescaler to div-by-2.
 * Scope         : Static (Private to this file)
 * Parameters    : None
 * Return Value  : None
 ******************************************************************************/
static void system_clock_init_to_72MHZ(){
	    RCC->CR |= RCC_CR_HSEON;
	    while (!(RCC->CR & RCC_CR_HSERDY));
	    FLASH->ACR |= FLASH_ACR_LATENCY_2;
	    FLASH->ACR |= FLASH_ACR_PRFTBE;
	    RCC->CFGR |= RCC_CFGR_PLLMULL9;
	    RCC->CFGR |= RCC_CFGR_PLLSRC;
	    RCC->CFGR |= RCC_CFGR_PPRE1_2;
	    RCC->CFGR &= ~RCC_CFGR_PPRE2;
	    RCC->CR |= RCC_CR_PLLON;
	    while (!(RCC->CR & RCC_CR_PLLRDY));
	    RCC->CFGR |= RCC_CFGR_SW_PLL;
	    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}
uint32_t Get_SYSCLK_freq(void) {
    uint32_t sysclk_source, pllmul, prediv, pllclk;
    uint32_t HSE_Value = 8000000;
    uint32_t HSI_Value = 8000000;
    sysclk_source = (RCC->CFGR >> 2) & 0x3;
    switch (sysclk_source) {
        case 0x00:
            return HSI_Value;
        case 0x01:
            return HSE_Value;
        case 0x02:
            if (RCC->CFGR & (1 << 16)) {
                if (RCC->CFGR & (1 << 17))
                    prediv = 2;
                else
                    prediv = 1;
                pllclk = HSE_Value / prediv;
            } else {
                pllclk = HSI_Value / 2;
            }

            pllmul = ((RCC->CFGR >> 18) & 0xF) + 2;
            if (pllmul == 15) pllmul = 16;

            return pllclk * pllmul;
        default:
            return HSI_Value;
    }
}
uint32_t Get_AHB_freq(void) {
    uint32_t presc_table[16] = {1,1,1,1,1,1,1,1,2,4,8,16,64,128,256,512};
    uint32_t hpre = (RCC->CFGR >> 4) & 0xF;
    return Get_SYSCLK_freq() / presc_table[hpre];
}

uint32_t Get_APB1_freq(void) {
    uint32_t presc_table[8] = {1,1,1,1,2,4,8,16};
    uint32_t ppre1 = (RCC->CFGR >> 8) & 0x7;
    return Get_AHB_freq() / presc_table[ppre1];
}
uint32_t Get_APB2_freq(void) {
    uint32_t presc_table[8] = {1,1,1,1,2,4,8,16};
    uint32_t ppre2 = (RCC->CFGR >> 11) & 0x7;
    return Get_AHB_freq() / presc_table[ppre2];
}
void Stm32f103_System_Clock_Init(void)
{
	system_clock_init_to_72MHZ();
}
