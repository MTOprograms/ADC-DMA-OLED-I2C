#ifndef INC_ADC_H_
#define INC_ADC_H_

#include "stm32f7xx.h"
#include "system_stm32f7xx.h"

extern volatile uint16_t ADC1_Wartosc;
extern float ADC1_Napiecie;
extern float ADC1_Procent_f;
extern uint8_t ADC1_Procent;

extern void ADC1_Inicjalizacja(void);                                         // Konfiguruj piny odpowiedzialne za obsługę wskaźnikowych diod LED.
extern void ADC1_OdczytajNapiecie(void);
#endif
