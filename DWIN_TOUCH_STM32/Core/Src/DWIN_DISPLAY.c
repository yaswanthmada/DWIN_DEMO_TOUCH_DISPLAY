/*
 * Dwin_Display.c
 *
 *  Created on: Jun 20, 2026
 *      Author: MadaYaswanth
 */

/**
 * dwin.c — DWIN DGUS II driver
 * STM32F103CBT6 | USART1 PA9=TX PA10=RX | 115200 8N1 | APB2=72MHz
 * No HAL, no DMA, no interrupts — pure register polling
 *
 * TX: frames pushed into ring buffer, drained one byte per DWIN_Poll()
 * RX: bytes read one per DWIN_Listen() call, parsed by state machine
 */
#include "stm32f103xb.h"
#include <string.h>
#include"Dwin_Display.h"
#include <stdio.h>
GEx_Display_t GEx_Display={0} ;

/* ════════════════════════════════════════════════════
 * Config
 * ════════════════════════════════════════════════════ */
#define PCLK2_HZ  72000000UL
#define BAUD      115200UL
#define RX_BUF_SZ 255u

/* ════════════════════════════════════════════════════
 * State
 * ════════════════════════════════════════════════════ */
static int16_t       page_offset  = 0;
static DWIN_Callback event_cb     = NULL;

typedef enum { RX_IDLE, RX_HEADER1, RX_HEADER2, RX_DATA } RxState;
static RxState rx_state     = RX_IDLE;
static uint8_t rx_buf[RX_BUF_SZ];
static uint8_t rx_index     = 0;
static uint8_t rx_frame_len = 0;

/* ════════════════════════════════════════════════════
 * DWIN_Init
 * ════════════════════════════════════════════════════ */
void DWIN_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN
                  | RCC_APB2ENR_AFIOEN
                  | RCC_APB2ENR_USART1EN;

    /* PA9  TX — AF push-pull 50 MHz: CRH[7:4]  = 0xB */
    GPIOA->CRH &= ~(0xFu << 4);
    GPIOA->CRH |=  (0xBu << 4);

    /* PA10 RX — input floating:      CRH[11:8] = 0x4 */
    GPIOA->CRH &= ~(0xFu << 8);
    GPIOA->CRH |=  (0x4u << 8);

    USART1->BRR = (uint16_t)(PCLK2_HZ / BAUD);
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
    USART1->CR2 = 0;
    USART1->CR3 = 0;
}

/* ════════════════════════════════════════════════════
 * TX — your function, exactly as provided
 * Waits for TXE before each byte, returns after last byte sent
 * ════════════════════════════════════════════════════ */
static void Uart1_Send_Buffer(const uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        while (!(USART1->SR & (1U << 7))) {}   /* wait TXE */
        USART1->DR = buf[i];
    }
    /* optional: wait TC (transfer complete) — uncomment if needed */
    /* while (!(USART1->SR & (1U << 6))) {} */
}

/* ════════════════════════════════════════════════════
 * Core frame builder
 * Frame: [0x5A][0xA5][LEN][CMD][ADDR_H][ADDR_L][DATA...]
 * LEN = 1(CMD) + 2(ADDR) + len(DATA)
 * ════════════════════════════════════════════════════ */
static void send_cmd(uint8_t cmd, uint16_t address,
                     const uint8_t *data, uint8_t data_len)
{
    uint8_t frame[8 + 200];   /* header(3) + cmd(1) + addr(2) + data(max 200) */
    uint8_t idx = 0;

    frame[idx++] = DWIN_HEADER_1;
    frame[idx++] = DWIN_HEADER_2;
    frame[idx++] = (uint8_t)(1u + 2u + data_len);   /* LEN */
    frame[idx++] = cmd;
    frame[idx++] = (uint8_t)(address >> 8);
    frame[idx++] = (uint8_t)(address & 0xFFu);
    for (uint8_t i = 0; i < data_len; i++)
        frame[idx++] = data[i];
    Uart1_Send_Buffer(frame, idx);
}

/* ════════════════════════════════════════════════════
 * DWIN_Poll — no longer needed (TX is now blocking)
 * Kept as empty stub so existing main.c still compiles
 * ════════════════════════════════════════════════════ */
