#include "it.h"
#include "board.h"
#include "comms.h"
#include "app.h"
#include "buzzer.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

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

// timeout timer parameter structs
timer_parameter_struct timeoutTimer_paramter_struct;

// PWM timer Parameter structs
timer_parameter_struct timerBldc_paramter_struct;	
timer_break_parameter_struct timerBldc_break_parameter_struct;
timer_oc_parameter_struct timerBldc_oc_parameter_struct;

// DMA (USART) structs
dma_parameter_struct dma_init_struct_usart;

//uint8_t usartMasterSlave_rx_buf[USART_MASTERSLAVE_RX_BUFFERSIZE];
//uint8_t usartSteer_COM_rx_buf[USART_STEER_COM_RX_BUFFERSIZE];

#define ARRAY_LEN(x) (uint32_t)(sizeof(x) / sizeof(*(x)))

uint8_t usart0_rx_buf[1];
uint8_t usart1_rx_buf[SERIAL_BUFFER_SIZE]; // USART Rx DMA circular buffer
uint32_t usart1_rx_buf_len = ARRAY_LEN(usart1_rx_buf);

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

//----------------------------------------------------------------------------
// Initializes the GPIOs
//----------------------------------------------------------------------------
void GPIO_init(void)
{
	rcu_periph_clock_enable(RCU_GPIOA);

	gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_5);
	gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, GPIO_PIN_1);
}

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

void SystemClock_96MHz_IRC8M(void) {
	rcu_deinit();

    // 1. HSI (8 Mhz)
    rcu_osci_on(RCU_IRC8M);
    while (rcu_osci_stab_wait(RCU_IRC8M) == ERROR) {}

	rcu_ahb_clock_config(RCU_AHB_CKSYS_DIV1);    // AHB = 96 Mhz
    rcu_apb1_clock_config(RCU_APB1_CKAHB_DIV2);   // APB1 = 48 Mhz (max 54 Mhz)
    rcu_apb2_clock_config(RCU_APB2_CKAHB_DIV1);   // APB2 = 96 Mhz

    rcu_pll_config(RCU_PLLSRC_IRC8M_DIV2, RCU_PLL_MUL24);

    rcu_osci_on(RCU_PLL_CK);
    while (rcu_osci_stab_wait(RCU_PLL_CK) == ERROR) {}

    rcu_system_clock_source_config(RCU_CKSYSSRC_PLL);
    while (rcu_system_clock_source_get() != RCU_SCSS_PLL) {}
}


void hardware_init(void) {
	//SystemClock_96MHz_IRC8M();
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / 1000);

	GPIO_init();
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

#include <platform.h>

void delay(uint16_t ms) {
    Delay(ms);
}

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

int main() {
	hardware.hardware_init();
	hardware.activate_latch();

	// Loop until button is released
	while (is_button_pressed())
	{
		delay(10);
	}

	uint8_t counter = 0;

	while (counter++ != 10) {
		delay(1000);

		uint16_t porta = gpio_input_port_get(GPIOA);

		printf("Core clock %u port a: ", SystemCoreClock);

		for (int i = 15; i >= 0; i--) {
			printf("%u", (porta >> i) & 1);
		}

		printf(" \r\n");

		usart_b_send_string("test message --\r\n");

		if (is_button_pressed()) {
			reset();
		}
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
