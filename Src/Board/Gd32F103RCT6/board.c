#include "it.h"
#include "defines.h"
#include "board.h"
#include "comms.h"
#include "app.h"
#include "buzzer.h"
#include "config.h"
#include "setup.h"
#include <stdio.h>
#include <string.h>
#include <platform.h>
#include "oscilloscope.h"

void delay(uint16_t ms) {
    Delay(ms);
}

//----------------------------------------------------------------------------
// Send buffer via USART
//----------------------------------------------------------------------------
void SendBuffer(uint32_t usart_periph, uint8_t buffer[], uint8_t length)
{
	uint8_t index = 0;
	
	for(; index < length; index++)
	{
        usart_data_transmit(usart_periph, buffer[index]);

        while (usart_flag_get(usart_periph, USART_FLAG_TBE) == RESET)
		{
        }
	}
}

#define TIMEOUT_FREQ  1000
#define ARRAY_LEN(x) (uint32_t)(sizeof(x) / sizeof(*(x)))

uint8_t usart0_rx_buf[1];
uint8_t usart1_rx_buf[SERIAL_BUFFER_SIZE]; // USART Rx DMA circular buffer
uint32_t usart1_rx_buf_len = ARRAY_LEN(usart1_rx_buf);

Buzzer buzzer;
volatile adc_buf_t adc_buffer;

#define OSCILLOSCOPE_CH_TIM1_
#define OSCILLOSCOPE_DATA_SIZE 256

uint16_t oscilloscope_data[OSCILLOSCOPE_DATA_SIZE];
volatile Oscilloscope oscilloscope;


// DMA (ADC) structs
dma_parameter_struct dma_init_struct_adc;
//----------------------------------------------------------------------------
// Initializes the interrupts
//----------------------------------------------------------------------------
void Interrupt_init(void)
{
  // Set IRQ priority configuration
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
}

//----------------------------------------------------------------------------
// Initializes the watchdog
//----------------------------------------------------------------------------
ErrStatus Watchdog_init(void)
{
	return SUCCESS;

	// Check if the system has resumed from FWDGT reset
	if (RESET != rcu_flag_get(RCU_FLAG_FWDGTRST))
	{   
		// FWDGTRST flag set
		rcu_all_reset_flag_clear();
	}
	
	// Clock source is IRC40K (40 kHz)
	// Prescaler is 16
	// Reload value is 4096 (0x0FFF)
	// Watchdog fires after 1638.4 ms
	if (fwdgt_config(0x0FFF, FWDGT_PSC_DIV16) != SUCCESS ||
		fwdgt_window_value_config(0x0FFF) != SUCCESS)
	{
		return ERROR;
	}

	// Enable free watchdog timer
	fwdgt_enable();
	
	return SUCCESS;
}

/*
* USART0 = USART1 in datasheet
* USART1 = USART2 in datasheet
* etc.
*/
void USART_A_Init(uint32_t iBaud)
{
	rcu_periph_clock_enable(RCU_USART1);
	rcu_periph_clock_enable(RCU_GPIOA);

	gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
	gpio_init(GPIOA, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, GPIO_PIN_3);
	
	usart_deinit(USART1);
	usart_baudrate_set(USART1, iBaud);
	usart_parity_config(USART1, USART_PM_NONE);
	usart_word_length_set(USART1, USART_WL_8BIT);
	usart_stop_bit_set(USART1, USART_STB_1BIT);
	
	usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
	usart_receive_config(USART1, USART_RECEIVE_ENABLE);
	
	usart_enable(USART1);
}

/*
* USART0 = USART1 in datasheet
* USART1 = USART2 in datasheet
* etc.
*/
void USART_B_Init(uint32_t iBaud)
{
	rcu_periph_clock_enable(RCU_USART2);
	rcu_periph_clock_enable(RCU_GPIOB);

	gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
	gpio_init(GPIOB, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, GPIO_PIN_11);
	
	usart_deinit(USART2);
	usart_baudrate_set(USART2, iBaud);
	usart_parity_config(USART2, USART_PM_NONE);
	usart_word_length_set(USART2, USART_WL_8BIT);
	usart_stop_bit_set(USART2, USART_STB_1BIT);
	
	usart_transmit_config(USART2, USART_TRANSMIT_ENABLE);
	usart_receive_config(USART2, USART_RECEIVE_ENABLE);
	
	usart_enable(USART2);
}

void reset(void)
{
	gpio_bit_write(GPIOA, GPIO_PIN_5, RESET);

	while(1)
	{
		// Reload watchdog until device is off
		fwdgt_counter_reload();
	}
}

#include "gd32f10x.h"
#include "gd32f10x_rcu.h"

#define ADC_CLOCK_PRESCALER_CONFIG RCU_CKADC_CKAPB2_DIV6
#define CONVERT_ADC_CYCLE_COUNT_TO_SYSTEM_CYCLE_COUNT(x) x * 6

void SystemClock_96MHz_IRC8M(void) {
	rcu_deinit();

    rcu_osci_on(RCU_IRC8M);
    while (rcu_osci_stab_wait(RCU_IRC8M) == ERROR) {}

	/**
	 * AHB = 96 Mhz
	 * APB1 = 48 Mhz (max 54 Mhz)
	 * APB2 = 96 Mhz 
	 */
	rcu_ahb_clock_config(RCU_AHB_CKSYS_DIV1);
    rcu_apb1_clock_config(RCU_APB1_CKAHB_DIV2);
    rcu_apb2_clock_config(RCU_APB2_CKAHB_DIV1);

	/**
	 * 8Mhz / 2 * 24 = 96Mhz
	 */
    rcu_pll_config(RCU_PLLSRC_IRC8M_DIV2, RCU_PLL_MUL24);

    rcu_osci_on(RCU_PLL_CK);
    while (rcu_osci_stab_wait(RCU_PLL_CK) == ERROR) {}

    rcu_system_clock_source_config(RCU_CKSYSSRC_PLL);
    while (rcu_system_clock_source_get() != RCU_SCSS_PLL) {}
}

