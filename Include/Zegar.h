#ifndef ZEGAR_H_
#define ZEGAR_H_

#include "stm32f7xx.h"
#include "system_stm32f7xx.h"

extern volatile uint32_t Zegar_SysTickMs;                                     // Deklaracja zmiennej dla licznika tyknięć SysTick w ms.

extern void Zegar_Wewn_Inicjalizacja(void);
extern void Zegar_Delay_ms(uint32_t CzasDelayMs);                             // Deklaracja funkcji Delay_ms.
#endif
