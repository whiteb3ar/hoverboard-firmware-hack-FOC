#ifndef DEFINES_PLATFORM_H
#define DEFINES_PLATFORM_H

#include "gd32f1x0.h"
#include "config.h"
	
#define TARGET_nvic_irq_enable(a, b, c){nvic_irq_enable(a, b, c);}
#define TARGET_nvic_priority_group_set(a){nvic_priority_group_set(a);}
#define TARGET_adc_vbat_disable(){adc_vbat_disable();}

#define TARGET_ADC_RDATA ADC_RDATA

#endif