void SystemClock_Config(void)
{
	//SystemClock_96MHz_IRC8M();
	SystemCoreClockUpdate();

	SysTick_Config(SystemCoreClock / 1000);
	nvic_irq_enable(SysTick_IRQn, 0, 0);
}

void MX_GPIO_Init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);

    // gpio_init(LEFT_HALL_U_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, LEFT_HALL_U_PIN);
	// gpio_init(LEFT_HALL_V_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, LEFT_HALL_V_PIN);
	// gpio_init(LEFT_HALL_W_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, LEFT_HALL_W_PIN);
	// gpio_init(RIGHT_HALL_U_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, RIGHT_HALL_U_PIN);
	// gpio_init(RIGHT_HALL_V_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, RIGHT_HALL_V_PIN);
	// gpio_init(RIGHT_HALL_W_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, RIGHT_HALL_W_PIN);

    // gpio_init(CHARGER_PORT, GPIO_MODE_IPU, GPIO_OSPEED_2MHZ, CHARGER_PIN);

#if defined(SUPPORT_BUTTONS_LEFT) || defined(SUPPORT_BUTTONS_RIGHT)
    gpio_init(BUTTON1_PORT, GPIO_MODE_IPU, GPIO_OSPEED_2MHZ, BUTTON1_PIN);
    gpio_init(BUTTON2_PORT, GPIO_MODE_IPU, GPIO_OSPEED_2MHZ, BUTTON2_PIN);
#endif

	gpio_init(BUTTON_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, BUTTON_PIN);

    gpio_init(LED_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, LED_PIN);
	gpio_init(BUZZER_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, BUZZER_PIN);
	gpio_init(OFF_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, OFF_PIN);

    gpio_init(LEFT_DC_CUR_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, LEFT_DC_CUR_PIN);
    gpio_init(LEFT_U_CUR_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, LEFT_U_CUR_PIN);
    gpio_init(LEFT_V_CUR_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, LEFT_V_CUR_PIN);
    gpio_init(RIGHT_DC_CUR_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, RIGHT_DC_CUR_PIN);
    gpio_init(RIGHT_U_CUR_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, RIGHT_U_CUR_PIN);
    gpio_init(RIGHT_V_CUR_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, RIGHT_V_CUR_PIN);
    gpio_init(DCLINK_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, DCLINK_PIN);

#if !defined(SUPPORT_BUTTONS_LEFT)
    gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, GPIO_PIN_3);
    gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, GPIO_PIN_2);
#endif

    // gpio_init(LEFT_TIM_UH_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_2MHZ, LEFT_TIM_UH_PIN);
    // gpio_init(LEFT_TIM_VH_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_2MHZ, LEFT_TIM_VH_PIN);
    // gpio_init(LEFT_TIM_WH_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_2MHZ, LEFT_TIM_WH_PIN);

    // gpio_init(LEFT_TIM_UL_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_2MHZ, LEFT_TIM_UL_PIN);
    // gpio_init(LEFT_TIM_VL_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_2MHZ, LEFT_TIM_VL_PIN);
    // gpio_init(LEFT_TIM_WL_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_2MHZ, LEFT_TIM_WL_PIN);

    // gpio_init(RIGHT_TIM_UH_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_2MHZ, RIGHT_TIM_UH_PIN);
    // gpio_init(RIGHT_TIM_VH_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_2MHZ, RIGHT_TIM_VH_PIN);
    // gpio_init(RIGHT_TIM_WH_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_2MHZ, RIGHT_TIM_WH_PIN);

    // gpio_init(RIGHT_TIM_UL_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_2MHZ, RIGHT_TIM_UL_PIN);
    // gpio_init(RIGHT_TIM_VL_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_2MHZ, RIGHT_TIM_VL_PIN);
    // gpio_init(RIGHT_TIM_WL_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_2MHZ, RIGHT_TIM_WL_PIN);
}

