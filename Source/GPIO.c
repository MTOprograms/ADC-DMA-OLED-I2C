#include "GPIO.h"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------- INFORMACJA O PRACY URZĄDZENIA - DIODY LED ---------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void GPIO_Inicjalizacja(void)
{                                                                             //-----------------------------------------------------
                                                                              //--------------- KONFIGURACJA ZEGARÓW ----------------
                                                                              //-----------------------------------------------------
  RCC -> AHB1ENR   |= RCC_AHB1ENR_GPIOBEN;                                    // Ustaw w rejestrze AHB1ENR włączony zegar dla portu GPIOB.

                                                                              //---------------------------------------------------
                                                                              //------------- PB0 - ZIELONA DIODA LED -------------
                                                                              //---------------------------------------------------
  GPIOB -> MODER   |= GPIO_MODER_MODER0_0;                                    // Ustaw w rejestrze MODER pin PB0 na tryb Wyjścia - Output Mode (01).
  GPIOB -> MODER   &= ~GPIO_MODER_MODER0_1;                                   // Drugi bit zeruj, aby był tryb Wyjścia.
  GPIOB -> OTYPER  &= ~GPIO_OTYPER_OT0_Msk;                                   // Ustaw w rejestrze OTYPER pin PB0 na tryb Push-Pull (0).
  GPIOB -> OSPEEDR &= ~GPIO_OSPEEDR_OSPEEDR0_Msk;                             // Ustaw w rejestrze OSPEEDR niską szybkość pinu PB0 (2-10MHz).
  GPIOB -> PUPDR   &= ~GPIO_PUPDR_PUPDR0_Msk;                                 // Ustaw w rejestrze PUPDR brak rezystora pull-up/pull-down dla pinu PB0 (floating).

                                                                              //---------------------------------------------------
                                                                              //------------ PB7 - NIEBIESKA DIODA LED ------------
                                                                              //---------------------------------------------------
  GPIOB -> MODER   |= GPIO_MODER_MODER7_0;                                    // Ustaw w rejestrze MODER pin PB7 na tryb Wyjścia - Output Mode (01).
  GPIOB -> MODER   &= ~GPIO_MODER_MODER7_1;                                   // Drugi bit zeruj, aby był tryb Wyjścia.
  GPIOB -> OTYPER  &= ~GPIO_OTYPER_OT7_Msk;                                   // Ustaw w rejestrze OTYPER pin PB7 na tryb Push-Pull (0).
  GPIOB -> OSPEEDR &= ~GPIO_OSPEEDR_OSPEEDR7_Msk;                             // Ustaw w rejestrze OSPEEDR niską szybkość pinu PB7 (2-10MHz).
  GPIOB -> PUPDR   &= ~GPIO_PUPDR_PUPDR7_Msk;                                 // Ustaw w rejestrze PUPDR brak rezystora pull-up/pull-down dla pinu PB7 (floating).

                                                                              //---------------------------------------------------
                                                                              //------------ PB14 - CZERWONA DIODA LED ------------
                                                                              //---------------------------------------------------
  GPIOB -> MODER   |= GPIO_MODER_MODER14_0;                                   // Ustaw w rejestrze MODER pin PB14 na tryb Wyjścia - Output Mode (01).
  GPIOB -> MODER   &= ~GPIO_MODER_MODER14_1;                                  // Drugi bit zeruj, aby był tryb Wyjścia.
  GPIOB -> OTYPER  &= ~GPIO_OTYPER_OT14_Msk;                                  // Ustaw w rejestrze OTYPER pin PB14 na tryb Push-Pull (0).
  GPIOB -> OSPEEDR &= ~GPIO_OSPEEDR_OSPEEDR14_Msk;                            // Ustaw w rejestrze OSPEEDR niską szybkość pinu PB14 (2-10MHz).
  GPIOB -> PUPDR   &= ~GPIO_PUPDR_PUPDR14_Msk;                                // Ustaw w rejestrze PUPDR brak rezystora pull-up/pull-down dla pinu PB14 (floating). 
}