void DWIN_Poll(void) { /* nothing — TX is handled inside send_cmd */ }

/* ════════════════════════════════════════════════════
 * RX packet processor
 * ════════════════════════════════════════════════════ */
static void process_packet(void)
{
    if (event_cb == NULL) return;

    uint8_t  cmd     = rx_buf[0];
    uint16_t address = ((uint16_t)rx_buf[1] << 8) | rx_buf[2];

    if (cmd == CMD_READ_VP) {
        uint8_t  word_cnt = rx_buf[3];
        uint32_t result   = 0;
        uint8_t  byte_len = 0;

        if (word_cnt == 1u && rx_index >= 6u) {
            result   = ((uint32_t)rx_buf[4] << 8) | rx_buf[5];
            byte_len = 2u;
        } else if (word_cnt == 2u && rx_index >= 8u) {
            result   = ((uint32_t)rx_buf[4] << 24)
                     | ((uint32_t)rx_buf[5] << 16)
                     | ((uint32_t)rx_buf[6] <<  8)
                     |  (uint32_t)rx_buf[7];
            byte_len = 4u;
        }

        if (byte_len > 0u)
            event_cb(address, result, byte_len);
    }
}

/* ════════════════════════════════════════════════════
 * DWIN_Listen — call every main loop
 * Reads one RX byte if available, advances state machine
 * Never blocks
 * ════════════════════════════════════════════════════ */
void DWIN_Listen(void)
{
    if (!(USART1->SR & USART_SR_RXNE)) return;

    /* clear error flags silently */
    if (USART1->SR & (USART_SR_ORE | USART_SR_FE | USART_SR_NE)) {
        (void)USART1->SR;
        (void)USART1->DR;
        return;
    }

    uint8_t b = (uint8_t)(USART1->DR & 0xFFu);

    switch (rx_state) {
        case RX_IDLE:
            if (b == DWIN_HEADER_1) rx_state = RX_HEADER1;
            break;
        case RX_HEADER1:
            rx_state = (b == DWIN_HEADER_2) ? RX_HEADER2 : RX_IDLE;
            break;
        case RX_HEADER2:
            rx_frame_len = b;
            rx_index     = 0;
            rx_state     = RX_DATA;
            break;
        case RX_DATA:
            if (rx_index < RX_BUF_SZ) rx_buf[rx_index++] = b;
            if (rx_index >= rx_frame_len) {
                process_packet();
                rx_state = RX_IDLE;
            }
            break;
    }
}

/* ════════════════════════════════════════════════════
 * DWIN_AttachCallback
 * ════════════════════════════════════════════════════ */
void DWIN_AttachCallback(DWIN_Callback cb) { event_cb = cb; }

/* ════════════════════════════════════════════════════
 * GROUP 1: Core VP Access
 * ════════════════════════════════════════════════════ */
void DWIN_WriteVP(uint16_t address, uint16_t value)
{
    uint8_t d[2] = { (uint8_t)(value >> 8), (uint8_t)(value & 0xFFu) };
    send_cmd(CMD_WRITE_VP, address, d, 2u);
}

void DWIN_WriteVP32(uint16_t address, uint32_t value)
{
    uint8_t d[4] = {
        (uint8_t)((value >> 24) & 0xFFu),
        (uint8_t)((value >> 16) & 0xFFu),
        (uint8_t)((value >>  8) & 0xFFu),
        (uint8_t)( value        & 0xFFu)
    };
    send_cmd(CMD_WRITE_VP, address, d, 4u);
}

void DWIN_WriteVPFloat(uint16_t address, float value)
{
    union { float f; uint32_t u; } v;
    v.f = value;
    DWIN_WriteVP32(address, v.u);
}

void DWIN_SetText(uint16_t address, const char *text)
{
    uint8_t len = (uint8_t)strlen(text);
    if (len > 200u) len = 200u;
    send_cmd(CMD_WRITE_VP, address, (const uint8_t *)text, len);
}

void DWIN_ReadVP(uint16_t address, uint8_t wordLen)
{
    send_cmd(CMD_READ_VP, address, &wordLen, 1u);
}

