/*
 * UART.c
 *
 *  Created on: Aug 5, 2026
 *      Author: MadaYaswanth
 */

#include"main.h"
#include"RCC.h"
#include <stdarg.h>
#include <stdio.h>

#define DEFAULT_BAUDRATE 115200UL

static inline uint32_t Calculate_BRR(uint32_t Clock, uint32_t Baud)
{
    if (Clock == 0 || Baud == 0)
    {
    	return 0;
    }
    uint32_t Mantissa = Clock / (16 * Baud);
    uint32_t Fraction = ((Clock % (16 * Baud)) + (Baud / 2)) / Baud;
    if (Fraction >= 16)
    {
    	Mantissa += 1;
        Fraction = 0;
    }
    return (Mantissa << 4) | (Fraction & 0x0F);
}
static bool UART1_Init(void)
{
    uint32_t Brr_val = Calculate_BRR(Get_APB2_freq(), DEFAULT_BAUDRATE);
    if (Brr_val == 0)
    {
        return false;
    }
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    GPIOA->CRH &= ~(GPIO_CRH_MODE9 | GPIO_CRH_CNF9 | GPIO_CRH_MODE10 | GPIO_CRH_CNF10);
    GPIOA->CRH |= (0x0B << GPIO_CRH_MODE9_Pos);
    GPIOA->CRH |= (0x04 << GPIO_CRH_MODE10_Pos);
    USART1->BRR = Brr_val;
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
    return ((USART1->CR1 & USART_CR1_UE) && (USART1->BRR == Brr_val)) ? true : false;
}
static bool UART3_Init(void)
{
    uint32_t brr_val = Calculate_BRR(Get_APB1_freq(), DEFAULT_BAUDRATE);
    if (brr_val == 0)
    {
        return false;
    }
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    GPIOB->CRH &= ~(GPIO_CRH_MODE10 | GPIO_CRH_CNF10 | GPIO_CRH_MODE11 | GPIO_CRH_CNF11);
    GPIOB->CRH |= (0x0B << GPIO_CRH_MODE10_Pos);
    GPIOB->CRH |= (0x04 << GPIO_CRH_MODE11_Pos);
    USART3->BRR = brr_val;
    USART3->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
    return ((USART3->CR1 & USART_CR1_UE) && (USART3->BRR == brr_val)) ? true : false;
}
bool USART1_Send_Char(uint8_t c)
{
    uint32_t timeout = 100000;
    while (!(USART1->SR & USART_SR_TXE))
    {
        if (--timeout == 0)
        {
        	return false;
        }
    }
    USART1->DR = (uint8_t)c;
    return true;
}

bool USART3_Send_Char(char c)
{
    uint32_t timeout = 100000;
    while (!(USART3->SR & USART_SR_TXE))
    {
        if (--timeout == 0)
        {
        	return false;
        }
    }
    USART3->DR = (uint8_t)c;
    return true;
}

void USART1_Send_String(const char *str)
{
    while(*str)
    {
    	USART1_Send_Char(*str++);
    }
}

void USART3_Send_String(const char *str)
{
        while(*str)
    	{
    	USART3_Send_Char(*str++);
    	}
}
void Send_On_Display_Uart(uint8_t *frame,uint8_t Size)
{
	for(uint8_t i=0;i<Size;i++)
	{
		USART3_Send_Char(frame[i]);
	}
}
void Uart_Printf(const char *format, ...)
{
    char buffer[100];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    USART1_Send_String(buffer);
}
bool Universal_Asyn_Rx_Tx_1_Init()
{
	return UART1_Init();
	}
bool Universal_Asyn_Rx_Tx_3_Init()
{
	return UART3_Init();
	}
