/*
 * Dwin_Display.h
 *
 *  Created on: Jun 19, 2026
 *      Author: MadaYaswanth
 */

#ifndef INC_DWIN_DISPLAY_H_
#define INC_DWIN_DISPLAY_H_

#include"Dwin_defines.h"
#include"main.h"
/* ── Callback type ──────────────────────────────────────
 * Fired when display sends data (read response).
 * address : VP address the data came from
 * data    : value (16-bit or 32-bit)
 * len     : byte length (2 = 16-bit, 4 = 32-bit)       */
typedef void (*DWIN_Callback)(uint16_t address, uint32_t data, uint8_t len);

/* ── Init ───────────────────────────────────────────── */
void DWIN_Init(void);

/* ── Must call every main loop iteration ───────────── */
void DWIN_Poll(void);    /* drains TX ring → USART        */
void DWIN_Listen(void);  /* parses RX bytes → callback    */

/* ── Callback ───────────────────────────────────────── */
void DWIN_AttachCallback(DWIN_Callback cb);

/* ── GROUP 1: Core VP Access ────────────────────────── */
void DWIN_WriteVP    (uint16_t address, uint16_t value);
void DWIN_WriteVP32  (uint16_t address, uint32_t value);
void DWIN_WriteVPFloat(uint16_t address, float value);
void DWIN_SetText    (uint16_t address, const char *text);
void DWIN_ReadVP     (uint16_t address, uint8_t wordLen);

/* ── GROUP 2: Hardware & System Control ─────────────── */
void DWIN_GetSoftwareVersion(void);
void DWIN_SetBaudRate       (uint32_t newBaud);
void DWIN_SetSystemConfig   (uint8_t touchSound, uint8_t dataUpload);
void DWIN_SetRTC            (uint8_t year, uint8_t month, uint8_t day,
                             uint8_t weekDay, uint8_t hour,
                             uint8_t minute, uint8_t second);
void DWIN_ReadRTC           (void);
void DWIN_PlayAudio         (uint8_t musicID, uint8_t volume);
void DWIN_Beep              (uint8_t duration_10ms);
void DWIN_SetBrightness     (uint8_t level);       /* 0–100          */
void DWIN_ReadBrightness    (void);
void DWIN_SystemReset       (void);

/* ── GROUP 3: Page & UI Control ─────────────────────── */
void DWIN_SetPage        (uint16_t pageID);
void DWIN_GetPage        (void);
void DWIN_SetPageOverlay (uint8_t enable, uint16_t overlayPageID);
void DWIN_SetTextColor   (uint16_t spAddress, uint16_t color);

/* ── GROUP 4: Multi-language ─────────────────────────── */
void DWIN_SetLanguageOffset(uint16_t offset);
void DWIN_SetLanguageICL   (uint8_t iclID);

/* ── GROUP 5: Curve ──────────────────────────────────── */
/* ch_mode upper nibble = channel enable bitmask
 * 0x10=ch0, 0x30=ch0+ch1, 0xF0=ch0–ch3               */
void DWIN_WriteCurve(uint8_t ch_mode, const int16_t *values, uint8_t count);
typedef struct
{
uint8_t Soc;
float Battery_Pack_voltage;
float Battery_Current;
float Cells[24];
uint8_t Avg_Temp;
uint8_t Bms_Fault;
uint8_t Bms_Fault_Count;
char Bms_Active_Fault[16][40];
}BMS_Data_t;
typedef struct
{
	float Mcu_Pack_voltage;
	float Mcu_Current;
	uint8_t Motor_Temp;
	uint8_t Mcu_Pcb_Temp;
	uint8_t Mcu_Fault;
	uint32_t Odo_Meter;
	uint32_t Trip_Value;
	uint8_t Mcu_Fault_Count;
	uint8_t Speed; //value given by MCU
	uint8_t Speed_1;//calculated according to motor rpm ,gear ratio, diameter of wheel
	uint8_t Vehicle_Mode;
	char Mcu_Active_Fault[22][40];
	bool Brake_Signal;
	bool Forward;
	bool Reverse;
	bool Neutral;
	bool Echo_Boost;

}MCU_Data_t;
typedef struct
{
	bool Battery_Low_12V;
    float  Voltage_12v;
}ADC_Data_t;
typedef struct
{
bool Hand_Brake;
bool Head_Light;
bool Charge_Ack;
bool Brake_Fluid;
bool Right_Ind;
bool Left_Ind;
bool Mppt_On_Off;
bool Blink_Phase_On_Off;
}IO_Data_t;
typedef struct
{
bool Is_Watch_Dog_Reset;
uint16_t Wdt_Count;
bool Is_System_Hard_Fault;
bool Is_Bms_Can_Ok;
bool Is_Mcu_Can_Ok;
bool Is_Tja1050A_Can_Ok;
bool Is_MCP2515_Can_Ok;
}System_Faults_t;
typedef struct
{
    BMS_Data_t Bms_Data;
    MCU_Data_t Mcu_Data;
    ADC_Data_t Adc_Data;
    IO_Data_t  IO_Data;
    System_Faults_t System_Data;
    bool Is_Ready;
}GEx_Display_t;
extern GEx_Display_t GEx_Display;

#endif /* INC_DWIN_DISPLAY_H_ */