/* ════════════════════════════════════════════════════
 * GROUP 2: Hardware & System Control
 * ════════════════════════════════════════════════════ */
void DWIN_GetSoftwareVersion(void) { DWIN_ReadVP(ADDR_VERSION,    1u); }
void DWIN_ReadRTC            (void) { DWIN_ReadVP(ADDR_RTC_NOW,   4u); }
void DWIN_ReadBrightness     (void) { DWIN_ReadVP(ADDR_BRIGHTNESS, 1u); }

void DWIN_SetBaudRate(uint32_t newBaud)
{
    uint16_t bv = (uint16_t)(3225600UL / newBaud);
    uint8_t d[4] = { 0x5Au, 0x00u, (uint8_t)(bv >> 8), (uint8_t)(bv & 0xFFu) };
    send_cmd(CMD_WRITE_VP, ADDR_UART2_CFG, d, 4u);
}

void DWIN_SetSystemConfig(uint8_t touchSound, uint8_t dataUpload)
{
    uint8_t cfg = 0x00u;
    if (touchSound) cfg |= 0x08u;
    if (dataUpload) cfg |= 0x20u;
    uint8_t d[4] = { 0x5Au, 0x00u, 0x00u, cfg };
    send_cmd(CMD_WRITE_VP, ADDR_SYS_CONFIG, d, 4u);
}

void DWIN_SetRTC(uint8_t year, uint8_t month, uint8_t day,
                 uint8_t weekDay, uint8_t hour, uint8_t minute, uint8_t second)
{
    uint8_t d[9] = { 0x5Au, 0xA5u, year, month, day, weekDay, hour, minute, second };
    send_cmd(CMD_WRITE_VP, ADDR_RTC_SET, d, 9u);
}

void DWIN_PlayAudio(uint8_t musicID, uint8_t volume)
{
    if (volume > 0x40u) volume = 0x40u;
    uint8_t d[4] = { musicID, 0x01u, volume, 0x00u };
    send_cmd(CMD_WRITE_VP, ADDR_BUZZER, d, 4u);
}

void DWIN_Beep(uint8_t duration_10ms)
{
    DWIN_WriteVP(ADDR_BUZZER, (uint16_t)duration_10ms);
}

void DWIN_SetBrightness(uint8_t level)
{
    if (level > 100u) level = 100u;
    uint8_t d[2] = {  0x00u, level };
    send_cmd(CMD_WRITE_VP, ADDR_BRIGHTNESS, d, 2u);
}

void DWIN_SystemReset(void)
{
    uint8_t d[4] = { 0x55u, 0xAAu, 0x5Au, 0xA5u };
    send_cmd(CMD_WRITE_VP, ADDR_SYS_RESET, d, 4u);
}

/* ════════════════════════════════════════════════════
 * GROUP 3: Page & UI Control
 * ════════════════════════════════════════════════════ */
void DWIN_SetPage(uint16_t pageID)
{
    uint16_t pid = (uint16_t)((int16_t)pageID + page_offset);
    uint8_t d[4] = { 0x5Au, 0x01u, (uint8_t)(pid >> 8), (uint8_t)(pid & 0xFFu) };
    send_cmd(CMD_WRITE_VP, ADDR_PAGE_SWITCH, d, 4u);
}

void DWIN_GetPage(void) { DWIN_ReadVP(ADDR_PIC_NOW, 1u); }

void DWIN_SetPageOverlay(uint8_t enable, uint16_t overlayPageID)
{
    uint8_t d[4] = {
        enable ? 0x5Au : 0x00u, 0x01u,
        (uint8_t)(overlayPageID >> 8),
        (uint8_t)(overlayPageID & 0xFFu)
    };
    send_cmd(CMD_WRITE_VP, ADDR_PAGE_STACK, d, 4u);
}

void DWIN_SetTextColor(uint16_t spAddress, uint16_t color)
{
    DWIN_WriteVP((uint16_t)(spAddress + 0x03u), color);
}

/* ════════════════════════════════════════════════════
 * GROUP 4: Multi-language
 * ════════════════════════════════════════════════════ */
void DWIN_SetLanguageOffset(uint16_t offset) { page_offset = (int16_t)offset; }

