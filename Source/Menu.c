#include "Menu.h"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------- DEFINICJA ZMIENNYCH GLOBALNYCH ---------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint8_t  Menu_AktualnaStrona = 0u;                                            // Przechowywanie aktualnie wyświetlanej strony na wyświetlaczu OLED.

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------- DEFINICJA ZMIENNYCH STATYCZNYCH --------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static uint32_t Menu_OstatniePrzejscieMs = 0ul;                               // Czas ostatniej zmiany strony na wyświetlaczu OLED.
static float Predkosc_KatY = 0.0f;                                            // Dla obieku 3D.
static float Predkosc_KatX = 0.0f;                                            // Dla obieku 3D.

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------- STRONA 0 WYŚWIETLACZA -------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Menu_Strona0(void)
{
  OLED_BufferClear();                                                         // Wyczyść cały bufor ekranu w RAM.
  OLED_SetFont(&Font_BankGothic_Md_BT_13px);                                  // Zmiana czcionki.
  OLED_DrawString(P, 14, "ADC W DMA", "CX");                                  // Wycentrowany w osi X tekst.
  OLED_DrawString(P, 27, "+", "CX");                                          // Wycentrowany w osi X tekst.
  OLED_DrawString(P, 41, "OLED I2C", "CX");                                   // Wycentrowany w osi X tekst.
  OLED_SetFont(&Font_5x7);                                                    // Zmiana czcionki.
  OLED_UpdateScreen();                                                        // Prześlij bufor RAM na wyświetlacz OLED.
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------- STRONA 1 WYŚWIETLACZA -------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Menu_Strona1(void)
{
  OLED_BufferClear();                                                         // Wyczyść cały bufor ekranu w RAM.
  OLED_Rect(0+P, 0, 128, 64, 1);                                              // Ramka dookoła całego obszaru 128x64.
  OLED_DrawString(5+P, 5, "STM32F767ZIT6", NULL);                             // Tytuł - nazwa mikrokontrolera.
  OLED_DrawString(5+P, 15, "ADC W DMA", NULL);                                // Podtytuł - używany jest ADC z DMA.
  OLED_DrawString(5+P, 25, "NA REJESTRACH", NULL);                            // Opis – konfiguracja na rejestrach.
  OLED_SetFont(&Font_8x8);                                                    // Zmiana czcionki.
  OLED_PrintValue(5+P, 37, ADC1_Napiecie, "V");                               // Wyświetl zmierzone napięcie przez ADC.
  OLED_PrintValue(5+P, 49, ADC1_Procent, "%");                                // Wyświetl wartość procentową - 3.3V - 100%.
  OLED_SetFont(&Font_5x7);                                                    // Zmiana czcionki.
  OLED_FillRect(90+P, 5, 30, 20, ADC1_Procent, 'B', 1);                       // Wypełniany prostokąt (pasek postępu) od lewej, proporcjonalnie do procent.
  OLED_FillCirclePie(106+P, 45, 10, ADC1_Procent, 90, 'J', 1);                // „Zegarowy” wycinek koła (pie chart) – procent wypełnienia od 90° zgodnie z ruchem wskazówek.
  OLED_FillTriangle(80+P, 55, 80+P, 40, 50+P, 55, ADC1_Procent, 'L', 1);      // Wypełnij trójkąt (np. wskaźnik) z clippingiem wg Procent od lewej.
  OLED_Rect(90+P, 5, 30, 20, 1);                                              // Obrys prostokąta (ramka wokół paska postępu).
  OLED_Circle(106+P, 45, 10, 1);                                              // Obrys koła (ramka wokół „pie chart”).
  OLED_Triangle(80+P, 55, 80+P, 40, 50+P, 55, 1);                             // Obrys trójkąta (kontur wskaźnika).
  OLED_UpdateScreen();                                                        // Prześlij bufor RAM na wyświetlacz OLED.
}   

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------- STRONA 2 WYŚWIETLACZA -------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Menu_Strona2(void)
{
  OLED_BufferClear();                                                         // Wyczyść cały bufor ekranu w RAM.
  OLED_Symbol(2+P, 0, &Symbol_Termo_Pusty, NULL);                             // Rysuj na ekranie OLED symbol termometru.
  OLED_FillRect(9+P, 3, 3, 21, ADC1_Procent, 'B', 1);                         // Rysuj na ekranie OLED proporcjonalnie do procent wypełniony prostokąt. Wypełnij od dołu.
  OLED_FillCirclePie(80+P, 30, 20, ADC1_Procent, 90, 'C', 1);                 // Rysuj na ekranie OLED proporcjonalnie do procent wypełniony wycinek koła. 
  OLED_Circle(80+P, 30, 20, 1);                                               // Rysuj na ekranie OLED obrys koła.
  OLED_UpdateScreen();                                                        // Prześlij bufor RAM na wyświetlacz OLED.
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------- STRONA 3 WYŚWIETLACZA -------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Menu_Strona3(void)
{
  OLED_BufferClear();                                                         // Wyczyść cały bufor ekranu w RAM.
  OLED_Symbol(3+P, 4, &Symbol_MTOPrograms, "CX");                             // Wycentrowany w osi X symbol.
  OLED_SetFont(&Font_BankGothic_Md_BT_13px);                                  // Zmiana czcionki.
  OLED_DrawString(P, 1, "MTOPrograms", "C7");                                 // Wycentrowany w osi X tekst. W Y strona 7.
  OLED_SetFont(&Font_5x7);                                                    // Powrót do mniejszej czcionki 5x7 dla pozostałych elementów.
  OLED_UpdateScreen();                                                        // Prześlij bufor RAM na wyświetlacz OLED.
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------- STRONA 4 WYŚWIETLACZA -------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Menu_Strona4(void)
{
  OLED_BufferClear();                                                         // Wyczyść cały bufor ekranu w RAM.
  OLED_Symbol(5+P, 1, &Symbol_Instagram, NULL);                               // Rysuj na ekranie OLED symbol Instagram.
  OLED_Symbol(35+P, 1, &Symbol_Facebook, NULL);                               // Rysuj na ekranie OLED symbol Facebook.
  OLED_Symbol(65+P, 1, &Symbol_VSCode, NULL);                                 // Rysuj na ekranie OLED symbol VSCode.
  OLED_Symbol(5+P, 33, &Discord_Symbol, NULL);                                // Rysuj na ekranie OLED symbol Discord.
  OLED_Symbol(35+P, 33, &Symbol_YouTube, NULL);                               // Rysuj na ekranie OLED symbol YouTube.
  OLED_Symbol(76+P, 33, &Symbol_GPT, NULL);                                   // Rysuj na ekranie OLED symbol GPT.
  OLED_UpdateScreen();                                                        // Prześlij bufor RAM na wyświetlacz OLED.
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------- STRONA 5 WYŚWIETLACZA -------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Menu_Strona5(void)
{
  OLED_BufferClear();                                                         // Wyczyść cały bufor ekranu w RAM.
  OLED_Symbol(P, 0, &Symbol_ST, NULL);                                        // Rysuj na ekranie OLED symbol ST.
  OLED_UpdateScreen();                                                        // Prześlij bufor RAM na wyświetlacz OLED.
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------- STRONA 6 WYŚWIETLACZA -------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Menu_Strona6(void)
{
  Predkosc_KatY += 0.07f;                                                     // Prędkość obrotu wokół Y.
  Predkosc_KatX += 0.07f;                                                     // Prędkość obrotu wokół X.
  if (Predkosc_KatY >= TWO_PI) Predkosc_KatY -= TWO_PI;                       // Jeśli kąt Y ≥ 2π – odejmij 2π, aby zawinąć do zakresu [0, 2π).
  if (Predkosc_KatX >= TWO_PI) Predkosc_KatX -= TWO_PI;                       // Jeśli kąt X ≥ 2π – odejmij 2π, aby zawinąć do zakresu [0, 2π).
  OLED_BufferClear();                                                         // Wyczyść cały bufor ekranu w RAM.
  OLED_Cube3D(Predkosc_KatY, Predkosc_KatX, 0, 0-P1, 65, "CXY");              // Wycentrowany w osi X i Y obiekt 3D.
  OLED_SetFont(&Font_BankGothic_Md_BT_13px);                                  // Zmiana czcionki.
  OLED_DrawString(0+P, 50, "Obiekt 3D", "CX");                                // Wycentrowany w osi X opis obiektu 3D.
  OLED_UpdateScreen();                                                        // Prześlij bufor RAM na wyświetlacz OLED.
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------- STRONA 7 WYŚWIETLACZA -------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Menu_Strona7(void)
{
  OLED_BufferClear();                                                         // Wyczyść cały bufor ekranu w RAM.
  OLED_SetFont(&Font_BankGothic_Md_BT_13px);                                  // Zmiana czcionki.
  OLED_DrawString(P, 0, "Wykres napiecia", "CX");                             // Wycentrowany w osi X tekst.
  OLED_ScopeDraw(&Wykres_Napiecia, 2+P, 20, 124, 44);                         // x, y, w, h.
  OLED_UpdateScreen();                                                        // Prześlij bufor RAM na wyświetlacz OLED.
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------- PRZEŁĄCZAJ STRONY WYŚWIETLACZA ZALEŻNIE OD CZASU ---------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Menu_ZmianaStronyCzas(uint32_t Czas)
{
  uint32_t Menu_AktualnyCzasMs = Zegar_SysTickMs;                             // Pobierz aktualny czas z timera SysTick (inkrementacja w przerwaniu o 1ms).
  if ((Menu_AktualnyCzasMs - Menu_OstatniePrzejscieMs) >= Czas)               // Jak minął czas:
  {                                                                           //
    Menu_OstatniePrzejscieMs = Menu_AktualnyCzasMs;                           //   Zapisz czas zmiany.
    Menu_AktualnaStrona++;                                                    //   Przejdź do następnej strony.
    if (Menu_AktualnaStrona >= 8)                                             //   Jak strona większa niż 8:
    {                                                                         //
      Menu_AktualnaStrona = 0;                                                //   Zacznij od Strona0.
    }                                                                         //
    P = 128u;                                                                 //   Reset wjazdu na wyświetlacz od lewej strony.
    P1 = 64u;                                                                 //   Reset wjazdu na wyświetlacz od góry.
  }
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------- WŁĄCZ WSKAZANĄ DIODĘ LED ------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Menu_UstawDiode(LED_t LED)
{
  LED_ZIELONA_OFF(); LED_NIEBIESKA_OFF(); LED_CZERWONA_OFF();
  switch (LED)
  {
    case LED_ZIELONA:   LED_ZIELONA_ON();   break;
    case LED_NIEBIESKA: LED_NIEBIESKA_ON(); break;
    case LED_CZERWONA:  LED_CZERWONA_ON();  break;
    default: break;
  }
}