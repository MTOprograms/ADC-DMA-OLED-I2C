#include "Zegar.h"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------- DEFINICJA ZMIENNYCH GLOBALNYCH ---------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
volatile uint32_t Zegar_SysTickMs = 0ul;                                      // Licznik SysTick, aktualizowany co 1ms.

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------- USTAWIENIE ZEGARA, PRESKALERÓW, PĘTLI PLL - 216MHZ -----------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Zegar_Wewn_Inicjalizacja(void)
{
  RCC -> APB1ENR |= RCC_APB1ENR_PWREN;                                        // Włączenie zegara dla bloku zasilania (PWR).
  __DSB();                                                                    // Zapewnia, że wszystkie wcześniejsze operacje pamięci i systemowe są zakończone przed kontynuacją.

  FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY_Msk) | FLASH_ACR_LATENCY_7WS; // Ustawienie opóźnienia dostępu do FLASH na 7 wait state (7 taktów zegara).
  while ((FLASH -> ACR & FLASH_ACR_LATENCY_Msk) != FLASH_ACR_LATENCY_7WS){}   // Czekaj, aż zmiana opóźnienia FLASH zostanie faktycznie zatwierdzona.

  PWR -> CR1 = (PWR -> CR1 & ~PWR_CR1_VOS_Msk) |                              // Ustawienie skalowania napięcia VOS na najwyższą wydajność (skalowanie 1).
               (PWR_CR1_VOS_0 | PWR_CR1_VOS_1);                               //

  PWR -> CR1 |= PWR_CR1_ODEN;                                                 // Włączenie trybu przetaktowania (Over-drive).
  RCC -> CR = (RCC -> CR & ~RCC_CR_HSITRIM_Msk) | (16 << RCC_CR_HSITRIM_Pos); // Ustaw w rejestrze CR kalibrację wewnętrznego oscylatora HSI na domyślną wartość (środek zakresu - 16).

  RCC -> CR |= RCC_CR_HSION;                                                  // Ustaw w rejestrze CR włączony wewnętrzny oscylator HSI (High Speed Internal).
  while ((RCC->CR & RCC_CR_HSIRDY) != RCC_CR_HSIRDY){}                        // Czekaj, aż oscylator HSI będzie gotowy (ustawi się flaga HSIRDY).

  RCC -> PLLCFGR = (RCC->PLLCFGR & ~(RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM  | //
                                     RCC_PLLCFGR_PLLN   | RCC_PLLCFGR_PLLP))| //
                   (8U << RCC_PLLCFGR_PLLM_Pos)   |                           // Dzielnik wejściowy PLLM = 8.
                   (216U << RCC_PLLCFGR_PLLN_Pos) |                           // Mnożnik PLLN = 216.
                   (0U << RCC_PLLCFGR_PLLP_Pos)   |                           // Dzielnik główny PLLP = 2.
                   (0U << RCC_PLLCFGR_PLLSRC_Pos);                            // Źródło PLL: HSI.

  RCC -> CR |= RCC_CR_PLLON;                                                  // Ustaw w rejestrze CR włączony generator PLL (Phase-Locked Loop).
  while (!(RCC -> CR & RCC_CR_PLLRDY)){}                                      // Czekaj, aż PLL się ustabilizuje i będzie gotowy do użycia (flaga PLLRDY).
  while (!(PWR -> CSR1 & PWR_CSR1_VOSRDY)){}                                  // Czekaj, aż regulator napięcia osiągnie gotowość.

  RCC -> CFGR = (RCC -> CFGR & ~RCC_CFGR_HPRE)  | RCC_CFGR_HPRE_DIV1;         // Ustaw w rejestrze CFGR dzielnik magistrali AHB (High-speed bus) na 1 - zegar AHB = SYSCLK.
  RCC -> CFGR = (RCC -> CFGR & ~RCC_CFGR_PPRE1) | RCC_CFGR_PPRE1_DIV4;        // Ustaw w rejestrze CFGR dzielnik magistrali APB1 (Low-speed bus) na 4 - zegar APB1 = AHB/4.
  RCC -> CFGR = (RCC -> CFGR & ~RCC_CFGR_PPRE2) | RCC_CFGR_PPRE2_DIV2;        // Ustaw w rejestrze CFGR dzielnik magistrali APB2 (High-speed bus) na 2 - zegar APB2 = AHB/2.

  RCC -> CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;                 // Ustawienie źródła taktowania systemowego na PLL.
  while ((RCC -> CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL){}                  // Czekaj, aż PLL zostanie ustawione jako aktualne źródło SYSCLK (potwierdzenie przełączenia).

  SysTick -> LOAD  = 216000000UL / 1000UL - 1UL;                              // Konfiguracja SysTick do generowania przerwań co 1 ms.
  SysTick -> VAL   = 0UL;                                                     // Zerowanie bieżącej wartości licznika SysTick.
  SysTick -> CTRL  = SysTick_CTRL_CLKSOURCE_Msk |                             // Źródło taktowania: zegar systemowy.
                     SysTick_CTRL_ENABLE_Msk    |                             // Włączenie SysTick.
                     SysTick_CTRL_TICKINT_Msk;                                // (opcjonalne) Włączenie przerwań SysTick.
  SystemCoreClock = 216000000;                                                // Aktualizacja zmiennej globalnej informującej o częstotliwości systemu.
  RCC -> DCKCFGR1 = (RCC -> DCKCFGR1 & ~RCC_DCKCFGR1_TIMPRE) | 0x00000000U;   // Konfiguracja preskalera zegara dla timerów.

  __DMB();                                                                    // Wszystkie operacje pamięciowe przed tą barierą zakończą się, zanim rozpoczną się operacje po niej.

  MPU -> CTRL = 0U;                                                           // Wyłączenie jednostki MPU (Memory Protection Unit).
  MPU -> RNR = 0x00U;                                                         // Wybór regionu 0.
  MPU -> RBAR = 0x00000000U & 0xFFFFFFE0U;                                    // Baza regionu – adres 0, wyrównany do 32 bajtów.

  MPU -> RASR = MPU_RASR_ENABLE_Msk          |                                // Włączenie regionu.
                (0x87U << MPU_RASR_SRD_Pos)  |                                // Maska subregionów – dezaktywacja części regionu.
                (0x1FU << MPU_RASR_SIZE_Pos) |                                // Rozmiar regionu = 4GB.
                (0x00U << MPU_RASR_TEX_Pos)  |                                // TEX = 0 (domyślna klasa pamięci).
                (0x00U << MPU_RASR_AP_Pos)   |                                // Brak dostępu (Access Permissions = No Access).
                MPU_RASR_XN_Msk              |                                // Zakaz wykonywania instrukcji (Execute Never).
                MPU_RASR_S_Msk               |                                // Pamięć współdzielona (Shareable).
                (0x00U)                      |                                // Nie-cache’owana.
                (0x00U);                                                      // Nie-buforowana.
  MPU -> CTRL = MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk;                // Włączenie MPU i domyślnych uprawnień dla trybu uprzywilejowanego.

  __DSB();                                                                    // Zapewnia, że wszystkie wcześniejsze operacje pamięci i systemowe są zakończone przed kontynuacją.
  __ISB();                                                                    // Synchronizuje potok instrukcji. Używana po zmianie ustawień systemowych lub pamięci, nowe instrukcje zostaną wykonane z aktualnymi danymi.
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------- OPÓŹNIENIE BLOKUJĄCE (MS) -----------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Zegar_Delay_ms(uint32_t CzasDelayMs)
{
  uint32_t PoczatekBlokady = Zegar_SysTickMs;                                 // Zapamiętaj czas startowy rozpoczęcia blokady programu.
  while ((Zegar_SysTickMs - PoczatekBlokady) < CzasDelayMs)                   // Zatrzymaj program w pętli, aż upłynie zadana ilość milisekund.
  {                                                                           //
    __NOP();                                                                  //   Pal prąd procesora na chama jak bydle. Opcjonalnie WFI do watchdoga.
  }
}