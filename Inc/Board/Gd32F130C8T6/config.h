#pragma once

// LAYOUT_2_X is used in defines_Gd32F130C8T6.h
#ifdef GD32F130 // TARGET = 1
#define LAYOUT 1
#define LAYOUT_SUB 1 // Layout 2.1.7 exisits as 2.1.7.0 and 2.1.7.1
#elif GD32F103		 // TARGET = 2
#define LAYOUT 1
#elif GD32E230 // TARGET = 3
#define LAYOUT 1
#elif MM32SPIN05 // TARGET = 4
#define LAYOUT 1
#endif

// #define MASTER 		// uncomment for MASTER firmware. Choose USART0_MASTERSLAVE or USART1_MASTERSLAVE in your defines_2-?.h file
// #define SLAVE	// uncomment for SLAVE firmware. Choose USART0_MASTERSLAVE or USART1_MASTERSLAVE in your defines_2-?.h file
#define SINGLE	// uncomment if firmware is for single board and no master-slave dual board setup

#if defined(MASTER) || defined(SINGLE)
#define MASTER_OR_SINGLE

#define REMOTE_UART
// #define REMOTE_UARTBUS	// ESP32 as master and multiple boards as multiple slaves ESP.tx-Hovers.rx and ESP.rx-Hovers.tx
// #define REMOTE_CRSF		// https://github.com/RoboDurden/Hoverboard-Firmware-Hack-Gen2.x/issues/26

#ifdef REMOTE_UARTBUS
#define SLAVE_ID 0 // must be unique for all hoverboards connected to the bus
#endif

#define CHECK_BUTTON // disable = add '//' if you use a slave board as master
#endif

// ################################################################################

//TODO: merge with FOC HACK
#define PWM_FREQ 16000 // PWM frequency in Hz
#define DEAD_TIME 60   // PWM deadtime (60 = 1�s, measured by oscilloscope)
#define DC_CUR_LIMIT 15 // Motor DC current limit in amps

// ################################################################################

#define DELAY_IN_MAIN_LOOP 5 // Delay in ms
#define TIMEOUT_MS 2000 // Time in milliseconds without steering commands before pwm emergency off

#ifdef MASTER_OR_SINGLE
#define INACTIVITY_TIMEOUT 8 // Minutes of not driving until poweroff (not very precise)

#endif

#if defined(MASTER) || defined(SLAVE)
#define MASTER_OR_SLAVE
#endif