void MX_TIM_Init(void)
{
	timer_parameter_struct timer_common_parameters;
    timer_oc_parameter_struct timer_common_channel_parameters;
    timer_break_parameter_struct timer_common_break_parameters;

    timer_struct_para_init(&timer_common_parameters);
    timer_common_parameters.prescaler = 0;
    timer_common_parameters.alignedmode = TIMER_COUNTER_CENTER_BOTH;
    timer_common_parameters.counterdirection = TIMER_COUNTER_UP;
    timer_common_parameters.period = SystemCoreClock / 2 / PWM_FREQ;
    timer_common_parameters.clockdivision = TIMER_CKDIV_DIV1;

	timer_channel_output_struct_para_init(&timer_common_channel_parameters);
    timer_common_channel_parameters.outputstate = TIMER_CCX_ENABLE;
    timer_common_channel_parameters.outputnstate = TIMER_CCXN_ENABLE;
    timer_common_channel_parameters.ocpolarity = TIMER_OC_POLARITY_HIGH;
    timer_common_channel_parameters.ocnpolarity = TIMER_OCN_POLARITY_LOW;
    timer_common_channel_parameters.ocidlestate = TIMER_OC_IDLE_STATE_LOW;
    timer_common_channel_parameters.ocnidlestate = TIMER_OCN_IDLE_STATE_HIGH;

	timer_break_struct_para_init(&timer_common_break_parameters);
    timer_common_break_parameters.runoffstate = TIMER_ROS_STATE_ENABLE;
    timer_common_break_parameters.ideloffstate = TIMER_IOS_STATE_ENABLE;
    timer_common_break_parameters.deadtime = DEAD_TIME;
    timer_common_break_parameters.breakpolarity = TIMER_BREAK_POLARITY_LOW;
    timer_common_break_parameters.outputautostate = TIMER_OUTAUTO_DISABLE;
    timer_common_break_parameters.protectmode = TIMER_CCHP_PROT_OFF;
    timer_common_break_parameters.breakstate = TIMER_BREAK_DISABLE;

	//TIMER MOTOR 1
	rcu_periph_clock_enable(RCU_TIMER0);

    timer_init(TIMER0, &timer_common_parameters);

    timer_channel_output_config(TIMER0, TIMER_CH_0, &timer_common_channel_parameters);
    timer_channel_output_config(TIMER0, TIMER_CH_1, &timer_common_channel_parameters);
    timer_channel_output_config(TIMER0, TIMER_CH_2, &timer_common_channel_parameters);

    timer_break_config(TIMER0, &timer_common_break_parameters);

	timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0, 0);
    timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_1, 0);
    timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_2, 0);
    timer_channel_output_mode_config(TIMER0, TIMER_CH_0, TIMER_OC_MODE_PWM0);
    timer_channel_output_mode_config(TIMER0, TIMER_CH_1, TIMER_OC_MODE_PWM0);
    timer_channel_output_mode_config(TIMER0, TIMER_CH_2, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER0, TIMER_CH_0, TIMER_OC_SHADOW_DISABLE);
    timer_channel_output_shadow_config(TIMER0, TIMER_CH_1, TIMER_OC_SHADOW_DISABLE);
    timer_channel_output_shadow_config(TIMER0, TIMER_CH_2, TIMER_OC_SHADOW_DISABLE);

	nvic_irq_enable(TIMER0_UP_IRQn, 0, 0);
	nvic_irq_enable(TIMER0_Channel_IRQn, 0, 0);
	timer_interrupt_enable(TIMER0, TIMER_INT_UP);
	timer_interrupt_enable(TIMER0, TIMER_INT_CH0);
	timer_interrupt_enable(TIMER0, TIMER_INT_CH1);
	timer_interrupt_enable(TIMER0, TIMER_INT_CH2);

	//TIMER MOTOR 2
	rcu_periph_clock_enable(RCU_TIMER7);
	timer_init(TIMER7, &timer_common_parameters);

	timer_channel_output_config(TIMER7, TIMER_CH_0, &timer_common_channel_parameters);
	timer_channel_output_config(TIMER7, TIMER_CH_1, &timer_common_channel_parameters);
	timer_channel_output_config(TIMER7, TIMER_CH_2, &timer_common_channel_parameters);

	timer_break_config(TIMER7, &timer_common_break_parameters);

	timer_channel_output_pulse_value_config(TIMER7, TIMER_CH_0, 0);
    timer_channel_output_pulse_value_config(TIMER7, TIMER_CH_1, 0);
    timer_channel_output_pulse_value_config(TIMER7, TIMER_CH_2, 0);
    timer_channel_output_mode_config(TIMER7, TIMER_CH_0, TIMER_OC_MODE_PWM0);
    timer_channel_output_mode_config(TIMER7, TIMER_CH_1, TIMER_OC_MODE_PWM0);
    timer_channel_output_mode_config(TIMER7, TIMER_CH_2, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER7, TIMER_CH_0, TIMER_OC_SHADOW_DISABLE);
    timer_channel_output_shadow_config(TIMER7, TIMER_CH_1, TIMER_OC_SHADOW_DISABLE);
    timer_channel_output_shadow_config(TIMER7, TIMER_CH_2, TIMER_OC_SHADOW_DISABLE);

	timer_interrupt_enable(TIMER7, TIMER_INT_TRG);
	nvic_irq_enable(TIMER7_TRG_CMT_IRQn, 0, 0);

	//TIMER ADC PHASE CURRENTS TRIGGER
	rcu_periph_clock_enable(RCU_TIMER1);
	timer_init(TIMER1, &timer_common_parameters);

	timer_interrupt_enable(TIMER1, TIMER_INT_TRG);
	nvic_irq_enable(TIMER1_IRQn, 0, 0);

	//TIMER ADC SECONDARY VALUES TRIGGER
	timer_parameter_struct timer_generic_adc_parameters;
	timer_struct_para_init(&timer_generic_adc_parameters);
    timer_generic_adc_parameters.prescaler = 0;
    timer_generic_adc_parameters.alignedmode = TIMER_COUNTER_EDGE;
    timer_generic_adc_parameters.counterdirection = TIMER_COUNTER_UP;
	//3 times as lower as pwm because it is not centrally aligned
    timer_generic_adc_parameters.period = SystemCoreClock / PWM_FREQ / 3;
    timer_generic_adc_parameters.clockdivision = TIMER_CKDIV_DIV1;

	rcu_periph_clock_enable(RCU_TIMER2);
	timer_init(TIMER2, &timer_generic_adc_parameters);

	timer_master_slave_mode_config(TIMER2, TIMER_MASTER_SLAVE_MODE_DISABLE);
    timer_master_output_trigger_source_select(TIMER2, TIMER_TRI_OUT_SRC_UPDATE);

	timer_interrupt_enable(TIMER2, TIMER_INT_TRG);
	nvic_irq_enable(TIMER2_IRQn, 0, 0);

	//TIMER interoperability

	/**                                              
	 *	Timer0─┬─►Timer7 with offset─┬─►(ADC3 Regular)DC Link
	 *	   	   │         			 │                             
	 *	   	   │         			 └─►Motor 2 PWM Channels
	 *	   	   │                    
	 *	   	   │─►Motor 1 PWM Channels
	 *	   	   │                   	                                        
	 *	   	   └─►Timer1 with offset(phase current adc)──►(ADC1&2 Dual Injected Mode)Phase Currents
	 *												
	 *	Timer2(misc adc trigger)──►(ADC1 & ADC2 Regular)
	 *	
	 * 	->TIMER0 ITI0: TIMER4_TRGO ITI1: TIMER1_TRGO ITI2: TIMER2_TRGO ITI3: TIMER3_TRGO
	 *	->TIMER7 ITI0: ->TIMER0_TRGO ITI1: TIMER1_TRGO ITI2: TIMER3_TRGO ITI3: TIMER4_TRGO
	 * 	->TIMER1 ITI0: ->TIMER0_TRGO ITI1: refer to note (5) ITI2: TIMER2_TRGO ITI3: TIMER3_TRGO
	 * 	TIMER2 ITI0: TIMER0_TRGO ITI1: TIMER1_TRGO ITI2: TIMER4_TRGO ITI3: TIMER3_TRGO
	 * 
	 * 	TIMER3 ITI0: TIMER0_TRGO ITI1: TIMER1_TRGO ITI2: TIMER2_TRGO ITI3: TIMER7_TRGO
	 * 	TIMER4 ITI0: TIMER1_TRGO ITI1: TIMER2_TRGO ITI2: TIMER3_TRGO ITI3: TIMER7_TRGO
	 * 	TIMER8 ITI0: TIMER1_TRGO ITI1: TIMER2_TRGO ITI2: TIMER9_TRGO ITI3: TIMER10_ TRGO
	 * 	TIMER11 ITI0: TIMER3_TRGO ITI1: TIMER4_TRGO ITI2: TIMER12_TRGO ITI3: TIMER13_ TRGO
	 */
	timer_master_slave_mode_config(TIMER0, TIMER_MASTER_SLAVE_MODE_DISABLE);
    timer_master_output_trigger_source_select(TIMER0, TIMER_TRI_OUT_SRC_ENABLE);
	
	timer_master_output_trigger_source_select(TIMER7, TIMER_TRI_OUT_SRC_UPDATE);
	timer_slave_mode_select(TIMER7, TIMER_SLAVE_MODE_PAUSE);
    timer_master_slave_mode_config(TIMER7, TIMER_MASTER_SLAVE_MODE_ENABLE);
    timer_input_trigger_source_select(TIMER7, TIMER_SMCFG_TRGSEL_ITI0);

	timer_master_output_trigger_source_select(TIMER1, TIMER_TRI_OUT_SRC_UPDATE);
	timer_slave_mode_select(TIMER1, TIMER_SLAVE_MODE_PAUSE);
	timer_master_slave_mode_config(TIMER1, TIMER_MASTER_SLAVE_MODE_ENABLE);
    timer_input_trigger_source_select(TIMER1, TIMER_SMCFG_TRGSEL_ITI0);

	/**
	 * Start counting >0 to effectively offset timers by the time it takes for one ADC conversion to complete.
	 * This method allows that the Phase currents ADC measurements are properly aligned with LOW-FET ON region for both motors
	 */
    timer_counter_value_config(TIMER7, CONVERT_ADC_CYCLE_COUNT_TO_SYSTEM_CYCLE_COUNT(ADC_PHASE_CURRENT_CONVERSION_CYCLE_COUNT));
	timer_counter_value_config(TIMER1, CONVERT_ADC_CYCLE_COUNT_TO_SYSTEM_CYCLE_COUNT(ADC_PHASE_CURRENT_CONVERSION_CYCLE_COUNT));

	/**
	 * Generate update event on overflow only. Should be set BEFORE timer enabled (otherwise underflow events take place)
	 */
	timer_repetition_value_config(TIMER1, 1);
	timer_repetition_value_config(TIMER7, 1);

	/**
	 * DISABLE MOTORS
	 */
	timer_primary_output_config(TIMER0, DISABLE);
	timer_primary_output_config(TIMER7, DISABLE);

	rcu_periph_clock_enable(RCU_TIMER3);

	timer_parameter_struct timer_debug_parameters;

    timer_struct_para_init(&timer_debug_parameters);
    timer_debug_parameters.prescaler = 0;
    timer_debug_parameters.alignedmode = TIMER_COUNTER_EDGE;
    timer_debug_parameters.counterdirection = TIMER_COUNTER_UP;
    timer_debug_parameters.period = UINT16_MAX;
    timer_debug_parameters.clockdivision = TIMER_CKDIV_DIV1;

	timer_init(TIMER3, &timer_debug_parameters);
	timer_slave_mode_select(TIMER3, TIMER_SLAVE_MODE_PAUSE);
	timer_master_slave_mode_config(TIMER3, TIMER_MASTER_SLAVE_MODE_ENABLE);
    timer_input_trigger_source_select(TIMER3, TIMER_SMCFG_TRGSEL_ITI0);
}

