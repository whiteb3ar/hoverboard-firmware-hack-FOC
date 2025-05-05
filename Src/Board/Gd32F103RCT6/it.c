#include "it.h"
#include "app.h"
#include "board.h"
#include "config.h"

#include <platform.h>

uint32_t msTicks;
uint32_t timeoutCounter_ms = 0;
FlagStatus timedOut = RESET;

extern int32_t steer;
extern int32_t speed;
extern FlagStatus beepsBackwards;

//----------------------------------------------------------------------------
// SysTick_Handler
//----------------------------------------------------------------------------
void SysTick_Handler(void)
{
  msTicks++;
}

//----------------------------------------------------------------------------
// Resets the timeout to zero
//----------------------------------------------------------------------------
void ResetTimeout(void)
{
  timeoutCounter_ms = 0;
}

//----------------------------------------------------------------------------
// Expected to run with 1kHz -> interrupt every 1ms
//----------------------------------------------------------------------------
void TIMER3_IRQHandler(void)
{	
	if (timeoutCounter_ms > 2000)
	{
		if (timedOut == RESET)
		{
			//do reset if required
		}
		
		timedOut = SET;
	}
	else
	{
		timedOut = RESET;
		timeoutCounter_ms++;
	}
	
	timer_interrupt_flag_clear(TIMER3, TIMER_INT_UP);
}

//----------------------------------------------------------------------------
// Timer0_Update_Handler
// Is called when upcouting of timer0 is finished and the UPDATE-flag is set
// AND when downcouting of timer0 is finished and the UPDATE-flag is set
// -> pwm of timer0 running with 16kHz -> interrupt every 31,25us
//----------------------------------------------------------------------------
void TIMER0_BRK_IRQHandler(void)
{
	// Start ADC conversion
	adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
	
	// Clear timer update interrupt flag
	timer_interrupt_flag_clear(RCU_TIMER0, TIMER_INT_UP);
}

extern uint32_t steerCounter;								// Steer counter for setting update rate

//----------------------------------------------------------------------------
// This function handles DMA_Channel0_IRQHandler interrupt
// Is called, when the ADC scan sequence is finished
// -> ADC is triggered from timer0-update-interrupt -> every 31,25us
//----------------------------------------------------------------------------
void DMA_Channel0_IRQHandler(void)
{
	main_bldc_irq_loop();
	
	if (dma_interrupt_flag_get(DMA0, DMA_CH0, DMA_INT_FLAG_FTF))
	{
		dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_FTF);        
	}
}

uint16_t iUartCounter = 0;

void USART3_IRQHandler(void) {
	if (usart_interrupt_flag_get(USART0, USART_INT_FLAG_IDLE)) {
		uint8_t temp = USART_RDATA(USART0);
		(void)temp;

		usart_interrupt_flag_clear(USART0, USART_INT_FLAG_IDLE);

		board_usart1_rx_check();
	}
}

#ifdef HAS_USART0
	// This function handles DMA_Channel1_2_IRQHandler interrupt
	// Is asynchronously called when USART0 RX finished
	void DMA_Channel1_2_IRQHandler(void)
	{
		//DEBUG_LedSet(	(steerCounter%20) < 10	,0)
		// USART steer/bluetooth RX
		if (dma_interrupt_flag_get(DMA_CH2, DMA_INT_FLAG_FTF))
		{
			//DEBUG_LedSet(	(iUartCounter++%10) < 5	,0)
			#if defined(USART0_REMOTE) && defined(MASTER_OR_SINGLE)
					RemoteCallback();
			#elif defined(USART0_MASTERSLAVE) && defined(MASTER_OR_SLAVE)
					UpdateUSARTMasterSlaveInput();
					// Update USART bluetooth input mechanism
					//UpdateUSARTBluetoothInput();
			#endif
			dma_interrupt_flag_clear(DMA_CH2, DMA_INT_FLAG_FTF);        
		}
	}
#endif

#ifdef HAS_USART1
	void USART1_IRQHandler(void) {
		if (usart_interrupt_flag_get(USART1, USART_INT_FLAG_IDLE)) {
			uint8_t temp = USART_RDATA(USART1);
        	(void)temp;

			usart_interrupt_flag_clear(USART1, USART_INT_FLAG_IDLE);

			board_usart1_rx_check();
		}
	}

	//----------------------------------------------------------------------------
	// This function handles DMA_Channel3_4_IRQHandler interrupt
	// Is asynchronously called when USART_SLAVE RX finished
	//----------------------------------------------------------------------------
	void DMA_Channel3_4_IRQHandler(void)
	{
		//DEBUG_LedSet(	(steerCounter%10) < 5	,0)
		// USART master slave RX
		if (dma_interrupt_flag_get(DMA_CH4, DMA_INT_FLAG_FTF))
		{
			#if defined(USART1_REMOTE) && defined(MASTER_OR_SINGLE)
					RemoteCallback();
			#elif defined(USART1_MASTERSLAVE) && defined(MASTER_OR_SLAVE)
					UpdateUSARTMasterSlaveInput();
					// Update USART bluetooth input mechanism
					//UpdateUSARTBluetoothInput();
			#endif
			
			dma_interrupt_flag_clear(DMA_CH4, DMA_INT_FLAG_FTF);        
		}
	}
#endif

//----------------------------------------------------------------------------
// Returns number of milliseconds since system start
//----------------------------------------------------------------------------
uint32_t millis()
{
	return msTicks;
}

//----------------------------------------------------------------------------
// Delays number of tick Systicks (happens every 10 ms)
//----------------------------------------------------------------------------
void Delay (uint32_t dlyTicks)
{
  uint32_t curTicks;

  curTicks = msTicks;
  while ((msTicks - curTicks) < dlyTicks)
	{
		__NOP();
	}
}

//----------------------------------------------------------------------------
// This function handles Non maskable interrupt.
//----------------------------------------------------------------------------
void NMI_Handler(void)
{
}

//----------------------------------------------------------------------------
// This function handles Hard fault interrupt.
//----------------------------------------------------------------------------
void HardFault_Handler(void)
{
  while(1) {}
}

//----------------------------------------------------------------------------
// This function handles Memory management fault.
//----------------------------------------------------------------------------
void MemManage_Handler(void)
{
  while(1) {}
}

//----------------------------------------------------------------------------
// This function handles Prefetch fault, memory access fault.
//----------------------------------------------------------------------------
void BusFault_Handler(void)
{
  while(1) {}
}

//----------------------------------------------------------------------------
// This function handles Undefined instruction or illegal state.
//----------------------------------------------------------------------------
void UsageFault_Handler(void)
{
  while(1) {}
}

//----------------------------------------------------------------------------
// This function handles System service call via SWI instruction.
//----------------------------------------------------------------------------
void SVC_Handler(void)
{
}

//----------------------------------------------------------------------------
// This function handles Debug monitor.
//----------------------------------------------------------------------------
void DebugMon_Handler(void)
{
}

//----------------------------------------------------------------------------
// This function handles Pendable request for system service.
//----------------------------------------------------------------------------
void PendSV_Handler(void)
{
}
