#include "ADC.h"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------- DEFINICJA ZMIENNYCH GLOBALNYCH ---------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
volatile uint16_t ADC1_Wartosc = 0u;                                          // Przechowywanie skwantowanego napięcia odczytanego z ADC.
float ADC1_Napiecie = 0.0f;                                                   // Obliczone napięcie z danych od ADC.
float ADC1_Procent_f = 0.0f;                                                  // Zmiennoprzecinkowe wyliczenie procenta napięcia z obranego zakresu.
uint8_t ADC1_Procent = 0u;                                                    // Zaokrąglone do wartości całkowitej wyliczenie procenta napięcia z obranego zakresu.

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------- ADC1 - POMIAR NAPIĘCIA - 0-3.3V - DMA - TRYB CYKLICZNY ---------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ADC1_Inicjalizacja()
{                                                                             //-----------------------------------------------------
                                                                              //--------------- KONFIGURACJA ZEGARÓW ----------------
                                                                              //-----------------------------------------------------
  RCC -> AHB1ENR |= RCC_AHB1ENR_GPIOAEN;                                      // Ustaw w rejestrze AHB1ENR włączony zegar dla portu GPIOA.
  RCC -> AHB1ENR |= RCC_AHB1ENR_DMA2EN;                                       // Ustaw w rejestrze AHB1ENR włączony zegar dla DMA2.
  RCC -> APB2ENR |= RCC_APB2ENR_ADC1EN;                                       // Ustaw w rejestrze APB2ENR włączony zegar dla ADC1.
  (void)RCC -> AHB1ENR;                                                       // Krótki dummy read (2 cykle).
  (void)RCC -> APB2ENR;                                                       // Krótki dummy read (2 cykle).

                                                                              //-----------------------------------------------------
                                                                              //----------------- KONFIGURACJA DMA2 -----------------
                                                                              //-----------------------------------------------------
  DMA2_Stream0 -> CR &= ~DMA_SxCR_EN;                                         // Ustaw w rejestrze CR wyłączony DMA2 Stream0 (0). Należy wyłączyć i odpiąć DMA w celu jego konfiguracji.
  while (DMA2_Stream0 -> CR & DMA_SxCR_EN){ }                                 // Zaczekaj na wyłączenie DMA2 Stream0.
  DMA2_Stream0 -> PAR  = (uint32_t)&ADC1 -> DR;                               // Ustaw w rejestrze PAR adres peryferium – rejestr danych ADC1 -> DR.
  DMA2_Stream0 -> M0AR = (uint32_t)&ADC1_Wartosc;                             // Ustaw w rejestrze M0AR adres pamięci docelowy - zmienna ADC1_Wartosc.
  DMA2_Stream0 -> NDTR = 1u;                                                  // Ustaw w rejestrze NDTR licznik o wartości rozmiaru tablicy na dane (0001).
  DMA2_Stream0 -> CR  = 0u;                                                   // Wyczyść rejestr CR dla DMA2 Stream0 (00000000000000000000000000000000).
  DMA2_Stream0 -> CR |= (0u << DMA_SxCR_CHSEL_Pos);                           // Ustaw w rejestrze CR wykorzystanie Kanału0 (0000).
  DMA2_Stream0 -> CR |= DMA_SxCR_PL_1;                                        // Ustaw w rejestrze CR priorytet High (10).
  DMA2_Stream0 -> CR |= DMA_SxCR_MSIZE_0;                                     // Ustaw w rejestrze CR rozmiar danej dostarczonej do pamięci na 16 bitów (01).
  DMA2_Stream0 -> CR |= DMA_SxCR_PSIZE_0;                                     // Ustaw w rejestrze CR rozmiar danej przychodzącej z peryferium na 16 bitów (01). 
  DMA2_Stream0 -> CR |= DMA_SxCR_CIRC;                                        // Ustaw w rejestrze CR włączony tryb cykliczny (1).
  DMA2_Stream0 -> CR |= DMA_SxCR_EN;                                          // Ustaw w rejestrze CR włączony DMA2 Stream0 Kanał0 (1).

                                                                              //-----------------------------------------------------
                                                                              //----------------- KONFIGURACJA GPIO -----------------
                                                                              //-----------------------------------------------------
  GPIOA -> MODER |=  GPIO_MODER_MODER0;                                       // Ustaw w rejestrze MODER pin PA0 na analog (11).
  GPIOA -> PUPDR &= ~GPIO_PUPDR_PUPDR0_Msk;                                   // Ustaw w rejestrze PUPDR pin PA0 jako No Pull-Up / No Pull-Down (00).

                                                                              //-----------------------------------------------------
                                                                              //----------------- KONFIGURACJA ADC1 -----------------
                                                                              //-----------------------------------------------------
  ADC123_COMMON -> CCR &= ~ADC_CCR_ADCPRE;                                    // Wyczyść w rejestrze CCR preskaler ADCPRE wspólny dla ADC1/2/3.
  ADC123_COMMON -> CCR |=  ADC_CCR_ADCPRE_0;                                  // Ustaw w rejestrze CCR preskaler ADCPRE (01). PCLK2 / 4 (bezpieczna prędkość dla ADC).
  ADC1 -> CR1 = 0u;                                                           // Wyczyść rejestr CR1 - Rozdzielczość 12 bit, bez trybu SCAN (jeden kanał).
  ADC1 -> CR2 = 0u;                                                           // Wyczyść rejestr CR2 - Wstępnie wyłączone CONT/DMA/EOCS/ADON.
  ADC1 -> CR2 |=  ADC_CR2_CONT;                                               // Ustaw w rejestrze CR2 tryb ciągły (CONT = 1), kolejne konwersje bez ponownego wyzwalania.
  ADC1 -> CR2 |=  ADC_CR2_EOCS;                                               // Ustaw w rejestrze CR2 EOCS = 1 – flaga EOC po każdej konwersji.
  ADC1 -> CR2 |=  ADC_CR2_DMA;                                                // Ustaw w rejestrze CR2 DMA = 1 – korzystanie z DMA.
  ADC1 -> CR2 |=  ADC_CR2_DDS;                                                // Ustaw w rejestrze CR2 DDS = 1 – ciągłe żądania DMA.
  ADC1 -> SMPR2 &= ~ADC_SMPR2_SMP0;                                           // Wyczyść w rejestrze SMPR2 czas próbkowania kanału 0.
  ADC1 -> SMPR2 |=  ADC_SMPR2_SMP0;                                           // Ustaw w rejestrze SMPR2 maksymalny czas próbkowania dla kanału 0 – 480 cykli (stabilny pomiar).
  ADC1 -> SQR1  &= ~ADC_SQR1_L;                                               // Ustaw długość sekwencji L = 0 – jedna konwersja w sekwencji.
  ADC1 -> SQR3  &= ~ADC_SQR3_SQ1;                                             // Ustaw pierwszy kanał w sekwencji SQ1 = 0 – kanał 0 (PA0).
  ADC1 -> CR2 |=  ADC_CR2_ADON;                                               // Ustaw w rejestrze CR2 włączony ADC1 – bit ADON = 1.
  ADC1 -> CR2 |=  ADC_CR2_SWSTART;                                            // Ustaw w rejestrze CR2 start pierwszej konwersji (SWSTART = 1), w trybie CONT ADC pracuje w kółko.
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------- KONWERSJA WYNIKU POMIARU ADC NA NAPIĘCIE ----------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ADC1_OdczytajNapiecie()
{
  ADC1_Napiecie = (ADC1_Wartosc * 3.3f) / 4095.0f;                            // Przelicz wartość z ADC (0..4095) na napięcie przy Vref = 3.3 V.
  ADC1_Procent_f = (ADC1_Napiecie / 3.3f) * 100.0f;                           // Przelicz napięcie na procent zajętości zakresu (0..100%).
  if (ADC1_Procent_f < 0.0f)   ADC1_Procent_f = 0.0f;                         // Ograniczenie dolne – nie mniej niż 0%.
  if (ADC1_Procent_f > 100.0f) ADC1_Procent_f = 100.0f;                       // Ograniczenie górne – nie więcej niż 100%.
  ADC1_Procent = (uint8_t)(ADC1_Procent_f + 0.5f);                            // Zaokrąglij wartość procentową do najbliższej liczby całkowitej.
}