uint16_t timer0_channels_osc[3] = {
	OSCILLOSCOPE_TIMER0_CH0_UPDATE,
	OSCILLOSCOPE_TIMER0_CH1_UPDATE,
	OSCILLOSCOPE_TIMER0_CH2_UPDATE
};

uint16_t timer0_channel_interrupts[3] = {
	TIMER_INT_FLAG_CH0,
	TIMER_INT_FLAG_CH1,
	TIMER_INT_FLAG_CH2
};

void TIMER0_UP_IRQHandler()
{
	uint16_t debugTime = timer_counter_read(TIMER3);
	oscilloscope_update(&oscilloscope, OSCILLOSCOPE_TIMER0_UPDATE, debugTime, 1);
}

void TIMER0_Channel_IRQHandler()
{
	uint16_t debugTime = timer_counter_read(TIMER3);
	uint32_t isCountingDown = TIMER_CTL0(TIMER0) & (uint32_t)TIMER_CTL0_DIR;

	for (uint16_t i = 0; i < 3; i++)
	{
		if (timer_interrupt_flag_get(TIMER0, timer0_channel_interrupts[i]))
		{
			if (isCountingDown)
			{
				oscilloscope_update(&oscilloscope, timer0_channels_osc[i], debugTime, 1);
				oscilloscope_update(&oscilloscope, timer0_channels_osc[i], debugTime, 0);
			}
			else
			{
				oscilloscope_update(&oscilloscope, timer0_channels_osc[i], debugTime, 0);
				oscilloscope_update(&oscilloscope, timer0_channels_osc[i], debugTime, 1);
			}

			timer_interrupt_flag_clear(TIMER0, timer0_channel_interrupts[i]);
		}
	}
}

