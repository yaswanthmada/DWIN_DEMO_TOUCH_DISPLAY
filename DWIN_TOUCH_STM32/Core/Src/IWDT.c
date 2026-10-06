/*
 * IWDT.c
 *
 *  Created on: Jul 15, 2026
 *      Author: MadaYaswanth
 */


#include"main.h"
#include"IWDT.h"
/**
 * @brief  Initialize Independent Watchdog (IWDG)
 * @param  timeout_ms : Required timeout in milliseconds.
 * @note
 *         Assumes LSI Clock = 40kHz.
 *         Prescaler is fixed to 64.
 *         Maximum reload value = 4095.
 *
 *         Counter Clock = 40000 / 64 = 625 Hz
 *         Reload = (timeout_ms × 625) / 1000
 *
 * @retval None
 */
void IWDG_Init(uint32_t timeout_ms)
{
    uint32_t reload;

    /* Enable write access to PR and RLR registers */
    IWDG->KR = 0x5555;

    /* Set Prescaler = 64
       PR = 4 corresponds to divide-by-64 */
    IWDG->PR = 4;

    /* Calculate reload value based on timeout */
    reload = (timeout_ms * 625UL) / 1000UL;

    /* Limit reload value to hardware range */
    if (reload > 4095)
        reload = 4095;

    /* Prevent invalid reload value */
    if (reload < 1)
        reload = 1;

    /* Load calculated reload value */
    IWDG->RLR = reload;

    /* Reload watchdog counter */
    IWDG->KR = 0xAAAA;

    /* Start Independent Watchdog */
    IWDG->KR = 0xCCCC;
}

/**
 * @brief  Refresh (Feed/Kick) Independent Watchdog.
 * @note
 *         Must be called before watchdog timeout.
 *         Otherwise MCU will reset automatically.
 *
 * @retval None
 */
void IWDG_Refresh(void)
{
    /* Reload watchdog counter */
    IWDG->KR = 0xAAAA;
}