void DWIN_SetLanguageICL(uint8_t iclID)
{
    uint8_t d[4] = { 0x5Au, 0x00u, 0x00u, iclID };
    send_cmd(CMD_WRITE_VP, ADDR_ICL_SWITCH, d, 4u);
}

/* ════════════════════════════════════════════════════
 * GROUP 5: Curve Buffer
 * CMD 0x84 has no address field — frame built manually
 * ════════════════════════════════════════════════════ */
void DWIN_WriteCurve(uint8_t ch_mode, const int16_t *values, uint8_t count)
{
    uint8_t frame[3u + 1u + 2u * 8u];   /* hdr(3) + CMD(1) + ch_mode(1) + data */
    uint8_t idx = 0;
    uint8_t data_len = (uint8_t)(1u + (count > 8u ? 8u : count) * 2u);

    frame[idx++] = DWIN_HEADER_1;
    frame[idx++] = DWIN_HEADER_2;
    frame[idx++] = (uint8_t)(1u + data_len);   /* LEN = CMD + data */
    frame[idx++] = CMD_WRITE_CURVE;
    frame[idx++] = ch_mode;
    for (uint8_t i = 0; i < count && i < 8u; i++) {
        frame[idx++] = (uint8_t)((uint16_t)values[i] >> 8);
        frame[idx++] = (uint8_t)((uint16_t)values[i] & 0xFFu);
    }

    Uart1_Send_Buffer(frame, idx);
}


void Update_Indicators(void)
{
    static uint32_t lastTick = 0;
    static uint16_t state = 0;

    if ((HAL_GetTick() - lastTick) >= 500)
    {
        lastTick = HAL_GetTick();
        DWIN_WriteVP(RIGHT_INDICATOR,state);
        DWIN_WriteVP(LEFT_INDICATOR,state);
        state ^= 1;
    }
}
#define PARK_MODE        0X1011

void Update_Mode(void)
{
    static uint32_t lastTick = 0;
    static uint8_t activeIcon = 0;

    if ((HAL_GetTick() - lastTick) >= 900)
    {
        lastTick = HAL_GetTick();
        DWIN_WriteVP( PARK_MODE, !(activeIcon == 0) ? 1 : 0);
        DWIN_WriteVP( REVERSE_MODE, !(activeIcon == 1) ? 1 : 0);
        DWIN_WriteVP( NEUTRAL_MODE, !(activeIcon == 2) ? 1 : 0);
        DWIN_WriteVP(DRIVE_MODE, !(activeIcon == 3) ? 1 : 0);
        activeIcon++;
        if (activeIcon >=4)
            activeIcon = 0;
    }
}
#define SPEED_TENS   0x1500
#define SPEED_UNITS  0x1502