void TIMER1_IRQHandler()
{
	if (timer_interrupt_flag_get(TIMER1, TIMER_INT_FLAG_TRG))
	{
		uint16_t debugTime = timer_counter_read(TIMER3);
		oscilloscope_update(&oscilloscope, OSCILLOSCOPE_PHASE_CURRENTs_ADC_START, debugTime, 1);

		timer_interrupt_flag_clear(TIMER1, TIMER_INT_FLAG_TRG);
	}
}

void TIMER2_IRQHandler()
{
	if (timer_interrupt_flag_get(TIMER2, TIMER_INT_FLAG_TRG))
	{
		uint16_t debugTime = timer_counter_read(TIMER3);
		oscilloscope_update(&oscilloscope, OSCILLOSCOPE_GENERAL_ADC_START, debugTime, 1);

		timer_interrupt_flag_clear(TIMER2, TIMER_INT_FLAG_TRG);
	}
}

void TIMER7_TRG_CMT_IRQHandler()
{
	if (timer_interrupt_flag_get(TIMER7, TIMER_INT_FLAG_TRG))
	{
		uint16_t debugTime = timer_counter_read(TIMER3);
		oscilloscope_update(&oscilloscope, OSCILLOSCOPE_DC_LINK_ADC_START, debugTime, 1);

		timer_interrupt_flag_clear(TIMER7, TIMER_INT_FLAG_TRG);
	}
}

void MX_Tim_Start(void)
{
	/**
	 * Kick off timers 
	 * 1. master timer with motor & adc slaves
	 * 2. misc adc
	 */
    timer_enable(TIMER0);
	timer_enable(TIMER2);
}

typedef struct {
	uint16_t batt1;
	uint16_t temp;
} adc1_regular_values_t;

adc1_regular_values_t adc1_regular_values;

typedef struct {
	uint16_t l_tx2;
	uint16_t l_rx2;
} adc2_regular_values_t;

adc2_regular_values_t adc2_regular_values;

typedef struct {
	uint16_t motor_1_dc_link_current;
	uint16_t motor_2_dc_link_current;
} adc3_regular_values_t;

adc3_regular_values_t adc3_dc_link_currents;

void MX_ADC1_Init(void)
{
    rcu_periph_clock_enable(RCU_ADC0);

    adc_deinit(ADC0);
	adc_mode_config(ADC_DAUL_INSERTED_PARALLEL);
    
	adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, DISABLE);
    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);

	adc_external_trigger_source_config(ADC0, ADC_INSERTED_CHANNEL, ADC0_1_EXTTRIG_INSERTED_T1_TRGO);
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);

	adc_interrupt_enable(ADC0, ADC_INT_EOIC);
	nvic_irq_enable(ADC0_1_IRQn, 0, 0);

	/**
	 * Inserted (critical) channels 
	 * motor_1_phase_current_A, motor_2_phase_current_B
	 */
	adc_channel_length_config(ADC0, ADC_INSERTED_CHANNEL, 2);

    adc_inserted_channel_config(ADC0, 0, ADC_CHANNEL_0, ADC_SAMPLETIME_7POINT5);
    adc_inserted_channel_config(ADC0, 1, ADC_CHANNEL_14, ADC_SAMPLETIME_7POINT5);

	/**
	 * Regular misc values (triggered by timer2)
	 * batt1 temp
	 */
    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_EXTTRIG_REGULAR_T2_TRGO);
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);

	adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, 2);
	
#if BOARD_VARIANT == 0
    adc_regular_channel_config(ADC0, 0, ADC_CHANNEL_12, ADC_SAMPLETIME_7POINT5);
#elif BOARD_VARIANT == 1
    adc_regular_channel_config(ADC0, 0, ADC_CHANNEL_1, ADC_SAMPLETIME_7POINT5);
#endif
	/**
	 * At 16Mhz 239.5 points is 239.5 * (1/16000000) * 1000000 = 14.96875us
	 * datasheet requires 17.1us
	 */
    adc_regular_channel_config(ADC0, 1, ADC_CHANNEL_16, ADC_SAMPLETIME_239POINT5);
    adc_tempsensor_vrefint_enable();

    adc_enable(ADC0);
	delay(1);
    adc_calibration_enable(ADC0);

    rcu_periph_clock_enable(RCU_DMA0);

	dma_parameter_struct dma_init_struct;

    dma_deinit(DMA0, DMA_CH0);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_addr = (uint32_t)&adc1_regular_values;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_16BIT;
    dma_init_struct.number = 2;
    dma_init_struct.periph_addr = (uint32_t)&ADC_RDATA(ADC0);
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(DMA0, DMA_CH0, &dma_init_struct);
    
    dma_circulation_enable(DMA0, DMA_CH0);
    
	dma_interrupt_enable(DMA0, DMA_CH0, DMA_INT_FTF);
    nvic_irq_enable(DMA0_Channel0_IRQn, 0, 0);

    dma_channel_enable(DMA0, DMA_CH0);

	adc_dma_mode_enable(ADC0);
}

