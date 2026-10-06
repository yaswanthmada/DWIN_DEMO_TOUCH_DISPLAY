/*
 * IWDT.h
 *
 *  Created on: Jul 15, 2026
 *      Author: MadaYaswanth
 */

#ifndef INC_IWDT_H_
#define INC_IWDT_H_


void IWDG_Init(uint32_t timeout_ms);
void IWDG_Refresh(void);

#endif /* INC_IWDT_H_ */
