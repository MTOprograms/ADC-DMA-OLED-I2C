#include "I2C.h"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------- I2C1 - EKRAN OLED ---------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void I2C1_Inicjalizacja(void)
{                                                                             //-----------------------------------------------------
                                                                              //--------------- KONFIGURACJA ZEGARÓW ----------------
                                                                              //-----------------------------------------------------
  RCC -> AHB1ENR  |= RCC_AHB1ENR_GPIOBEN;                                     // Ustaw w rejestrze AHB1ENR włączony zegar dla portu GPIOB.
  RCC -> APB1ENR  |= RCC_APB1ENR_I2C1EN;                                      // Ustaw w rejestrze APB1ENR włączony zegar dla I2C1.
  RCC -> APB1RSTR |= RCC_APB1RSTR_I2C1RST;                                    // Twardy reset I2C1.
  RCC -> APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;                                   // Twardy reset I2C1.

                                                                              //---------------------------------------------------
                                                                              //---- KONFIGURACJA ZASILANIA EKRANU OLED - PB15 ----
                                                                              //---------------------------------------------------
  GPIOB -> MODER   |= GPIO_MODER_MODER15_0;                                   // Ustaw w rejestrze MODER pin PB15 na tryb Wyjścia - Output Mode (01).
  GPIOB -> MODER   &= ~GPIO_MODER_MODER15_1;                                  // Drugi bit zeruj, aby był tryb Wyjścia.
  GPIOB -> OTYPER  &= ~GPIO_OTYPER_OT15_Msk;                                  // Ustaw w rejestrze OTYPER pin PB15 na tryb Push-Pull (0).
  GPIOB -> OSPEEDR &= ~GPIO_OSPEEDR_OSPEEDR15_Msk;                            // Ustaw w rejestrze OSPEEDR niską szybkość pinu PB15 (2-10MHz).
  GPIOB -> PUPDR   &= ~GPIO_PUPDR_PUPDR15_Msk;                                // Ustaw w rejestrze PUPDR brak rezystora pull-up/pull-down dla pinu PB15 (floating).  
  GPIOB -> BSRR    |= GPIO_BSRR_BS15;                                         // Ustaw w rejestrze BSRR stan wysoki na pinie PB15 (Zasilanie ekranu OLED).
  Zegar_Delay_ms(200ul);                                                      // Opóźnienie 200ms.

                                                                              //---------------------------------------------------
                                                                              //----------- KONFIGURACJA I2C SCL - PB8 ------------
                                                                              //---------------------------------------------------
  GPIOB -> MODER   &= ~GPIO_MODER_MODER8_Msk;                                 // Ustaw w rejestrze MODER pin PB8 na Input Mode (00).
  GPIOB -> MODER   |=  GPIO_MODER_MODER8_1;                                   // Ustaw w rejestrze MODER pin PB8 na Alternate Function (10).
  GPIOB -> OTYPER  |=  GPIO_OTYPER_OT8;                                       // Ustaw w rejestrze OTYPER pin PB8 jako open-drain (01).
  GPIOB -> OSPEEDR |=  GPIO_OSPEEDER_OSPEEDR8;                                // Ustaw w rejestrze OSPEEDR pin PB8 na High Speed (11).
  GPIOB -> PUPDR   &= ~GPIO_PUPDR_PUPDR8_Msk;                                 // Ustaw w rejestrze PUPDR pin PB8 jako No Pull-Up / No Pull-Down (00) - są zewn. rezystory.
  GPIOB -> AFR[1]  &= ~GPIO_AFRH_AFRH0_Msk;                                   // W wyższej części rejestru AFR ustaw funkcję alternatywną AF0 dla pinu PB8.
  GPIOB -> AFR[1]  |= (4 << GPIO_AFRH_AFRH0_Pos);                             // W wyższej części rejestru AFR ustaw funkcję alternatywną AF4 (I2C SCL) dla pinu PB8.

                                                                              //---------------------------------------------------
                                                                              //----------- KONFIGURACJA I2C SDA - PB9 ------------
                                                                              //---------------------------------------------------
  GPIOB -> MODER   &= ~GPIO_MODER_MODER9_Msk;                                 // Ustaw w rejestrze MODER pin PB9 na Input Mode (00).
  GPIOB -> MODER   |=  GPIO_MODER_MODER9_1;                                   // Ustaw w rejestrze MODER pin PB9 na Alternate Function (10).
  GPIOB -> OTYPER  |=  GPIO_OTYPER_OT9;                                       // Ustaw w rejestrze OTYPER pin PB9 jako open-drain (01).
  GPIOB -> OSPEEDR |=  GPIO_OSPEEDER_OSPEEDR9;                                // Ustaw w rejestrze OSPEEDR pin PB9 na High Speed (11).
  GPIOB -> PUPDR   &= ~GPIO_PUPDR_PUPDR9_Msk;                                 // Ustaw w rejestrze PUPDR pin PB8 jako No Pull-Up / No Pull-Down (00) - są zewn. rezystory.
  GPIOB -> AFR[1]  &= ~GPIO_AFRH_AFRH1_Msk;                                   // W wyższej części rejestru AFR ustaw funkcję alternatywną AF0 dla pinu PB9.
  GPIOB -> AFR[1]  |= (4 << GPIO_AFRH_AFRH1_Pos);                             // W wyższej części rejestru AFR ustaw funkcję alternatywną AF4 (I2C SDA) dla pinu PB9.

                                                                              //---------------------------------------------------
                                                                              //---------------- KONFIGURACJA I2C -----------------
                                                                              //---------------------------------------------------
  I2C1 -> CR1 &= ~I2C_CR1_PE;                                                 // Wyłącz I2C przed zmianą TIMINGR.
  I2C1 -> ICR  = I2C_ICR_STOPCF | I2C_ICR_NACKCF;                             // Wyczyść flagi STOP i NACK przed ponownym startem.
  I2C1 -> ICR  = I2C_ICR_BERRCF | I2C_ICR_ARLOCF;                             // Wyczyść flagi błędu magistrali i utraty arbitrażu.
  I2C1 -> TIMINGR = OLED_TIMING;                                              // TIMINGR – wartość z CubeMX/AN4235 dla ~100 kHz przy rodzinie F7.
  I2C1 -> CR1 &= ~I2C_CR1_ANFOFF;                                             // Ustaw w rejestrze CR1 włączony filtr analogowy.
  I2C1 -> CR1 &= ~I2C_CR1_DNF;                                                // Ustaw w rejestrze CR1 wyłączony filtr cyfrowy.
  I2C1 -> CR1 |= I2C_CR1_PE;                                                  // Włącz I2C.
}