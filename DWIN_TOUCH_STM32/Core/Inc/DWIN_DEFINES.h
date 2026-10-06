/*
 * Dwin_defines.h
 *
 *  Created on: Jun 18, 2026
 *      Author: MadaYaswanth
 */

#ifndef INC_DWIN_DEFINES_H_
#define INC_DWIN_DEFINES_H_

/* ── Frame Headers ─────────────────────────────────── */
#define DWIN_HEADER_1           0x5A
#define DWIN_HEADER_2           0xA5

/* ── Commands ──────────────────────────────────────── */
#define CMD_WRITE_VP            0x82  /* Write Variable Memory  */
#define CMD_READ_VP             0x83  /* Read Variable Memory   */
#define CMD_WRITE_CURVE         0x84  /* Write Curve Buffer     */

/* ── System Variable Addresses ─────────────────────── */
#define ADDR_SYS_RESET          0x0004
#define ADDR_NOR_FLASH_RW       0x0008
#define ADDR_UART2_CFG          0x000C
#define ADDR_VERSION            0x000F
#define ADDR_RTC_NOW            0x0010
#define ADDR_PIC_NOW            0x0014
#define ADDR_SYS_CONFIG         0x0080
#define ADDR_BRIGHTNESS         0x0082
#define ADDR_PAGE_SWITCH        0x0084
#define ADDR_PWM0_SET           0x0086
#define ADDR_RTC_SET            0x009C
#define ADDR_BUZZER             0x00A0
#define ADDR_ICL_SWITCH         0x00DE
#define ADDR_PAGE_STACK         0x00E8

/* ── RGB565 Colors ─────────────────────────────────── */
#define COLOR_BLACK             0x0000
#define COLOR_WHITE             0xFFFF
#define COLOR_RED               0xF800
#define COLOR_GREEN             0x07E0
#define COLOR_BLUE              0x001F
#define COLOR_YELLOW            0xFFE0
#define COLOR_CYAN              0x07FF
#define COLOR_MAGENTA           0xF81F


#define LEFT_INDICATOR    0x1000
#define RIGHT_INDICATOR    0x1004

#define CHARGER_DETECTION 0x1001
#define AIR_BAG		0X1002
#define BRAKE_FLUID 0X1003
#define HAND_BRAKE  0X1005
#define ABS         0X1006
#define PARK_LIGHTS 0X1007

#define CAR_ROAD    0X1015
#define SPEED		0X1500
#define TEMP_VAL    0x2000

#define SOC_100     0X1017
#define SOC_80     0X1018
#define SOC_50     0X1019
#define SOC_37     0X1020
#define SOC_24     0X1021
#define SOC_14     0X1022

#define REVERSE_MODE     0X1012
#define NEUTRAL_MODE     0X1013
#define DRIVE_MODE      0X1014
#define ECHO_BOOST        0x1042

#define CELL_1  0x8000
#define CELL_2 0x8004
#define CELL_3 0x8008
#define CELL_4 0x8012
#define CELL_5 0x8016
#define CELL_6 0x8020
#define CELL_7 0x8024
#define CELL_8 0x8028
#define CELL_9 0x8032
#define CELL_10 0x8036
#define CELL_11 0x8040
#define CELL_12 0x8044
#define CELL_13 0x8048
#define CELL_14 0x8052
#define CELL_15 0x8056
#define CELL_16 0x8060
#define CELL_17 0x8064
#define CELL_18 0x8068
#define CELL_19 0x8072
#define CELL_20 0x8076
#define CELL_21 0x8080
#define CELL_22 0x8084
#define CELL_23 0x8088
#define CELL_24 0x8092

#define BATTERY_PACK_VOL 0x8096
#define BATTERY_PACK_CUR 0x8108
#define BATTERY_12VOL    0x8112
#define MCU_CUR          0x8100
#define MCU_VOL          0x8104
#define MCU_FAULT_COUNT  0x1520
#define BMS_FAULT_COUNT  0x1522
#define IS_BMS_OK        0x1524
#define IS_MCU_OK        0x1526
#define IS_CAN_OK        0x1528
#define IS_WDTR          0x1530
#define IS_HARD_FAULT    0x1532


#define MCU_TEMP   0x1538
#define MOTOR_TEMP 0x1536
#define BAT_PACK_TEMP    0x1540
#define DISPLAY_MAX_FAULTS   4


#define MCU_FAULT_1   0x1034
#define MCU_FAULT_2   0x1035
#define MCU_FAULT_3   0x1036
#define MCU_FAULT_4	  0x1037

#define BMS_FAULT_1   0x1038
#define BMS_FAULT_2   0x1039
#define BMS_FAULT_3   0x1040
#define BMS_FAULT_4   0x1041


/* ── Color helper: pack R(0-31) G(0-63) B(0-31) ───── */
#define DWIN_RGB(r,g,b) \
    ((uint16_t)(((uint16_t)((r)&0x1F)<<11)|((uint16_t)((g)&0x3F)<<5)|((uint16_t)((b)&0x1F))))

void Update_Dwin_Display();

#endif /* INC_DWIN_DEFINES_H_ */