void Update_Speed(void)
{
    static uint32_t lastTick = 0;
    static uint8_t Speed = 30;

    if ((HAL_GetTick() - lastTick) >= 900)
    {
        lastTick = HAL_GetTick();

        uint8_t tensDigit  = (Speed * 205) >> 11;
        uint8_t unitsDigit = Speed - ((tensDigit << 3) + (tensDigit << 1));
        DWIN_WriteVP(SPEED_TENS, tensDigit);
        HAL_Delay(10);
        DWIN_WriteVP(SPEED_UNITS, unitsDigit);
        if (Speed > 76)
        {
            Speed = 30;
        }
        Speed++;
    }
}
void Update_Temp()
{
    static uint32_t lastTick = 0;
    static uint8_t Temp=24;
    if ((HAL_GetTick() - lastTick) >= 1000)
    {
        lastTick = HAL_GetTick();
        DWIN_WriteVP(TEMP_VAL,Temp);
        if(Temp>50)
        {
        	Temp=24;
        }
        Temp++;
    }
}
void Update_BF_AB_CD_PL_ABS_HB()
{
    static uint32_t lastTick = 0;
    static uint16_t state = 0;

        if(HAL_GetTick() - lastTick >= 1000)
        {
            lastTick = HAL_GetTick();

            switch(state)
            {
                case 0:
                    DWIN_WriteVP(CHARGER_DETECTION, 1);
                    DWIN_WriteVP(AIR_BAG,           0);
                    DWIN_WriteVP(BRAKE_FLUID,       1);
                    DWIN_WriteVP(HAND_BRAKE,        0);
                    DWIN_WriteVP(ABS,               0);
                    DWIN_WriteVP(PARK_LIGHTS,       0);
                    break;

                case 1:
                    DWIN_WriteVP(CHARGER_DETECTION, 0);
                    DWIN_WriteVP(AIR_BAG,           1);
                    DWIN_WriteVP(BRAKE_FLUID,       0);
                    DWIN_WriteVP(HAND_BRAKE,        1);
                    DWIN_WriteVP(ABS,               0);
                    DWIN_WriteVP(PARK_LIGHTS,       0);
                    break;

                case 2:
                    DWIN_WriteVP(CHARGER_DETECTION, 0);
                    DWIN_WriteVP(AIR_BAG,           0);
                    DWIN_WriteVP(BRAKE_FLUID,       0);
                    DWIN_WriteVP(HAND_BRAKE,        0);
                    DWIN_WriteVP(ABS,               1);
                    DWIN_WriteVP(PARK_LIGHTS,       1);
                    break;
            }

            state++;

            if(state >= 3)
            {
                state = 0;
            }
        }
}
void Update_Road()
{
    static uint32_t lastTick = 0;
    static uint16_t state = 0;

    if ((HAL_GetTick() - lastTick) >= 350)
    {
        lastTick = HAL_GetTick();
        DWIN_WriteVP(CAR_ROAD,state);

        state ^= 1;
    }
}
void Update_RTC()
{
	DWIN_SetRTC(26,  10,  14,12,  11, 45,  33);
}
void Update_Soc(void)
{
    static uint32_t lastTick = 0;
    static uint8_t activeIcon = 0;

    if ((HAL_GetTick() - lastTick) >= 1000)
    {
        lastTick = HAL_GetTick();

        DWIN_WriteVP( SOC_100, !(activeIcon == 0) ? 1 : 0);
        DWIN_WriteVP( SOC_80, !(activeIcon == 1) ? 1 : 0);
        DWIN_WriteVP( SOC_50, !(activeIcon == 2) ? 1 : 0);
        DWIN_WriteVP(SOC_37, !(activeIcon == 3) ? 1 : 0);
        DWIN_WriteVP( SOC_24, !(activeIcon == 4) ? 1 : 0);
        DWIN_WriteVP(SOC_14, !(activeIcon == 5) ? 1 : 0);
        activeIcon++;
        if (activeIcon >=6)
            activeIcon = 0;
    }
}
static void DWIN_Write_VP(uint16_t address, uint16_t value)
{
    uint8_t d[2] = { (uint8_t)(value >> 8), (uint8_t)(value & 0xFFu) };
    send_cmd(CMD_WRITE_VP, address, d, 2u);
}

/*******************************************************************************
 * Function Name : DWIN_Write_VP32
 * Description   : Writes a 32-bit integer value to a specified Variable Pointer (VP)
 *                 address on the DWIN display.
 * Scope         : Static (Private to this file)
 * Parameters    : address - Target Variable Pointer address
 *                 value   - 32-bit unsigned integer value to write
 * Return Value  : None
 ******************************************************************************/
static void DWIN_Write_VP32(uint16_t address, uint32_t value)
{
    uint8_t d[4] = {
        (uint8_t)((value >> 24) & 0xFFu),
        (uint8_t)((value >> 16) & 0xFFu),
        (uint8_t)((value >>  8) & 0xFFu),
        (uint8_t)( value        & 0xFFu)
    };
    send_cmd(CMD_WRITE_VP, address, d, 4u);
}
/*******************************************************************************
 * Function Name : DWIN_Write_VPFloat
 * Description   : Writes a single-precision floating-point value to a specified
 *                 Variable Pointer (VP) address by casting it via a union.
 * Scope         : Static (Private to this file)
 * Parameters    : address - Target Variable Pointer address
 *                 value   - Float value to write
 * Return Value  : None
 ******************************************************************************/
static void DWIN_Write_VPFloat(uint16_t address, float value)
{
    union { float f; uint32_t u; } v;
    v.f = value;
    DWIN_Write_VP32(address, v.u);
}


