#ifndef INC_I2C_H_
#define INC_I2C_H_

#include "stm32f7xx.h"
#include "system_stm32f7xx.h"
#include "Zegar.h"

#define OLED_TIMING    0x00303D5B                                             // Częstotliwość pracy magistrali I2C - ok. 100 kHz.

extern void I2C1_Inicjalizacja(void);                                         // Konfiguruj piny odpowiedzialne za obsługę wskaźnikowych diod LED.
#endif