void MX_ADC2_Init(void)
{
    rcu_periph_clock_enable(RCU_ADC1);

    adc_deinit(ADC1);

	adc_data_alignment_config(ADC1, ADC_DATAALIGN_RIGHT);
    adc_special_function_config(ADC1, ADC_CONTINUOUS_MODE, DISABLE);
    adc_special_function_config(ADC1, ADC_SCAN_MODE, ENABLE);

	/**
	 * Inserted (critical) channels slave mode 
	 * motor_1_phase_current_B, motor_2_phase_current_C
	 */
	adc_external_trigger_source_config(ADC1, ADC_INSERTED_CHANNEL, ADC0_1_2_EXTTRIG_INSERTED_NONE);
	adc_external_trigger_config(ADC1, ADC_INSERTED_CHANNEL, ENABLE);

	adc_channel_length_config(ADC1, ADC_INSERTED_CHANNEL, 2);

    adc_inserted_channel_config(ADC1, 0, ADC_CHANNEL_13, ADC_SAMPLETIME_7POINT5);
    adc_inserted_channel_config(ADC1, 1, ADC_CHANNEL_15, ADC_SAMPLETIME_7POINT5);

	/**
	 * Misc values triggered by Timer2
	 */
	adc_external_trigger_source_config(ADC1, ADC_REGULAR_CHANNEL, ADC0_1_EXTTRIG_REGULAR_T2_TRGO);
	adc_external_trigger_config(ADC1, ADC_REGULAR_CHANNEL, ENABLE);

    adc_channel_length_config(ADC1, ADC_REGULAR_CHANNEL, 2);

    adc_regular_channel_config(ADC1, 0, ADC_CHANNEL_2, ADC_SAMPLETIME_7POINT5);
    adc_regular_channel_config(ADC1, 1, ADC_CHANNEL_3, ADC_SAMPLETIME_7POINT5);

    adc_enable(ADC1);
	delay(1);
    adc_calibration_enable(ADC1);

	rcu_periph_clock_enable(RCU_DMA0);

	dma_parameter_struct dma_init_struct;

    dma_deinit(DMA0, DMA_CH1);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_addr = (uint32_t)&adc2_regular_values;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_16BIT;
    dma_init_struct.number = 2;
    dma_init_struct.periph_addr = (uint32_t)&ADC_RDATA(ADC1);
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(DMA0, DMA_CH1, &dma_init_struct);
    
    dma_circulation_enable(DMA0, DMA_CH1);
    
	dma_interrupt_enable(DMA0, DMA_CH1, DMA_INT_FTF);
    nvic_irq_enable(DMA0_Channel1_IRQn, 0, 0);

    dma_channel_enable(DMA0, DMA_CH1);

	adc_dma_mode_enable(ADC1);
}

void MX_ADC3_Init(void)
{
    rcu_periph_clock_enable(RCU_ADC2);

    adc_deinit(ADC2);
	adc_data_alignment_config(ADC2, ADC_DATAALIGN_RIGHT);
    adc_special_function_config(ADC2, ADC_CONTINUOUS_MODE, DISABLE);
    adc_special_function_config(ADC2, ADC_SCAN_MODE, ENABLE);

	/**
	 * Regular channels
	 * motor_1_dc_link, motor_2_dc_link
	 */
    adc_external_trigger_source_config(ADC2, ADC_REGULAR_CHANNEL, ADC2_EXTTRIG_REGULAR_T7_TRGO);
	adc_external_trigger_config(ADC2, ADC_REGULAR_CHANNEL, ENABLE);

    adc_channel_length_config(ADC2, ADC_REGULAR_CHANNEL, 2);
	adc_regular_channel_config(ADC2, 0, ADC_CHANNEL_10, ADC_SAMPLETIME_7POINT5);
	adc_regular_channel_config(ADC2, 1, ADC_CHANNEL_11, ADC_SAMPLETIME_7POINT5);

    adc_enable(ADC2);
	delay(1);
    adc_calibration_enable(ADC2);

	rcu_periph_clock_enable(RCU_DMA0);

	dma_parameter_struct dma_init_struct;

    dma_deinit(DMA0, DMA_CH2);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_addr = (uint32_t)&adc3_dc_link_currents;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_16BIT;
    dma_init_struct.number = 2;
    dma_init_struct.periph_addr = (uint32_t)&ADC_RDATA(ADC2);
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(DMA0, DMA_CH2, &dma_init_struct);
    
    dma_circulation_enable(DMA0, DMA_CH2);
    
	dma_interrupt_enable(DMA0, DMA_CH2, DMA_INT_FTF);
    nvic_irq_enable(DMA0_Channel2_IRQn, 0, 0);

    dma_channel_enable(DMA0, DMA_CH2);

	adc_dma_mode_enable(ADC2);
}

void hardware_init(void) {
	rcu_periph_clock_enable(RCU_AF);
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);

	nvic_irq_enable(MemoryManagement_IRQn, 0, 0);
    nvic_irq_enable(BusFault_IRQn, 0, 0);
    nvic_irq_enable(UsageFault_IRQn, 0, 0);
    nvic_irq_enable(SVCall_IRQn, 0, 0);
    nvic_irq_enable(DebugMonitor_IRQn, 0, 0);
    nvic_irq_enable(PendSV_IRQn, 0, 0);

	SystemClock_Config();

	/**
	 * 96Mhz / 6 = 16Mhz of ADC
	 */
	rcu_adc_clock_config(ADC_CLOCK_PRESCALER_CONFIG);
    rcu_periph_clock_disable(RCU_DMA0);
    
    MX_GPIO_Init();
    MX_TIM_Init();
    MX_ADC1_Init();
    MX_ADC2_Init();

	USART_B_Init(19200);
}

void activate_latch(void) {
	gpio_bit_write(GPIOA, GPIO_PIN_5, SET);
}

void reset_watchog(void)
{
	fwdgt_counter_reload();
}

