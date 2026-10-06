/*
 * UART.h
 *
 *  Created on: Aug 5, 2026
 *      Author: MadaYaswanth
 */

#ifndef INC_UART_H_
#define INC_UART_H_

bool Universal_Asyn_Rx_Tx_1_Init();
bool Universal_Asyn_Rx_Tx_3_Init();
void Uart_Printf(const char *format, ...);
void Send_On_Display_Uart(uint8_t *frame,uint8_t Size);
#endif /* INC_UART_H_ */
