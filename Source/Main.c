#include "Main.h"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------- POCZĄTEK PROGRAMU - FUNKCJA MAIN --------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int main(void)
{
  Zegar_Wewn_Inicjalizacja();                                                 // Ustaw taktowanie zegara mikrokontrolera na 216MHz.
  GPIO_Inicjalizacja();                                                       // Konfiguruj GPIO do obsługi diod LED.
  ADC1_Inicjalizacja();                                                       // Konfiguruj ADC w DMA z pomiarem na pinie PA0.
  I2C1_Inicjalizacja();                                                       // Konfiguruj I2C do wyświetlaczA OLED na pinach PB8 SCL, PB9 SDA.
  OLED_Inicjalizacja();                                                       // Inicjaizuj wyświetlacz OLED.
  OLED_ScopeInit(&Wykres_Napiecia, 2, 20, 124, 44, 0.0f, 3.3f, 2000ul);       // Inicjalizuj wykres napięcia na ekranie OLED.

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------- PĘTLA GŁÓWNA PROGRAMU -------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  while (1)
  {
    ADC1_OdczytajNapiecie();                                                  // Konwertuj wynik pomiaru ADC na napięcie.
    Menu_ZmianaStronyCzas(6000ul);                                            // Przełącz stronę co 6000ms (6 sekund).
    OLED_ScopeAddSample(&Wykres_Napiecia, ADC1_Napiecie, Zegar_SysTickMs);    // Dodaj próbkę napięcia do wykresu czasowego.
    OLED_AnimacjaPrawo();                                                     // Animuj obraz wjeżdżając na ekran z prawej strony.

    switch (Menu_AktualnaStrona)                                              // Kolejka stron do wyświetlenia:
    {                                                                         //
      case 0:  Menu_Strona0(); break;                                         //   Uruchom funkcję wyświetlającą Menu_Strona0.
      case 1:  Menu_Strona1(); break;                                         //   Uruchom funkcję wyświetlającą Menu_Strona1.
      case 2:  Menu_Strona2(); break;                                         //   Uruchom funkcję wyświetlającą Menu_Strona2.
      case 3:  Menu_Strona3(); break;                                         //   Uruchom funkcję wyświetlającą Menu_Strona3.
      case 4:  Menu_Strona4(); break;                                         //   Uruchom funkcję wyświetlającą Menu_Strona4.
      case 5:  Menu_Strona5(); break;                                         //   Uruchom funkcję wyświetlającą Menu_Strona5.
      case 6:  Menu_Strona6(); break;                                         //   Uruchom funkcję wyświetlającą Menu_Strona6.
      case 7:  Menu_Strona7(); break;                                         //   Uruchom funkcję wyświetlającą Menu_Strona7.
      default: Menu_Strona0(); break;                                         //   Uruchom funkcję wyświetlającą Menu_Strona0.
    }
    Menu_UstawDiode((LED_t)(Menu_AktualnaStrona % 3u));
  }
}