int is_button_pressed(void) {
	return gpio_input_bit_get(GPIOA, GPIO_PIN_1);
}

void light_led()
{
	//no led
}

void start_adc()
{
	//do nothing already started?
}

int is_uart3_available(void) {
	FlagStatus status = usart_flag_get(USART1, USART_FLAG_TC);

	return SET == status;
}

void uart3_transmit(uint8_t *data, int size)
{
	SendBuffer(USART1, data, size);
}

uint8_t non_dma_rx[SERIAL_BUFFER_SIZE] = { 'a', 'b', 'c', 'a', 'v' };

volatile uint32_t received_length = 0;
volatile uint32_t old_position = 0;
volatile uint32_t current_buffer_position = 0;
volatile uint32_t dma_count = 0;
volatile uint32_t test_counter = 0;
volatile uint8_t data_received = 0;

void process_usart_rx(uint32_t dma_remaining_count) {
	data_received = true;

	test_counter++;

	dma_count = dma_remaining_count;
	received_length = 0;
	current_buffer_position = usart1_rx_buf_len - dma_remaining_count;

	// if (current_buffer_position != old_position)
	// {
	// 	if (current_buffer_position > old_position)
	// 	{
	// 		received_length = current_buffer_position - old_position;
	// 		memcpy(non_dma_rx, &usart1_rx_buf[old_position], received_length);
	// 	}
	// 	else if ((usart1_rx_buf_len - old_position + current_buffer_position) > 0)
	// 	{
	// 		uint32_t head_length = current_buffer_position;
	// 		uint32_t tail_length = usart1_rx_buf_len - old_position;

	// 		memcpy(non_dma_rx, &usart1_rx_buf[old_position], tail_length);

	// 		if (head_length > 0)
	// 		{
	// 			memcpy(&non_dma_rx[usart1_rx_buf_len - old_position], &usart1_rx_buf[0], head_length);
	// 		}

	// 		received_length = head_length + tail_length;
	// 	}
	// }

	// memcpy(non_dma_rx, usart1_rx_buf, SERIAL_BUFFER_SIZE);

	old_position = current_buffer_position; // Update old position
	if (old_position == usart1_rx_buf_len)
	{
		old_position = 0;
	}

	//usart3_rx_check(current_buffer_position);
}

void USARTx_IRQHandler(uint32_t handle) {
	if (handle == USART0) {

	} else if (handle == USART1) {

	}
}

void board_usart0_rx_check() {
	
}

void board_usart1_rx_check() {
	
}

void init_eeprom(void)
{
	//TOOD:
}

void read_configuration_value(uint16_t address, uint16_t *value)
{
	//TODO:
}

void read_configuration(uint16_t *configuration)
{
	//TOOD:
}

FlagStatus buzzerToggle = RESET;

void toggle_buzzer(void)
{
	buzzerToggle = buzzerToggle == RESET ? SET : RESET;

	//digitalWrite(BUZZER, buzzerToggle);
}

void switch_buzzer_off(void)
{
	//digitalWrite(BUZZER, RESET);
}

void unit_uart2_dma(uint8_t *buffer, int size)
{
  //no dma
}

void unit_uart3_dma(uint8_t *buffer, int size)
{
  //no dma
}

#include "Board/Gd32F130C8T6/comms.h"
#include "board.h"
#include "logger.h"
#include "config.h"

void uart_putchar(char *data)
{
    SendBuffer(UART3, (uint8_t *)data, 1);
}

Logger logger = {
    .putchar = uart_putchar
};

Hardware hardware = {
    .activate_latch = activate_latch,
    .hardware_init = hardware_init,
    .is_button_pressed = is_button_pressed,
    .light_led = light_led,
    .start_adc = start_adc,

	.toggle_buzzer = toggle_buzzer,
	.switch_buzzer_off = switch_buzzer_off,

    .is_uart3_available = is_uart3_available,
    .uart3_transmit = uart3_transmit,
    .unit_uart2_dma = unit_uart2_dma,
    .unit_uart3_dma = unit_uart3_dma,

    .init_eeprom = init_eeprom,
    .read_configuration = read_configuration,
    .read_configuration_value = read_configuration_value,

	.reset_watchdog = reset_watchog,
    .reset = reset
};

void usart_b_send_byte(uint8_t data) {
    usart_data_transmit(USART2 , data);
	
    while(usart_flag_get(USART2, USART_FLAG_TBE) == RESET)
	{
	}
}

void usart_b_send_string(const char *str) {
	while(*str) {
		usart_b_send_byte(*str++);
	}
}

/**
 * battery voltage filter coefficient in fixed-point
 *
 * 		coef_fixedPoint = coef_floatingPoint * 2^16
 * 
 * In this case 655 = 0.01 * 2^16
 */
#define LOW_PASS_FILT_COEF	655
#define FLOAT_TO_FIXED(x) ((int32_t)((x) * 65536.0f))

int16_t temperatureAdc = 1693;
static int32_t temperatureAdcFixdt = FLOAT_TO_FIXED(1693);

#define BAT_CALIB_ADC 	1170
#define BAT_CALIB_REAL_VOLTAGE  3000

int32_t batVoltage = (3000 * BAT_CALIB_ADC) / BAT_CALIB_REAL_VOLTAGE;
static int32_t batVoltageFixdt = (3000 * BAT_CALIB_ADC) / BAT_CALIB_REAL_VOLTAGE << 16;

/* Low pass filter fixed-point 32 bits: fixdt(1,32,16)
 * Max:  32767.99998474121
 * Min: -32768
 * Res:  1.52587890625e-05
 *
 * Inputs:       u     = int16 or int32
 * Outputs:      y     = fixdt(1,32,16)
 * Parameters:   coef  = fixdt(0,16,16) = [0,65535U]
 *
 * Example:
 * If coef = 0.8 (in floating point), then coef = 0.8 * 2^16 = 52429 (in fixed-point)
 * filtLowPass16(u, 52429, &y);
 * yint = (int16_t)(y >> 16); // the integer output is the fixed-point ouput shifted by 16 bits
 */