static void Update_Dwin_Display_2(void)
{


	DWIN_Write_VP(
        ECHO_BOOST,
        GEx_Display.Mcu_Data.Echo_Boost);



    DWIN_Write_VP(
        MCU_TEMP,
        GEx_Display.Mcu_Data.Mcu_Pcb_Temp);


    DWIN_Write_VP(
        MOTOR_TEMP,
        GEx_Display.Mcu_Data.Motor_Temp);



    DWIN_Write_VP(
        BAT_PACK_TEMP,
        GEx_Display.Bms_Data.Avg_Temp);

    DWIN_Write_VPFloat(
        BATTERY_PACK_VOL,
        GEx_Display.Bms_Data.Battery_Pack_voltage);


    DWIN_Write_VPFloat(
        BATTERY_PACK_CUR,
        GEx_Display.Bms_Data.Battery_Current);

    DWIN_Write_VPFloat(
    		BATTERY_12VOL,
        GEx_Display.Adc_Data.Voltage_12v);

    DWIN_Write_VPFloat(
    		MCU_CUR,
		GEx_Display.Mcu_Data.Mcu_Current);

    DWIN_Write_VPFloat(
    		MCU_VOL,
		GEx_Display.Mcu_Data.Mcu_Pack_voltage);


    DWIN_Write_VP(
        MCU_FAULT_COUNT,
        GEx_Display.Mcu_Data.Mcu_Fault_Count);


    DWIN_Write_VP(
        BMS_FAULT_COUNT,
        GEx_Display.Bms_Data.Bms_Fault_Count);



    DWIN_Write_VP(
        IS_BMS_OK,
        GEx_Display.System_Data.Is_Bms_Can_Ok);


    DWIN_Write_VP(
        IS_MCU_OK,
        GEx_Display.System_Data.Is_Mcu_Can_Ok);


    DWIN_Write_VP(
        IS_CAN_OK,
        GEx_Display.System_Data.Is_MCP2515_Can_Ok&&GEx_Display.System_Data.Is_MCP2515_Can_Ok);


    DWIN_Write_VP(
        IS_WDTR,
        GEx_Display.System_Data.Is_Watch_Dog_Reset);


    DWIN_Write_VP(
        IS_HARD_FAULT,
        GEx_Display.System_Data.Is_System_Hard_Fault);

    DWIN_Write_VPFloat(
        CELL_1,
        GEx_Display.Bms_Data.Cells[0]);


    DWIN_Write_VPFloat(
        CELL_2,
        GEx_Display.Bms_Data.Cells[1]);


    DWIN_Write_VPFloat(
        CELL_3,
        GEx_Display.Bms_Data.Cells[2]);


    DWIN_Write_VPFloat(
        CELL_4,
        GEx_Display.Bms_Data.Cells[3]);


    DWIN_Write_VPFloat(
        CELL_5,
        GEx_Display.Bms_Data.Cells[4]);


    DWIN_Write_VPFloat(
        CELL_6,
        GEx_Display.Bms_Data.Cells[5]);


    DWIN_Write_VPFloat(
        CELL_7,
        GEx_Display.Bms_Data.Cells[6]);


    DWIN_Write_VPFloat(
        CELL_8,
        GEx_Display.Bms_Data.Cells[7]);


    DWIN_Write_VPFloat(
        CELL_9,
        GEx_Display.Bms_Data.Cells[8]);


    DWIN_Write_VPFloat(
        CELL_10,
        GEx_Display.Bms_Data.Cells[9]);


    DWIN_Write_VPFloat(
        CELL_11,
        GEx_Display.Bms_Data.Cells[10]);


    DWIN_Write_VPFloat(
        CELL_12,
        GEx_Display.Bms_Data.Cells[11]);


    DWIN_Write_VPFloat(
        CELL_13,
        GEx_Display.Bms_Data.Cells[12]);


    DWIN_Write_VPFloat(
        CELL_14,
        GEx_Display.Bms_Data.Cells[13]);


    DWIN_Write_VPFloat(
        CELL_15,
        GEx_Display.Bms_Data.Cells[14]);


    DWIN_Write_VPFloat(
        CELL_16,
        GEx_Display.Bms_Data.Cells[15]);

    DWIN_Write_VPFloat(
    		CELL_17,
        GEx_Display.Bms_Data.Cells[16]);

    DWIN_Write_VPFloat(
    		CELL_18,
        GEx_Display.Bms_Data.Cells[17]);

    DWIN_Write_VPFloat(
    		CELL_19,
        GEx_Display.Bms_Data.Cells[18]);

    DWIN_Write_VPFloat(
    		CELL_20,
        GEx_Display.Bms_Data.Cells[19]);

    DWIN_Write_VPFloat(
    		CELL_21,
        GEx_Display.Bms_Data.Cells[20]);

    DWIN_Write_VPFloat(
    		CELL_22,
        GEx_Display.Bms_Data.Cells[21]);

    DWIN_Write_VPFloat(
    		CELL_23,
        GEx_Display.Bms_Data.Cells[22]);

    DWIN_Write_VPFloat(
    		CELL_24,
        GEx_Display.Bms_Data.Cells[23]);

}
void DWIN_Display_Test(void)
{

    static uint8_t motor_temp = 30;

    static uint8_t mcu_temp = 25;

    static uint8_t avg_temp = 25;

    static float pack_voltage = 70.00f;

    static float pack_current = 0.00f;

    static float cell_voltage = 3.20f;

    static float voltage_12v = 11.50f;

    static bool toggle = false;

    uint8_t i;




    pack_voltage += 0.10f;

    if (pack_voltage >= 76.00f)
    {
        pack_voltage = 70.00f;
    }
    pack_current += 0.50f;

    if (pack_current >= 50.00f)
    {
        pack_current = 20.00f;
    }

    voltage_12v += 0.5f;

    if (voltage_12v >= 14.00f)
    {
        voltage_12v = 11.50f;
    }

    motor_temp++;

    if (motor_temp >= 100)
    {
        motor_temp = 30;
    }


    mcu_temp++;

    if (mcu_temp >= 80)
    {
        mcu_temp = 25;
    }

    avg_temp++;

    if (avg_temp >= 60)
    {
        avg_temp = 25;
    }


    cell_voltage += 0.01f;

    if (cell_voltage >= 3.67f)
    {
        cell_voltage = 3.00f;
    }


//    static uint8_t fault_count = 0;
//
//    fault_count++;
//
//    if (fault_count > 4)
//    {
//        fault_count = 0;
//    }
//
//
//
//    if (fault_count >= 1)
//    {
//        strcpy(GEx_Display.Mcu_Data.Mcu_Active_Fault[0],
//               "MCU Over Temperature");
//    }
//
//    if (fault_count >= 2)
//    {
//        strcpy(GEx_Display.Mcu_Data.Mcu_Active_Fault[1],
//               "MCU Over Voltage");
//    }
//
//    if (fault_count >= 3)
//    {
//        strcpy(GEx_Display.Mcu_Data.Mcu_Active_Fault[2],
//               "MCU Over Current");
//    }
//
//    if (fault_count >= 4)
//    {
//        strcpy(GEx_Display.Mcu_Data.Mcu_Active_Fault[3],
//               "MCU CAN Error");
//    }
//
//
//    for (uint8_t i = fault_count; i < 4; i++)
//    {
//        GEx_Display.Mcu_Data.Mcu_Active_Fault[i][0] = '\0';
//    }
//
////    GEx_Display.Mcu_Data.Mcu_Fault = (fault_count != 0);
//
//
//    if (fault_count >= 1)
//    {
//        strcpy(GEx_Display.Bms_Data.Bms_Active_Fault[0],
//               "BMS Over Voltage");
//    }
//
//    if (fault_count >= 2)
//    {
//        strcpy(GEx_Display.Bms_Data.Bms_Active_Fault[1],
//               "BMS Under Voltage");
//    }
//
//    if (fault_count >= 3)
//    {
//        strcpy(GEx_Display.Bms_Data.Bms_Active_Fault[2],
//               "BMS Over Current");
//    }
//
//    if (fault_count >= 4)
//    {
//        strcpy(GEx_Display.Bms_Data.Bms_Active_Fault[3],
//               "BMS CAN Error");
//    }
//
//    for (uint8_t i = fault_count; i < 4; i++)
//    {
//        GEx_Display.Bms_Data.Bms_Active_Fault[i][0] = '\0';
//    }

//    GEx_Display.Bms_Data.Bms_Fault = (fault_count != 0);



    GEx_Display.Bms_Data.Battery_Pack_voltage =
        pack_voltage;


    GEx_Display.Bms_Data.Battery_Current =
        pack_current;


    GEx_Display.Bms_Data.Avg_Temp =
        avg_temp;


    GEx_Display.Bms_Data.Bms_Fault =
        toggle;



    for (i = 0; i < 24; i++)
    {
    	if(i%2==0)
    	{
            GEx_Display.Bms_Data.Cells[i] =
                cell_voltage;
    	}
    	else
    	{
            GEx_Display.Bms_Data.Cells[i] =
                cell_voltage + 0.01f;
    	}

    }


    GEx_Display.Mcu_Data.Mcu_Pack_voltage =
        pack_voltage;

    GEx_Display.Mcu_Data.Mcu_Current =
        pack_current;


    GEx_Display.Mcu_Data.Motor_Temp =
        motor_temp;


    GEx_Display.Mcu_Data.Mcu_Pcb_Temp =
        mcu_temp;


    GEx_Display.Mcu_Data.Mcu_Fault =
        toggle;


    GEx_Display.Mcu_Data.Echo_Boost =
       !toggle;


    GEx_Display.Adc_Data.Voltage_12v =
        voltage_12v;


    GEx_Display.System_Data.Is_Watch_Dog_Reset =
    		toggle;


    GEx_Display.System_Data.Is_System_Hard_Fault =
    		!toggle;


    GEx_Display.System_Data.Is_Bms_Can_Ok =
        !toggle;


    GEx_Display.System_Data.Is_Mcu_Can_Ok =
    		toggle;


    GEx_Display.System_Data.Is_MCP2515_Can_Ok =
        !toggle;
    toggle=!toggle;

}
void Update_Fault_Dashboard() {

    static uint32_t lastTick = 0;
    static uint16_t state = 0;

    if ((HAL_GetTick() - lastTick) >= 1000)
    {
        lastTick = HAL_GetTick();

        DWIN_WriteVP(MCU_FAULT_1, (state < 1) ? 1 : 0);
        DWIN_WriteVP(MCU_FAULT_2, (state < 2) ? 1 : 0);
        DWIN_WriteVP(MCU_FAULT_3, (state < 3) ? 1 : 0);
        DWIN_WriteVP(MCU_FAULT_4, (state < 4) ? 1 : 0);
        DWIN_WriteVP(BMS_FAULT_1, (state < 1) ? 1 : 0);
        DWIN_WriteVP(BMS_FAULT_2, (state < 2) ? 1 : 0);
        DWIN_WriteVP(BMS_FAULT_3, (state < 3) ? 1 : 0);
        DWIN_WriteVP(BMS_FAULT_4, (state < 4) ? 1 : 0);

        /* Fault count */
        GEx_Display.Bms_Data.Bms_Fault_Count = state;
        GEx_Display.Mcu_Data.Mcu_Fault_Count = state;

        state++;

        if (state > 4)
        {
            state = 0;
        }
    }

}
void Update_Dwin_Display()
{
    static uint32_t startTick = 0,startTick1=0;
    static uint8_t enable = 0;
    if (!enable)
    {
        if (startTick == 0)
        {
            startTick = HAL_GetTick();    // Start timer once
        }

        if ((HAL_GetTick() - startTick) >= 3000U)
        {
        	  DWIN_SetPage(01);
            enable = 1;                   // Enable continuous execution
        }

        return;
    }
	Update_Indicators();
	Update_BF_AB_CD_PL_ABS_HB();
	Update_Mode();
	Update_Speed();
	Update_Temp();
	Update_Road();
	Update_Soc();
	Update_Fault_Dashboard();
	Update_Dwin_Display_2();
    if ((HAL_GetTick() - startTick1) >= 1000U)
    {
    	DWIN_Display_Test();
    	startTick1=HAL_GetTick();
    }

}