void filtLowPass32(int32_t u, uint16_t coef, int32_t *y)
{
	int64_t tmp;
	tmp = ((int64_t)((u << 4) - (*y >> 12)) * coef) >> 4;
	tmp = CLAMP(tmp, -2147483648LL, 2147483647LL); // Overflow protection: 2147483647LL = 2^31 - 1
	*y = (int32_t)tmp + (*y);
}

int main() {
	oscilloscope_init(&oscilloscope, oscilloscope_data, OSCILLOSCOPE_DATA_SIZE);

	hardware.hardware_init();
	hardware.activate_latch();

	// Loop until button is released
	while (is_button_pressed())
	{
		delay(10);
	}

	uint32_t pwmPeriod = SystemCoreClock / 2 / PWM_FREQ;

	timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0, pwmPeriod / 8);
	timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_1, pwmPeriod / 4);
	timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_2, pwmPeriod / 2);

	timer_primary_output_config(TIMER0, DISABLE);
	timer_primary_output_config(TIMER7, DISABLE);

	MX_Tim_Start();

	uint8_t power_off_counter = 0;
	uint16_t oscilloscope_latest = oscilloscope.index;

	if (oscilloscope.status == ACTIVE)
	{
		printf("oscilloscope:started\r\n");
	}

	while (1) {
		while (oscilloscope.index > oscilloscope_latest && oscilloscope.status != FINISHED)
		{
			usart_b_send_byte(oscilloscope.data[oscilloscope_latest]);
			usart_b_send_byte(oscilloscope.data[oscilloscope_latest] >> 8);

			oscilloscope_latest++;
		}

		if (oscilloscope.status == FINISHED)
		{
			printf("oscilloscope:finished\r\n");
		}

		/**
		 * Temperature (°C) = {(V25 – Vtemperature (digit)) / Avg_Slope} + 25.
		 * 	= V25 / Avg_Slope - Vtemperature / Avg_Slope + 25
		 *  =  const(V25 / Avg_Slope + 25) - digit * const(3.3 / 4095 / Avg_Slope)
		 *	+---------------------------------------------------------------------+
		 *	| Table 4-38. Temperature sensor characteristics (1)                  |
		 *	+-------------------+----------------------+------+------+------+-----+
		 *	| Symbol            | Parameter            | Min  | Typ  | Max  |Unit|
		 *	+-------------------+----------------------+------+------+------+-----+
		 *	| TL                | VSENSE linearity with|  —   | ±1.5 |  —   | °C |
		 *	|                   | temperature          |      |      |      |    |
		 *	+-------------------+----------------------+------+------+------+-----+
		 *	| Avg_Slope         | Average slope        |  —   | 4.1  |  —   |mV/°C|
		 *	+-------------------+----------------------+------+------+------+-----+
		 *	| V25               | Voltage at 25 °C     |  —   | 1.45 |  —   | V  |
		 *	+-------------------+----------------------+------+------+------+-----+
		 *	| tS_temp (2)       | ADC sampling time    |  —   | 17.1 |  —   | μs |
		 *	|                   | when reading the     |      |      |      |    |
		 *	|                   | temperature          |      |      |      |    |
		 *	+-------------------+----------------------+------+------+------+-----+
		 */

		/**
		 * 	(1.45 - 1800 * 3.3 / 4095) / 0.0041 + 25
		 *	24.86598767086572
		 *	(1.45 / 0.0041 + 25) - 1800 * (3.3 / 4095 / 0.0041)
		 *	24.865987670865707
		 *	(1.45 / 0.0041 + 25)
		 *	378.6585365853658
		 *	(3.3 / 4095 / 0.0041)
		 *	0.19655141606361118
		 */
		
		filtLowPass32(adc1_regular_values.temp, LOW_PASS_FILT_COEF, &temperatureAdcFixdt);
		int32_t temperature = (
			FLOAT_TO_FIXED(378.6585365853658f) - ((int64_t)temperatureAdcFixdt >> 16) * FLOAT_TO_FIXED(0.19655141606361118f)
		) >> 16;

		filtLowPass32(adc1_regular_values.batt1, LOW_PASS_FILT_COEF, &batVoltageFixdt);
    	int16_t batRealCalibratedVoltage = (int16_t)(batVoltageFixdt >> 16) * BAT_CALIB_REAL_VOLTAGE / BAT_CALIB_ADC;

		printf("batt adc: %u batt voltage: %u adc temp: %u temp: %li l_tx2: %u l_rx2: %u dc link 1: %u dc link 2: %u \r\n",
			adc1_regular_values.batt1,
			batRealCalibratedVoltage,
			adc1_regular_values.temp,
			temperature,
			adc2_regular_values.l_tx2,
			adc2_regular_values.l_rx2,
			adc3_dc_link_currents.motor_1_dc_link_current,
			adc3_dc_link_currents.motor_2_dc_link_current);
		
		power_off_counter = 0;
		while (hardware.is_button_pressed())
		{
			power_off_counter++;

			Delay(10);
		}

		if (power_off_counter > 5) {
			reset();
		}

		Delay(1000);
	}

	reset();

	while(1)
	{
	}
}

/* =========================== Retargeting printf =========================== */
/* retarget the C library printf function to the USART */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
PUTCHAR_PROTOTYPE
{
	usart_b_send_byte(ch);

	return ch;
}

#ifdef __GNUC__
int _write(int file, char *data, int len)
{
	int i;
	for (i = 0; i < len; i++)
	{
		__io_putchar(*data++);
	}
	return len;
}
#endif
