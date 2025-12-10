#include "OLED.h"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------- DEFINICJA ZMIENNYCH GLOBALNYCH ---------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint8_t P = 128u;
uint8_t P1 = 64u;


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------- DEFINICJA ZMIENNYCH STATYCZNYCH --------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static uint8_t OLED_Buffer[OLED_WIDTH * OLED_PAGES];                          // Bufor ekranu (1 bajt = 8 pikseli w pionie, cała pamięć RAM wyświetlacza)
static const OLED_Font_t *OLED_CurrentFont = &Font_5x7;                       // Aktualnie używana czcionka – domyślnie 5x7


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------- DEKLARACJA FUNKCJI STATYCZNYCH ---------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static int  OLED_I2C_Write(uint8_t addr, const uint8_t *data, uint8_t len);   // Prototyp funkcji niskopoziomowego zapisu po I2C.
static void OLED_WriteCommand(uint8_t cmd);                                   // Prototyp funkcji wysyłającej pojedynczą komendę do SSD1306.
static void OLED_WriteDataByte(uint8_t data);                                 // Prototyp funkcji wysyłającej pojedynczy bajt danych do SSD1306.
static void OLED_ClipLine(int xa, int xb, int y, int xmi, int xma, uint8_t c);// Prototyp funkcji rysującej linię przyciętą do zakresu X (xmi–xma).
static uint16_t OLED_TextWidth(const OLED_Font_t *fnt, const char *txt);      // Prototyp funkcji liczącej szerokość tekstu w pikselach dla czcionki.


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------ OBIEKT WYKRESU NAPIĘCIA NA OLED ----------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
OLED_Scope_t Wykres_Napiecia =
{
  .x              = 0,                                                        // Pozycja X lewego górnego rogu obszaru wykresu (w pikselach).
  .y              = 0,                                                        // Pozycja Y lewego górnego rogu obszaru wykresu (w pikselach).
  .w              = 128,                                                      // Szerokość obszaru wykresu w pikselach.
  .h              = 32,                                                       // Wysokość obszaru wykresu w pikselach.
  .min_val        = 0.0f,                                                     // Minimalna wartość osi Y (np. 0.0 V – dół wykresu).
  .max_val        = 5.0f,                                                     // Maksymalna wartość osi Y (np. 5.0 V – góra wykresu).
  .time_window_ms = 2000,                                                     // Szerokość okna czasowego w ms – ile historii mieści się jednocześnie na wykresie.
  .last_pixel_ms  = 0,                                                        // Znacznik czasu (ms) ostatnio narysowanej kolumny – uzupełniany w czasie pracy.
  .pixel_dt_ms    = 20,                                                       // Odstęp czasu między kolejnymi kolumnami wykresu (ms); 2000/20 ≈ 100 kolumn/okno.
  .write_idx      = 0,                                                        // Indeks w buforze próbek, pod który zostanie wpisana następna próbka napięcia.
  .samples        = {0}                                                       // Bufor próbek napięcia do rysowania przebiegu – startowo wyzerowany.
};


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------- FUNKCJE ZAPISU DO I2C -------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static int OLED_I2C_Write(uint8_t addr, const uint8_t *data, uint8_t len)     //---------------------------------------------------------------
{                                                                             //--- WYŚLIJ CIĄG BAJTÓW "data[0..len-1]" PRZEZ I2C DO EKRANU ---
                                                                              //----- Zwraca: 0 -> OK, -1 -> BŁĄD (NACK / BERR / TIMEOUT) -----
                                                                              //---------------------------------------------------------------
  if (OLED_I2C -> ISR & I2C_ISR_STOPF)  OLED_I2C -> ICR |= I2C_ICR_STOPCF;    // Upewnij się, że poprzedni STOP wyczyszczony.
  if (OLED_I2C -> ISR & I2C_ISR_NACKF)  OLED_I2C -> ICR |= I2C_ICR_NACKCF;    // Upewnij się, że poprzedni NACK wyczyszczony.
  if (OLED_I2C -> ISR & I2C_ISR_BERR)   OLED_I2C -> ICR |= I2C_ICR_BERRCF;    // Upewnij się, że poprzedni BERR wyczyszczony.
  if (OLED_I2C -> ISR & I2C_ISR_ARLO)   OLED_I2C -> ICR |= I2C_ICR_ARLOCF;    // Upewnij się, że poprzedni ARLO wyczyszczony.
  OLED_I2C -> CR2 = 0;                                                        // Wyczyść rejestr CR2 (konfiguracja nowej transakcji).
  OLED_I2C -> CR2 |= (uint32_t)(addr << 1);                                   // Wpisz adres slave'a (7-bit) przesunięty o 1 pod bit R/W.
  OLED_I2C -> CR2 &= ~I2C_CR2_RD_WRN;                                         // Ustaw kierunek: zapis (0 = write).
  OLED_I2C -> CR2 &= ~I2C_CR2_ADD10;                                          // Używamy adresowania 7-bitowego (nie 10-bit).
  OLED_I2C -> CR2 |= ((uint32_t)len << I2C_CR2_NBYTES_Pos);                   // Ustaw liczbę bajtów do wysłania.
  OLED_I2C -> CR2 |= I2C_CR2_AUTOEND;                                         // Po wysłaniu ostatniego bajtu wygeneruj automatyczny STOP.
  OLED_I2C -> CR2 |= I2C_CR2_START;                                           // Wygeneruj START – rozpoczęcie transakcji na magistrali.
  for (uint8_t i = 0; i < len; i++)                                           // Pętla po wszystkich bajtach do wysłania.
  {                                                                           //
    uint32_t timeout = 100000;                                                // Licznik timeoutu na zdarzenie w I2C.
    while (!(OLED_I2C->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF                    // Czekaj aż TXIS (gotowy na bajt) lub wystąpi błąd/NACK/BERR/ARLO.
                            | I2C_ISR_BERR | I2C_ISR_ARLO))){                 //
      if (--timeout == 0){return -1;}                                         // Jeśli przekroczono timeout – zwróć błąd -1.
    }                                                                         //
    if (OLED_I2C -> ISR & (I2C_ISR_NACKF | I2C_ISR_BERR | I2C_ISR_ARLO))      // Sprawdź, czy pojawił się NACK albo błąd magistrali.
    {                                                                         //
      OLED_I2C -> ICR |= I2C_ICR_NACKCF | I2C_ICR_BERRCF | I2C_ICR_ARLOCF;    // Wyczyść flagi błędów w rejestrze ICR.
      return -1;                                                              // Przerwij funkcję i zgłoś błąd.
    }                                                                         //
    OLED_I2C -> TXDR = data[i];                                               // Zapisz kolejny bajt do rejestru nadawczego TXDR.
  }                                                                           //
  while (!(OLED_I2C -> ISR & I2C_ISR_STOPF)) { }                              // Czekaj, aż kontroler wygeneruje STOP (koniec transakcji).
  OLED_I2C -> ICR |= I2C_ICR_STOPCF;                                          // Wyczyść flagę STOPF.
  return 0;                                                                   // Zwróć 0 – transmisja zakończona poprawnie.OLED_Inicjalizacja
}

static void OLED_WriteCommand(uint8_t cmd)                                    //---------------------------------------------------------------
{                                                                             //-------------- WYŚLIJ POJEDYNCZĄ KOMENDĘ DO OLED --------------
                                                                              //---------------------------------------------------------------
  uint8_t buf[2];                                                             // Bufor 2 bajtów: [0] = sterujący, [1] = komenda.
  buf[0] = OLED_CTRL_CMD;                                                     // Bajt sterujący: informuje OLED, że to komenda.
  buf[1] = cmd;                                                               // Właściwa wartość komendy.
  (void)OLED_I2C_Write(OLED_I2C_ADDR, buf, 2);                                // Wyślij 2 bajty przez I2C do wyświetlacza.
}

static void OLED_WriteDataByte(uint8_t data)                                  //---------------------------------------------------------------
{                                                                             //------------ WYŚLIJ POJEDYNCZY BAJT DANYCH DO OLED ------------
                                                                              //---------------------------------------------------------------
  uint8_t buf[2];                                                             // Bufor 2 bajtów: [0] = sterujący, [1] = dane.
  buf[0] = OLED_CTRL_DATA;                                                    // Bajt sterujący: informuje OLED, że to dane (nie komenda).
  buf[1] = data;                                                              // Wartość bajtu danych (piksele / mapa pamięci).
  (void)OLED_I2C_Write(OLED_I2C_ADDR, buf, 2);                                // Wyślij dane do wyświetlacza.
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------- FUNKCJE STEROWANIA EKRANEM -----------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void OLED_Inicjalizacja(void)                                                 //---------------------------------------------------------------
{                                                                             //-------- INICJALIZUJ KONTROLER OLED I USTAW PARAMETRY ---------
                                                                              //---------------------------------------------------------------
  OLED_WriteCommand(0xAE);                                                    // DISPLAYOFF – wyłącz wyświetlacz na czas konfiguracji.
  OLED_WriteCommand(0x20);                                                    // MEMORYMODE – wybór trybu adresowania pamięci.
  OLED_WriteCommand(0x00);                                                    // Tryb adresowania horyzontalny.
  OLED_WriteCommand(0xB0);                                                    // Page 0 (startowa strona po restarcie).
  OLED_WriteCommand(0xC8);                                                    // Odwrócony kierunek skanowania COM (remap).
  OLED_WriteCommand(0x00);                                                    // Początkowa kolumna (low nibble = 0).
  OLED_WriteCommand(0x10);                                                    // Początkowa kolumna (high nibble = 0).
  OLED_WriteCommand(0x40);                                                    // Start line = 0 (pierwsza linia wyświetlana na górze).
  OLED_WriteCommand(0x81);                                                    // SETCONTRAST – ustawienie kontrastu.
  OLED_WriteCommand(0x7F);                                                    // Wartość kontrastu ~połowa (0x00–0xFF).
  OLED_WriteCommand(0xA1);                                                    // SEGREMAP – mapowanie segmentów (mirror w poziomie).
  OLED_WriteCommand(0xA6);                                                    // Normal display (0 = piksel zgaszony, 1 = świeci).
  OLED_WriteCommand(0xA8);                                                    // SETMULTIPLEX – współczynnik multipleksowania.
  OLED_WriteCommand(0x3F);                                                    // 1/64 duty (dla 128x64).
  OLED_WriteCommand(0xA4);                                                    // Display follows RAM (nie wymuszaj pełnego ON).
  OLED_WriteCommand(0xD3);                                                    // SETDISPLAYOFFSET – przesunięcie w pionie.
  OLED_WriteCommand(0x00);                                                    // Brak przesunięcia (offset = 0).
  OLED_WriteCommand(0xD5);                                                    // SETDISPLAYCLOCKDIV – ustawienie zegara wyświetlacza.
  OLED_WriteCommand(0x80);                                                    // Domyślne, zalecane wartości (oscillator + divide ratio).
  OLED_WriteCommand(0xD9);                                                    // SETPRECHARGE – czas precharge.
  OLED_WriteCommand(0xF1);                                                    // Ustawienie precharge (wartość typowa dla wewn. pompy).
  OLED_WriteCommand(0xDA);                                                    // SETCOMPINS – konfiguracja pinów COM.
  OLED_WriteCommand(0x12);                                                    // Alternatywne piny COM, wyłączony remap L/R.
  OLED_WriteCommand(0xDB);                                                    // SETVCOMDETECT – poziom referencji VCOM.
  OLED_WriteCommand(0x40);                                                    // Typowa wartość (ok. 0.77*Vcc).
  OLED_WriteCommand(0x8D);                                                    // CHARGEPUMP – konfiguracja wewnętrznej pompy ładunku.
  OLED_WriteCommand(0x14);                                                    // Włącz wewnętrzną pompę (potrzebna przy zasilaniu z 3.3V).
  OLED_WriteCommand(0xAF);                                                    // DISPLAYON – włącz wyświetlacz.
  OLED_SetFont(&Font_5x7);                                                    // Ustaw czcionkę wyświetlacza OLED.
  OLED_Clear();                                                               // Wyczyść zawartość ekranu (wysyłając zera).
}

void OLED_BufferClear(void)                                                   //---------------------------------------------------------------
{                                                                             //----------------- WYCZYŚĆ BUFOR EKRANU W RAM ------------------
                                                                              //---------------------------------------------------------------
  for (uint16_t i = 0; i < OLED_WIDTH * OLED_PAGES; i++)                      // Pętla po wszystkich bajtach bufora:
  {                                                                           //
    OLED_Buffer[i] = 0x00;                                                    //   Ustaw każdy bajt na 0x00 (wszystkie piksele zgaszone).
  }                                                                           //
}

void OLED_Clear(void)                                                         //---------------------------------------------------------------
{                                                                             //-------- WYCZYŚĆ EKRAN (BUFOR + FIZYCZNY WYŚWIETLACZ) ---------
                                                                              //---------------------------------------------------------------
  OLED_BufferClear();                                                         // Wyczyść bufor RAM (logiczny obraz).
  OLED_UpdateScreen();                                                        // Wyślij cały bufor na wyświetlacz.
}

void OLED_ClearPage(uint8_t page)                                             //---------------------------------------------------------------
{                                                                             //-------------------- WYCZYŚĆ STRONĘ EKRANU --------------------
                                                                              //---------------------------------------------------------------
  if (page >= OLED_PAGES) return;                                             // Jeśli numer strony poza zakresem – wyjdź.
                                                                              //
  uint16_t offset = page * OLED_WIDTH;                                        // Początek strony w buforze.
  for (uint16_t i = 0; i < OLED_WIDTH; i++)                                   // Wyczyść tylko tę stronę w buforze:
  {                                                                           //
    OLED_Buffer[offset + i] = 0x00;                                           //   Zgaś wszystkie piksele w tej stronie:
  }                                                                           //
                                                                              //
  OLED_WriteCommand(0xB0 | page);                                             // Ustaw numer strony (0xB0–0xB7).
  OLED_WriteCommand(0x00);                                                    // Ustaw dolny nibble adresu kolumny (LOW).
  OLED_WriteCommand(0x10);                                                    // Ustaw górny nibble adresu kolumny (HIGH).
                                                                              //
  for (uint8_t col = 0; col < OLED_WIDTH; col++)                              // Iteruj po wszystkich kolumnach w danej stronie:
  {                                                                           //
    OLED_WriteDataByte(0x00);                                                 //   Zapisz 0x00 – cała kolumna w tej stronie wygaszona.
  }                                                                           //
}

void OLED_UpdateScreen(void)                                                  //---------------------------------------------------------------
{                                                                             //------------ WYŚLIJ CAŁY BUFOR RAM NA WYŚWIETLACZ -------------
                                                                              //---------------------------------------------------------------
  for (uint8_t page = 0; page < OLED_PAGES; page++)                           // Dla każdej strony pamięci (strona = 8 pionowych pikseli).
  {                                                                           //
    OLED_WriteCommand(0xB0 | page);                                           //   Ustaw bieżącą stronę.
    OLED_WriteCommand(0x00);                                                  //   Ustaw dolny adres kolumny na 0.
    OLED_WriteCommand(0x10);                                                  //   Ustaw górny adres kolumny na 0.
                                                                              //
    uint8_t tmp[1 + OLED_WIDTH];                                              //   Bufor: [0] = bajt sterujący, [1..] = dane kolumn.
    tmp[0] = OLED_CTRL_DATA;                                                  //   Bajt sterujący: tryb danych.
                                                                              //
    for (uint8_t x = 0; x < OLED_WIDTH; x++)                                  //   Skopiuj dane strony do bufora tymczasowego:
    {                                                                         //
      tmp[1 + x] = OLED_Buffer[page * OLED_WIDTH + x];                        //     Każda kolumna z bufora RAM do tmp.
    }                                                                         //
                                                                              //
    OLED_I2C_Write(OLED_I2C_ADDR, tmp, 1 + OLED_WIDTH);                       //   Wyślij całą linię (1 bajt sterujący + 128 bajtów danych).
  }                                                                           //
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------- RYSOWANIE PIKSELI NA EKRANIE ----------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t c)                          //---------------------------------------------------------------
{                                                                             //---------- USTAW / ZGAŚ POJEDYNCZY PIKSEL W BUFORZE -----------
                                                                              //---------------------------------------------------------------
  if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;                            // Jeśli współrzędne są poza ekranem – nic nie rób.
                                                                              //
  uint16_t index = (y / 8) * OLED_WIDTH + x;                                  // Oblicz indeks bajtu w buforze (strona * szerokość + kolumna).
  uint8_t  bit   = (1 << (y % 8));                                            // Wylicz bit wewnątrz bajtu odpowiadający danemu pikselowi.
                                                                              //
  if (c)                                                                      // Jeśli kolor ≠ 0 -> włącz piksel:
  {                                                                           //
    OLED_Buffer[index] |=  bit;                                               //   Ustaw odpowiedni bit (OR z maską).
  }                                                                           //
  else                                                                        // W przeciwnym razie (color == 0) -> zgaś piksel:
  {                                                                           //
    OLED_Buffer[index] &= ~bit;                                               //   Wyzeruj odpowiedni bit (AND z negacją maski).
  }
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------- RYSOWANIE LINII NA EKRANIE -----------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void OLED_ClipLine(int a, int b, int y, int xmi, int xma, uint8_t c)   //---------------------------------------------------------------
{                                                                             //------ RYSUJ POZIOMY ODCINEK Z UWZGLĘDNIENIEM CLIPPINGU -------
                                                                              //---------------------------------------------------------------
  if (y < 0 || y >= OLED_HEIGHT) return;                                      // Jeśli y poza wysokością ekranu – nic nie rysuj.
                                                                              //
  if (a > b)                                                                  // Jeśli początek > koniec – zamień miejscami.
  {                                                                           //
    int t = a;                                                                //   Zapisz tymczasową wartość.
    a = b;                                                                    //   Zamień xa z xb.
    b = t;                                                                    //   Zakończ zamianę.
  }                                                                           //
                                                                              //
  if (b < xmi || a > xma) return;                                             // Jeśli cały odcinek poza oknem clipu – pomiń.
                                                                              //
  if (a < xmi) a = xmi;                                                       // Ogranicz początek do lewej krawędzi clipu.
  if (b > xma) b = xma;                                                       // Ogranicz koniec do prawej krawędzi clipu.
                                                                              //
  if (a < 0) a = 0;                                                           // Ogranicz do lewej krawędzi ekranu.
  if (b >= OLED_WIDTH) b = OLED_WIDTH - 1;                                    // Ogranicz do prawej krawędzi ekranu.
  if (a > b) return;                                                          // Jeśli po przycięciach nic nie pozostało – wyjdź.
                                                                              //
  for (int x = a; x <= b; x++)                                                // Pętla po wszystkich pikselach w poziomie.
  {                                                                           //
    OLED_DrawPixel((uint8_t)x, (uint8_t)y, c);                                //   Rysuj pojedynczy piksel w danym kolorze.
  }
}

void OLED_HLine(uint8_t x0, uint8_t x1, uint8_t y, uint8_t c)                 //---------------------------------------------------------------
{                                                                             //--------------------- RYSUJ LINIĘ POZIOMĄ ---------------------
                                                                              //---------------------------------------------------------------
  if (y >= OLED_HEIGHT) return;                                               // Jeśli y poza ekranem – nic nie rysuj.
  if (x1 < x0) {uint8_t t = x0; x0 = x1; x1 = t;}                             // Jeśli początek > koniec – zamień x0 i x1.
                                                                              //
  for (uint8_t x = x0; x <= x1 && x < OLED_WIDTH; x++)                        // Iteruj od x0 do x1, pilnując szerokości ekranu.
  {                                                                           //
    OLED_DrawPixel(x, y, c);                                                  //   Rysuj poziomą linię piksel po pikselu.
  }
}

void OLED_VLine(uint8_t x, uint8_t y0, uint8_t y1, uint8_t c)                 //---------------------------------------------------------------
{                                                                             //--------------------- RYSUJ LINIĘ PIONOWĄ ---------------------
                                                                              //---------------------------------------------------------------
  if (x >= OLED_WIDTH) return;                                                // Jeśli x poza ekranem – nic nie rysuj.
  if (y1 < y0) { uint8_t t = y0; y0 = y1; y1 = t; }                           // Zamień y0 i y1 jeśli odwrócone.
                                                                              //
  for (uint8_t y = y0; y <= y1 && y < OLED_HEIGHT; y++)                       // Iteruj od y0 do y1, pilnując wysokości ekranu.
  {                                                                           //
    OLED_DrawPixel(x, y, c);                                                  //   Rysuj pionową linię piksel po pikselu.
  }
}

void OLED_Line(int x0, int y0, int x1, int y1, uint8_t c)                     //---------------------------------------------------------------
{                                                                             //--------------- RYSUJ LINIĘ DOWOLNĄ (BRESENHAM) ---------------
                                                                              //---------------------------------------------------------------
  int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);                                 // |x1 - x0| – różnica po X.
  int sx = (x0 < x1) ? 1 : -1;                                                // Kierunek kroku po X (1 w prawo, -1 w lewo).
  int dy = (y1 > y0) ? (y0 - y1) : (y1 - y0);                                 // Uwaga: dy jest ujemne (algorytm Bresenhama).
  int sy = (y0 < y1) ? 1 : -1;                                                // Kierunek kroku po Y (1 w dół, -1 w górę).
  int err = dx + dy;                                                          // Wartość błędu początkowego.
                                                                              //
  while (1)                                                                   // Pętla rysowania linii (Bresenham).
  {                                                                           //
    OLED_DrawPixel((uint8_t)x0, (uint8_t)y0, c);                              //   Narysuj piksel w aktualnej pozycji.
                                                                              //
    if (x0 == x1 && y0 == y1) break;                                          //   Jeśli doszliśmy do końca – przerwij pętlę.
                                                                              //
    int e2 = 2 * err;                                                         //   Podwojony błąd dla porównania.
                                                                              //
    if (e2 >= dy) { err += dy; x0 += sx; }                                    //   Korekta po X, jeśli trzeba.
    if (e2 <= dx) { err += dx; y0 += sy; }                                    //   Korekta po Y, jeśli trzeba.
  }
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------- RYSOWANIE PROSTOKĄTA NA EKRANIE --------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void OLED_Rect(int x, int y, int w, int h, uint8_t c)                         //---------------------------------------------------------------
{                                                                             //------------------- RYSUJ PROSTOKĄT (RAMKA) -------------------
                                                                              //---------------------------------------------------------------
  int x2 = x + w - 1;                                                         // Prawa krawędź prostokąta (x + szerokość - 1).
  int y2 = y + h - 1;                                                         // Dolna krawędź prostokąta (y + wysokość - 1).

  OLED_Line(x,  y,  x2, y,  c);                                               // Górna krawędź prostokąta (linia pozioma).
  OLED_Line(x,  y2, x2, y2, c);                                               // Dolna krawędź prostokąta.
  OLED_Line(x,  y,  x,  y2, c);                                               // Lewa krawędź prostokąta (linia pionowa).
  OLED_Line(x2, y,  x2, y2, c);                                               // Prawa krawędź prostokąta.
}

void OLED_FillRect(int x, int y, int w, int h, uint8_t p, char d, uint8_t c)  //---------------------------------------------------------------
{                                                                             //----------- RYSUJ Wypełniony PROSTOKĄT (BEZ OBRYSU) -----------
                                                                              //---------------------------------------------------------------
  if (w <= 0 || h <= 0) return;                                               // Jeśli wymiary ≤ 0 – nie rysuj, zakończ funkcję.
  if (p > 100) p = 100;                                                       // Zabezpieczenie – ogranicz procent do 100%.
  if (p == 0) return;                                                         // 0% – nie rysuj, zakończ funkcję.

                                                                              //---------------------------------------------------------------
                                                                              //----------- POZIOMO – WYPEŁNIENIE PO X, LEWO/PRAWO ------------
                                                                              //---------------------------------------------------------------
  if (d == 'L' || d == 'l' || d == 'R' || d == 'r')                           // Kierunek poziomy – procent liczony po osi X.
  {                                                                           //
    int x_min   = x;                                                          //   Początek prostokąta po X.
    int x_max   = x + w - 1;                                                  //   Koniec prostokąta po X.
    int total_w = x_max - x_min + 1;                                          //   Całkowita szerokość prostokąta.
                                                                              //
    int fill_w = (total_w * (int)p) / 100;                                    //   Szerokość faktycznie wypełniona.
    if (fill_w <= 0) return;                                                  //   Jeśli po obliczeniu 0 – nie rysuj, zakończ funkcję.
                                                                              //
    int x_draw_min;                                                           //   Zakres X, który będzie wypełniony – początek.
    int x_draw_max;                                                           //   Zakres X, który będzie wypełniony – koniec.
                                                                              //
    if (d == 'L' || d == 'l')                                                 //   Wypełnianie od LEWEJ strony.
    {                                                                         // 
      x_draw_min = x_min;                                                     //   Start z lewej krawędzi prostokąta.
      x_draw_max = x_min + fill_w - 1;                                        //   Koniec wg procentu.
    }                                                                         //
    else                                                                      //   'R' / 'r' – wypełnianie od PRAWEJ strony.
    {                                                                         //
      x_draw_max = x_max;                                                     //   Start z prawej krawędzi prostokąta.
      x_draw_min = x_max - fill_w + 1;                                        //   Początek wg procentu.
    }                                                                         //
                                                                              //
    if (x_draw_min < 0)               x_draw_min = 0;                         //   Przytnij do ekranu po lewej.
    if (x_draw_max >= OLED_WIDTH)     x_draw_max = OLED_WIDTH - 1;            //   Przytnij do ekranu po prawej.
                                                                              //
    for (int yy = y; yy < y + h; yy++)                                        //   Iteracja po wierszach prostokąta (po osi Y).
    {                                                                         //
      if (yy < 0 || yy >= OLED_HEIGHT) continue;                              //   Jeśli wiersz poza ekranem – pomiń.
                                                                              //
      int xa = x_draw_min;                                                    //   Początek wiersza do rysowania po X.
      int xb = x_draw_max;                                                    //   Koniec wiersza do rysowania po X.
                                                                              //
      if (xa < 0)           xa = 0;                                           //   Dodatkowe zabezpieczenie po lewej.
      if (xb >= OLED_WIDTH) xb = OLED_WIDTH - 1;                              //   Dodatkowe zabezpieczenie po prawej.
      if (xa > xb)          continue;                                         //   Jeśli nic nie zostaje – przejdź do kolejnego wiersza.
                                                                              //
      for (int xx = xa; xx <= xb; xx++)                                       //   Rysuj kolejne piksele w poziomie w tym wierszu.
      {                                                                       //
        OLED_DrawPixel((uint8_t)xx, (uint8_t)yy, c);                          //   Ustaw piksel w zadanym kolorze.
      }                                                                       //
    }                                                                         //
    return;                                                                   //   Zakończ – przypadek poziomy (L/R) obsłużony.
  }
                                                                              //---------------------------------------------------------------
                                                                              //------------ PIONOWO – WYPEŁNIENIE PO Y, GÓRA/DÓŁ -------------
                                                                              //---------------------------------------------------------------
  int y_min   = y;                                                            // Początek prostokąta po Y.
  int y_max   = y + h - 1;                                                    // Koniec prostokąta po Y.
  int total_h = y_max - y_min + 1;                                            // Całkowita wysokość prostokąta.
                                                                              //
  int fill_h = (total_h * (int)p) / 100;                                      // Wysokość faktycznie wypełniona.
  if (fill_h <= 0) return;                                                    // Jeśli po obliczeniu 0 – nie rysuj, zakończ funkcję.
                                                                              //
  int y_draw_min;                                                             // Zakres Y, który będzie wypełniony – początek.
  int y_draw_max;                                                             // Zakres Y, który będzie wypełniony – koniec.
                                                                              //
  if (d == 'T' || d == 't')                                                   // 'T' / 't' – wypełnianie od GÓRY.
  {                                                                           //
    y_draw_min = y_min;                                                       // Start od górnej krawędzi prostokąta.
    y_draw_max = y_min + fill_h - 1;                                          // Koniec wg procentu (w dół).
  }                                                                           //
  else                                                                        // 'B' / 'b' lub inny – wypełnianie od DOŁU.
  {                                                                           //
    y_draw_max = y_max;                                                       // Start od dolnej krawędzi prostokąta.
    y_draw_min = y_max - fill_h + 1;                                          // Początek wg procentu (w górę).
  }                                                                           //
                                                                              //
  if (y_draw_min < 0)               y_draw_min = 0;                           // Przytnij do ekranu u góry.
  if (y_draw_max >= OLED_HEIGHT)    y_draw_max = OLED_HEIGHT - 1;             // Przytnij do ekranu na dole.
                                                                              //
  for (int yy = y_draw_min; yy <= y_draw_max; yy++)                           // Iteracja po wybranych wierszach (po osi Y).
  {                                                                           //
    if (yy < 0 || yy >= OLED_HEIGHT) continue;                                // Dodatkowa kontrola – jeśli poza ekranem, pomiń.
                                                                              //
    for (int xx = x; xx < x + w; xx++)                                        // Rysuj pełną szerokość prostokąta w tym wierszu.
    {                                                                         //
      if (xx < 0 || xx >= OLED_WIDTH) continue;                               // Clipping po X – jeśli poza ekranem, pomiń ten piksel.
                                                                              //
      OLED_DrawPixel((uint8_t)xx, (uint8_t)yy, c);                            // Ustaw piksel w zadanym kolorze.
    }                                                                         //
  }                                                                           //
}                                                                             //


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------- RYSOWANIE KOŁA NA EKRANIE -----------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void OLED_Circle(int x, int y, int r, uint8_t c)                              //---------------------------------------------------------------
{                                                                             //--------------------- RYSUJ KOŁO (OBWÓD) ----------------------
                                                                              //---------------------------------------------------------------
  if (r <= 0) return;                                                         // Jeśli promień ≤ 0 – nie rysuj, zakończ funkcję.
                                                                              //
  int x0     = 0;                                                             // Startowy X na okręgu (początek algorytmu midpoint).
  int y0     = r;                                                             // Startowy Y = promień – punkt na górze okręgu.
  int f     = 1 - r;                                                          // Wartość funkcji decyzyjnej algorytmu midpoint.
  int ddf_x = 1;                                                              // Pochodna/inkrement po X (2*x+1, start od 1).
  int ddf_y = -2 * r;                                                         // Pochodna/inkrement po Y (−2*r, będzie rosła ku zeru).
                                                                              // Punkty kardynalne (góra, dół, lewo, prawo) względem środka (x0, y0)
  OLED_DrawPixel(x,     y + r, c);                                            // ( 0, +r)
  OLED_DrawPixel(x,     y - r, c);                                            // ( 0, −r)
  OLED_DrawPixel(x + r, y,     c);                                            // (+r,  0)
  OLED_DrawPixel(x - r, y,     c);                                            // (−r,  0)

  while (x0 < y0)                                                             // Pętla dopóki x < y – rysujemy oktanty symetrycznie.
  {                                                                           //
    if (f >= 0)                                                               //   Jeśli funkcja decyzyjna ≥ 0 – punkt „wyszedł” na zewnątrz.
    {                                                                         //   Korekta toru: zmniejszamy Y (schodzimy w dół po okręgu).
      y0--;                                                                   //
      ddf_y += 2;                                                             //
      f     += ddf_y;                                                         //
    }                                                                         //
                                                                              //
    x0++;                                                                     //   Przesuń X o 1 w prawo – kolejny krok po osi X.
    ddf_x += 2;                                                               //
    f     += ddf_x;                                                           //
                                                                              //   8 oktantów względem środka (x0, y0)
    OLED_DrawPixel(x + x0, y + y0, c);                                        //   Oktant 1:  (+x, +y)
    OLED_DrawPixel(x - x0, y + y0, c);                                        //   Oktant 2:  (−x, +y)
    OLED_DrawPixel(x + x0, y - y0, c);                                        //   Oktant 8:  (+x, −y)
    OLED_DrawPixel(x - x0, y - y0, c);                                        //   Oktant 7:  (−x, −y)
    OLED_DrawPixel(x + y0, y + x0, c);                                        //   Oktant 4:  (+y, +x)
    OLED_DrawPixel(x - y0, y + x0, c);                                        //   Oktant 3:  (−y, +x)
    OLED_DrawPixel(x + y0, y - x0, c);                                        //   Oktant 5:  (+y, −x)
    OLED_DrawPixel(x - y0, y - x0, c);                                        //   Oktant 6:  (−y, −x)
  }
}

void OLED_FillCircle(int x, int y, int r, uint8_t p, char d, uint8_t c)       //---------------------------------------------------------------
{                                                                             //------------- RYSUJ WYPEŁNIONE KOŁO (BEZ OBRYSU) --------------
                                                                              //---------------------------------------------------------------
  if (r <= 0) return;                                                         // Jeśli promień ≤ 0 – nie rysuj, zakończ funkcję.
  if (p == 0) return;                                                         // Jeśli procent = 0 – nie rysuj, zakończ funkcję.
  if (p > 100) p = 100;                                                       // Zabezpieczenie górnej granicy parametru percent.
  int x_min = x - r;                                                          // Lewa granica bounding-boxa koła (x0 - r).
  int x_max = x + r;                                                          // Prawa granica bounding-boxa koła (x0 + r).
  int y_min = y - r;                                                          // Górna granica bounding-boxa koła (y0 - r).
  int y_max = y + r;                                                          // Dolna granica bounding-boxa koła (y0 + r).
                                                                              //
  int total_w = x_max - x_min + 1;                                            // Całkowita szerokość koła w pikselach (po X).
  if (total_w <= 0) return;                                                   // Jeśli szerokość ≤ 0 – nie rysuj, zakończ funkcję.
                                                                              //
  int clip_min_x = x_min;                                                     // Domyślny clip po X – całe koło (bez ograniczeń L/R).
  int clip_max_x = x_max;                                                     //
  int y_fill_min = y_min;                                                     // Domyślny zakres w pionie – całe koło (bez ograniczeń T/B).
  int y_fill_max = y_max;                                                     //

                                                                              //---------------------------------------------------------------
                                                                              //----------- POZIOMO – WYPEŁNIENIE PO X, LEWO/PRAWO ------------
                                                                              //---------------------------------------------------------------
  if (d == 'L' || d == 'l' || d == 'R' || d == 'r')                           // Kierunek poziomy – procent liczony po osi X.
  {                                                                           //
    int fill_w = (total_w * (int)p + 99) / 100;                               //   Szerokość części wypełnionej, zaokrąglona w górę.
    if (fill_w <= 0) return;                                                  //   Jeśli po zaokrągleniu 0 – nie rysuj, zakończ funkcję.
                                                                              //
    if (d == 'L' || d == 'l')                                                 //   Wypełnianie od LEWEJ strony (L/l).
    {                                                                         //   Wyznacz zakres X od LEWEJ krawędzi bounding-boxa koła.
      clip_min_x = x_min;                                                     //   Minimalny X clipu = lewa krawędź koła.
      clip_max_x = x_min + fill_w - 1;                                        //   Maksymalny X clipu wg procentu.
    }                                                                         //
    else                                                                      //   Wypełnianie od PRAWEJ strony (R/r).
    {                                                                         //   Wyznacz zakres X od PRAWEJ krawędzi bounding-boxa koła.
      clip_max_x = x_max;                                                     //   Maksymalny X clipu = prawa krawędź koła.
      clip_min_x = x_max - fill_w + 1;                                        //   Minimalny X clipu wg procentu.
    }
  }                                                                           //---------------------------------------------------------------
                                                                              //------------ PIONOWO – WYPEŁNIENIE PO Y, GÓRA/DÓŁ -------------
                                                                              //---------------------------------------------------------------
  else if (d == 'T' || d == 't' || d == 'B' || d == 'b')                      // Kierunek pionowy – procent liczony po osi Y.
  {                                                                           //
    int total_h = y_max - y_min + 1;                                          //   Całkowita wysokość koła w pikselach (po Y).
    if (total_h <= 0) return;                                                 //   Jeśli wysokość ≤ 0 – nie rysuj, zakończ funkcję.
                                                                              //
    int fill_h = (total_h * (int)p + 99) / 100;                               //   Wysokość części wypełnionej, zaokrąglona w górę.
    if (fill_h <= 0) return;                                                  //   Jeśli po zaokrągleniu 0 – nie rysuj, zakończ funkcję.
                                                                              //
    if (d == 'T' || d == 't')                                                 //   Wypełnianie od GÓRY (Top).
    {                                                                         //   Wyznacz zakres Y od górnej krawędzi bounding-boxa koła.
      y_fill_min = y_min;                                                     //   Początek zakresu w pionie – górna krawędź koła.
      y_fill_max = y_min + fill_h - 1;                                        //   Koniec zakresu w pionie wg procentu (w dół).
    }                                                                         //
    else                                                                      //   Wypełnianie od DOŁU (Bottom).
    {                                                                         //   Wyznacz zakres Y od dolnej krawędzi bounding-boxa koła.
      y_fill_max = y_max;                                                     //   Koniec zakresu w pionie – dolna krawędź koła.
      y_fill_min = y_max - fill_h + 1;                                        //   Początek zakresu wg procentu (w górę).
    }                                                                         //
                                                                              //
    clip_min_x = x_min;                                                       //   Dla T/B używamy pełnej szerokości koła po X.
    clip_max_x = x_max;                                                       //
  }
                                                                              //---------------------------------------------------------------
                                                                              //--------------- INNY ZNAK – PEŁNE WYPEŁNIENIE -----------------
                                                                              //---------------------------------------------------------------
  else                                                                        // Jeśli dir jest inny niż L/R/T/B.
  {                                                                           //   Zachowanie domyślne – wypełnij całe koło (ignoruj procent).
    y_fill_min = y_min;                                                       //   Pełny zakres w pionie.
    y_fill_max = y_max;                                                       //
    clip_min_x = x_min;                                                       //   Pełny zakres w poziomie.
    clip_max_x = x_max;                                                       //
  }
                                                                              //---------------------------------------------------------------
                                                                              //------------ ALGORYTM MIDPOINT – WYPEŁNIANIE KOŁA -------------
                                                                              //---------------------------------------------------------------
  int x0     = 0;                                                             // Współrzędna X w algorytmie midpoint (start od środka).
  int y0     = r;                                                             // Współrzędna Y początkowo równa promieniowi.
  int f      = 1 - r;                                                         // Wartość funkcji decyzyjnej algorytmu midpoint.
  int ddf_x  = 1;                                                             // Pochodna przyrostu po X (2*x0+1, start od 1).
  int ddf_y  = -2 * r;                                                        // Pochodna przyrostu po Y (−2*r, będzie rosła w górę).
                                                                              //
  while (x0 <= y0)                                                            // Dopóki x0 ≤ y0 – rysujemy wszystkie „pierścienie” bez dziur.
  {                                                                           //
    int y_line;                                                               //   Zmienna pomocnicza – aktualna linia Y do rysowania.
                                                                              //
    y_line = y + y0;                                                          //   Dolna część koła – Y = y + y0.
    if (y_line >= y_fill_min && y_line <= y_fill_max)                         //   Czy ta linia mieści się w zakresie T/B?
    {                                                                         //
      OLED_ClipLine(x - x0, x + x0, y_line, clip_min_x, clip_max_x, c);       //   Rysuj poziomy odcinek w dolnej części koła ograniczony clipem po X (L/R lub pełne).
    }                                                                         //
                                                                              //
    y_line = y - y0;                                                          //   Górna część koła – Y = y - y0.
    if (y_line >= y_fill_min && y_line <= y_fill_max)                         //   Sprawdź zakres pionowy dla tej linii.
    {                                                                         //
      OLED_ClipLine(x - x0, x + x0, y_line, clip_min_x, clip_max_x, c);       //   Rysuj poziomy odcinek w górnej części koła.
    }                                                                         //
                                                                              //
    y_line = y + x0;                                                          //   „Boczna” część – Y = y + x0 (symetria względem osi).
    if (y_line >= y_fill_min && y_line <= y_fill_max)                         //   Sprawdź zakres pionowy dla tej linii.
    {                                                                         //
      OLED_ClipLine(x - y0, x + y0, y_line, clip_min_x, clip_max_x, c);       //   Rysuj poziomy odcinek po lewej/prawej stronie koła.
    }                                                                         //
                                                                              //
    y_line = y - x0;                                                          //   Symetryczna „boczna” linia – Y = y - x0.
    if (y_line >= y_fill_min && y_line <= y_fill_max)                         //   Sprawdź, czy linia mieści się w zakresie T/B.
    {                                                                         //
      OLED_ClipLine(x - y0, x + y0, y_line, clip_min_x, clip_max_x, c);       //   Rysuj poziomy odcinek symetrycznie u góry/dole.
    }                                                                         //
                                                                              //
    if (f >= 0)                                                               //   Jeśli funkcja decyzyjna ≥ 0 – zmniejsz Y (korekta toru).
    {                                                                         //
      y0--;                                                                   //   Przesuń Y o jeden w górę (bliżej środka).
      ddf_y += 2;                                                             //   Zwiększ pochodną po Y.
      f     += ddf_y;                                                         //   Zaktualizuj funkcję decyzyjną o wkład Y.
    }                                                                         //
                                                                              //
    x0++;                                                                     //   Przesuń X o jeden w prawo – kolejny promień.
    ddf_x += 2;                                                               //   Zwiększ pochodną po X.
    f     += ddf_x;                                                           //   Zaktualizuj funkcję decyzyjną o wkład X.
  }                                                                           //
}

void OLED_FillCirclePie(int x, int y, int r, uint8_t p, int k, char d, uint8_t c) //---------------------------------------------------------------
{                                                                                 //--------- RYSUJ WYPEŁNIONY WYCINEK KOŁA (BEZ OBRYSU) ---------- 
                                                                                  //---------------------------------------------------------------
  if (p == 0 || r <= 0) return;                                                   // Jeśli procent = 0 lub promień ≤ 0 – nie rysuj, zakończ funkcję.
  if (p > 100) p = 100;                                                           // Zabezpieczenie górnego zakresu procenta.
  if (p >= 100)                                                                   // Dla 100% – wypełniamy całe koło, bez liczenia kątów.
  {                                                                               //
    int r2 = r * r;                                                               //   Kwadrat promienia do porównywania odległości (bez sqrt).
    for (int dy = -r; dy <= r; dy++)                                              //   Pętla po wierszach kwadratu otaczającego okrąg.
    {                                                                             //   Iteracja po współrzędnej Y względem środka.
      for (int dx = -r; dx <= r; dx++)                                            //   Pętla po kolumnach w tym wierszu, iteracja po współrzędnej X względem środka.
      {                                                                           // 
        if (dx * dx + dy * dy <= r2)                                              //   Jeśli punkt (dx,dy) leży wewnątrz koła (d^2 ≤ r^2).
        {                                                                         //
          int gx = x + dx;                                                        //   Przelicz X punktu do współrzędnych globalnych.
          int gy = y + dy;                                                        //   Przelicz Y punktu do współrzędnych globalnych.
          OLED_DrawPixel((uint8_t)gx, (uint8_t)gy, c);                            //   Narysuj piksel w zadanym kolorze.
        }                                                                         //
      }                                                                           //
    }                                                                             //
    return;                                                                       // Zakończ – całe koło zostało narysowane.
  }                                                                               //
                                                                                  //
  float sweep_deg = 360.0f * (float)p / 100.0f;                                   // Kąt sektora w stopniach odpowiadający percent (0..360).
  if (sweep_deg <= 0.0f) return;                                                  // Jeśli kąt wyszedł ≤ 0 – nie rysuj, zakończ funkcję.
                                                                                  //
  float start = (float)k;                                                         // Kąt początkowy sektora w stopniach (przed normalizacją).
  start = fmodf(start, 360.0f);                                                   // Sprowadź wartość start do zakresu [0..360).
  if (start < 0.0f)                                                               // Jeśli po fmod wyszło ujemne (np. -10°).
    start += 360.0f;                                                              // Dodaj 360°, aby wrócić do przedziału [0..360).
                                                                                  //
  int r2 = r * r;                                                                 // Kwadrat promienia – ponownie do testu, tym razem dla sektora.
                                                                                  //
  for (int dy = -r; dy <= r; dy++)                                                // Pętla po wierszach w kwadracie otaczającym koło.
  {                                                                               //   Iteracja po współrzędnej Y względnej względem środka.
    for (int dx = -r; dx <= r; dx++)                                              //   Pętla po kolumnach w tym samym kwadracie.
    {                                                                             //   Iteracja po współrzędnej X względnej względem środka.
      int gx = x + dx;                                                            //   Współrzędna X w układzie globalnym.
      int gy = y + dy;                                                            //   Współrzędna Y w układzie globalnym.
                                                                                  //
      int d2 = dx * dx + dy * dy;                                                 //   Kwadrat odległości punktu od środka koła.
      if (d2 > r2) continue;                                                      //   Jeśli d^2 > r^2 – punkt leży poza okręgiem.
                                                                                  //
      float ang = atan2f(-(float)dy, (float)dx) * 180.0f / (float)M_PI;           //   Kąt punktu względem osi X (w stopniach, 0° na prawo).
      if (ang < 0.0f) ang += 360.0f;                                              //   Jeśli kąt ujemny (zakres [-180..0)).
                                                                                  //
      uint8_t inside = 0;                                                         //   Flaga – czy punkt należy do wybranego sektora (0/1).
                                                                                  //
      if (d == 'C' || d == 'c')                                                   //   Kierunek zgodny z ruchem wskazówek zegara (Clockwise).
      {                                                                           //   Obsługa sektora narastającego „w dół” kątów.
        float end = start - sweep_deg;                                            //   Kąt końcowy: start -> start - sweep (zgodne z zegarem).
        if (end < 0.0f)                                                           //   Normalizacja kąta końcowego do [0..360).
          end += 360.0f;                                                          //   Dodaj 360°, jeśli wynik był ujemny.
                                                                                  //
        if (start >= end)                                                         //   Przypadek bez przejścia przez 0° (ciągły zakres).
        {                                                                         //   Np. start=300°, end=240° -> sektor [240..300].
          if (ang <= start && ang >= end)                                         //   Jeśli kąt piksela jest między end a start.
            inside = 1;                                                           //   Punkt leży wewnątrz sektora CW.
        }                                                                         //
        else                                                                      //   Przypadek z przejściem przez 0°.
        {                                                                         //   Np. start=10°, end=330° -> [330..360] U [0..10].
          if (ang <= start || ang >= end)                                         //   Punkt w jednym z dwóch zakresów brzegowych.
            inside = 1;                                                           //   Punkt leży wewnątrz sektora CW z wrapem.
        }                                                                         //
      }                                                                           //
      else                                                                        //   Kierunek przeciwny do ruchu wskazówek zegara (Counter-Clockwise).
      {                                                                           //   Obsługa sektora narastającego „w górę” kątów.
        float end = start + sweep_deg;                                            //   Kąt końcowy: start -> start + sweep (przeciwnie do zegara).
        if (end >= 360.0f)                                                        //   Normalizacja kąta końcowego do [0..360).
          end -= 360.0f;                                                          //   Odejmij 360°, jeśli przekracza 360°.
                                                                                  //
        if (start <= end)                                                         //   Przypadek bez przejścia przez 0°.
        {                                                                         //   Np. start=30°, end=120° -> sektor [30..120].
          if (ang >= start && ang <= end)                                         //   Jeśli kąt piksela jest między start a end.
            inside = 1;                                                           //   Punkt leży wewnątrz sektora CCW.
        }                                                                         // 
        else                                                                      //   Przypadek z przejściem przez 0°.
        {                                                                         //   Np. start=330°, end=60° -> [330..360] U [0..60].
          if (ang >= start || ang <= end)                                         //   Jeśli kąt piksela w jednym z dwóch zakresów.
            inside = 1;                                                           //   Punkt leży wewnątrz sektora CCW z wrapem.
        }                                                                         // 
      }                                                                           // 
                                                                                  // 
      if (inside)                                                                 //   Jeśli punkt jest wewnątrz wybranego sektora.
        OLED_DrawPixel((uint8_t)gx, (uint8_t)gy, c);                              //   Narysuj piksel w zadanym kolorze.
    }                                                                             //
  }                                                                               //
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------- RYSOWANIE TRÓJKĄTA NA EKRANIE ---------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void OLED_Triangle(int x1, int y1, int x2, int y2,int x3, int y3, uint8_t c)  //---------------------------------------------------------------
{                                                                             //-------------------- RYSUJ REÓJKĄT (OBWÓD) --------------------
                                                                              //---------------------------------------------------------------
  OLED_Line(x1, y1, x2, y2, c);                                               // Rysuj krawędź między punktem 1 i 2.
  OLED_Line(x2, y2, x3, y3, c);                                               // Rysuj krawędź między punktem 2 i 3.
  OLED_Line(x3, y3, x1, y1, c);                                               // Rysuj krawędź między punktem 3 i 1.
}

void OLED_FillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint8_t p, char d, uint8_t c) //---------------------------------------------------------------
 {                                                                                                   //------------ RYSUJ WYPEŁNIONY TRÓJKĄT (BEZ OBRYSU) ------------
                                                                                                     //---------------------------------------------------------------
  if (p == 0) return;                                                         // Jeśli procent = 0 – nie rysuj, zakończ funkcję.
  if (p > 100) p = 100;                                                       // Zabezpieczenie górnego zakresu procentaNna 100%.
                                                                              //
  int x_min = x1;                                                             // Minimalny X – startowo pierwszy wierzchołek.
  int x_max = x1;                                                             // Maksymalny X – startowo pierwszy wierzchołek.
                                                                              //
  if (x2 < x_min) x_min = x2;                                                 // Aktualizuj x_min na podstawie X2.
  if (x3 < x_min) x_min = x3;                                                 // Aktualizuj x_min na podstawie X3.
  if (x2 > x_max) x_max = x2;                                                 // Aktualizuj x_max na podstawie X2.
  if (x3 > x_max) x_max = x3;                                                 // Aktualizuj x_max na podstawie X3.
                                                                              //
  int y_min = y1;                                                             // Minimalny Y – startowo pierwszy wierzchołek.
  int y_max = y1;                                                             // Maksymalny Y – startowo pierwszy wierzchołek.
                                                                              //
  if (y2 < y_min) y_min = y2;                                                 // Aktualizuj y_min na podstawie Y2.
  if (y3 < y_min) y_min = y3;                                                 // Aktualizuj y_min na podstawie Y3.
  if (y2 > y_max) y_max = y2;                                                 // Aktualizuj y_max na podstawie Y2.
  if (y3 > y_max) y_max = y3;                                                 // Aktualizuj y_max na podstawie Y3.
                                                                              //
  if (y_min > y_max) return;                                                  // Jeśli min Y > max Y – nie rysuj, zakończ funkcję.
                                                                              //
  int clip_min_x = x_min;                                                     // Domyślny clip po X – cały bounding-box trójkąta.
  int clip_max_x = x_max;                                                     // Domyślny clip po X – cały bounding-box trójkąta.
  int y_start    = y_min;                                                     // Domyślny początek pętli po Y – od góry bboxa.
  int y_end      = y_max;                                                     // Domyślny koniec pętli po Y – do dołu bboxa.

                                                                              //---------------------------------------------------------------
                                                                              //----------- POZIOMO – WYPEŁNIENIE PO X, LEWO/PRAWO ------------
                                                                              //---------------------------------------------------------------
  if (d == 'L' || d == 'l' || d == 'R' || d == 'r')                           // Kierunek poziomy – analogicznie jak w prostokącie.
  {                                                                           // 
    int total_w = x_max - x_min + 1;                                          //   Całkowita szerokość bboxa trójkąta po osi X.
    if (total_w <= 0) return;                                                 //   Jeżeli szerokość ≤ 0 – nie rysuj, zakończ funkcję.
                                                                              //
    int fill_w = (total_w * (int)p + 99) / 100;                               //   Szerokość części wypełnionej, zaokrąglona w górę.
    if (fill_w <= 0) return;                                                  //   Jeśli po zaokrągleniu wyszło 0 – nie rysuj, zakończ funkcję.
                                                                              //
    if (d == 'L' || d == 'l')                                                 //   Wypełnianie od lewej strony (L/l).
    {                                                                         //   Wyznacz zakres X od LEWEJ krawędzi bounding-boxa.
      clip_min_x = x_min;                                                     //   Minimalny X clipu = lewa krawędź bboxa.
      clip_max_x = x_min + fill_w - 1;                                        //   Maksymalny X clipu wg procentu.
    }                                                                         //
    else                                                                      //   Wypełnianie od prawej strony (R/r).
    {                                                                         //   Wyznacz zakres X od PRAWEJ krawędzi bounding-boxa.
      clip_max_x = x_max;                                                     //   Maksymalny X clipu = prawa krawędź bboxa.
      clip_min_x = x_max - fill_w + 1;                                        //   Minimalny X clipu wg procentu.
    } 
  }                                                                           //---------------------------------------------------------------
                                                                              //------------ PIONOWO – WYPEŁNIENIE PO Y, GÓRA/DÓŁ -------------
                                                                              //---------------------------------------------------------------
  else if (d == 'T' || d == 't' || d == 'B' || d == 'b')                      // Kierunek pionowy – procent po osi Y.
  {                                                                           // 
    int total_h = y_max - y_min + 1;                                          //   Całkowita wysokość bboxa trójkąta po osi Y.
    if (total_h <= 0) return;                                                 //   Jeżeli wysokość ≤ 0 – nie rysuj, zakończ funkcję.
                                                                              //
    int fill_h = (total_h * (int)p + 99) / 100;                               //   Wysokość części wypełnionej, zaokrąglona w górę.
    if (fill_h <= 0) return;                                                  //   Jeśli po zaokrągleniu wyszło 0 – nie rysuj, zakończ funkcję.
                                                                              //
    if (d == 'T' || d == 't')                                                 //   Wypełnianie od GÓRY (Top).
    {                                                                         //   Wyznacz zakres Y od górnej krawędzi bounding-boxa.
      y_start = y_min;                                                        //   Początek pętli po Y – górna krawędź bboxa.
      y_end   = y_min + fill_h - 1;                                           //   Koniec pętli po Y wg procentu.
    }                                                                         //
    else                                                                      //   Wypełnianie od DOŁU (Bottom).
    {                                                                         //   Wyznacz zakres Y od dolnej krawędzi bounding-boxa.
      y_end   = y_max;                                                        //   Koniec pętli po Y – dolna krawędź bboxa.
      y_start = y_max - fill_h + 1;                                           //   Początek pętli po Y wg procentu.
    } 
  }                                                                           //---------------------------------------------------------------
                                                                              //--------------- INNE ZNAKI - PEŁNE WYPEŁNIENIE ----------------
                                                                              //---------------------------------------------------------------
  else                                                                        // Jeśli dir jest inny niż L/R/T/B.
  {                                                                           // Zachowanie domyślne – wypełnij cały trójkąt.
                                                                              // (nie zmieniamy – rysujemy 100%).
  }            
                                                                              //---------------------------------------------------------------
                                                                              //- GŁÓWNA PĘTLA SCANLINE – PRZECIĘCIA Z KRAWĘDZIAMI I ODCINKI --
                                                                              //---------------------------------------------------------------
  for (int y = y_start; y <= y_end; y++)                                      // Iteruj po wierszach tylko w wybranym zakresie Y.
  {                                                                           // Początek obsługi pojedynczej linii scanline.
    float x_intersections[3];                                                 //   Tablica na maksymalnie 3 przecięcia z krawędziami.
    int   count = 0;                                                          //   Licznik znalezionych przecięć w bieżącym wierszu.
                                                                              //
    if ((y >= y1 && y < y2) || (y >= y2 && y < y1))                           //   Czy scanline przecina krawędź (x1,y1)-(x2,y2)?
    {                                                                         //   Jeśli tak – liczymy parametr t dla tej krawędzi.
      float t = (float)(y - y1) / (float)(y2 - y1);                           //   Parametr t – położenie punktu na krawędzi.
      x_intersections[count++] = (float)x1 + t * (float)(x2 - x1);            //   Oblicz X przecięcia i zapisz w tablicy.
    }                                                                         //   Koniec obsługi przecięcia z krawędzią 1-2.
                                                                              //
    if ((y >= y2 && y < y3) || (y >= y3 && y < y2))                           //   Czy scanline przecina krawędź (x2,y2)-(x3,y3)?
    {                                                                         //   Jeśli tak – liczymy parametr t dla tej krawędzi.
      float t = (float)(y - y2) / (float)(y3 - y2);                           //   Parametr t – położenie punktu na krawędzi.
      x_intersections[count++] = (float)x2 + t * (float)(x3 - x2);            //   Oblicz X przecięcia i zapisz w tablicy.
    }                                                                         //   Koniec obsługi przecięcia z krawędzią 2-3.
                                                                              //
    if ((y >= y3 && y < y1) || (y >= y1 && y < y3))                           //   Czy scanline przecina krawędź (x3,y3)-(x1,y1)?
    {                                                                         //   Jeśli tak – liczymy parametr t dla tej krawędzi.
      float t = (float)(y - y3) / (float)(y1 - y3);                           //   Parametr t – położenie punktu na krawędzi.
      x_intersections[count++] = (float)x3 + t * (float)(x1 - x3);            //   Oblicz X przecięcia i zapisz w tablicy.
    }                                                                         //   Koniec obsługi przecięcia z krawędzią 3-1.
                                                                              //
    if (count < 2) continue;                                                  //   Jeżeli przecięć jest mniej niż 2 – brak odcinka do rysowania.
                                                                              //
    int xa = (int)(x_intersections[0] + 0.5f);                                //   Zaokrąglij pierwszy punkt przecięcia do najbliższego X.
    int xb = (int)(x_intersections[1] + 0.5f);                                //   Zaokrąglij drugi punkt przecięcia do najbliższego X.
                                                                              //
    OLED_ClipLine(xa, xb, y, clip_min_x, clip_max_x, c);                      //   Rysuj poziomy odcinek od xa do xb w wierszu Y, ograniczony clipem po X
  }                                                                           //   (procent L/R) lub pełnym bboxem, Użyj zadanego koloru piksela.                                                                  
}        


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------- RYSOWANIE TEKSTU NA EKRANIE ----------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static uint16_t OLED_TextWidth(const OLED_Font_t *fnt, const char *txt)       //---------------------------------------------------------------
{                                                                             //---------------- SZEROKOŚĆ TEKSTU W PIKSELACH -----------------
                                                                              //---------------------------------------------------------------
  if (!fnt || !txt) return 0;                                                 // Brak czcionki lub tekstu - zwróć 0.
                                                                              //
  uint16_t    width   = 0;                                                    // Sumaryczna szerokość tekstu w pikselach.
  uint8_t     w_max   = fnt->width;                                           // Maksymalna szerokość pojedynczego znaku.
  uint8_t     spacing = fnt->letter_spacing;                                  // Odstęp poziomy między znakami.
  const char *p       = txt;                                                  // Wskaźnik iteracyjny po buforze tekstu.
                                                                              //
  while (*p)                                                                  // Pętla po kolejnych znakach aż do znaku '\0'.
  {                                                                           //
    char    ch = *p++;                                                        //   Pobierz aktualny znak i przesuwaj wskaźnik na następny.
    uint8_t c  = (uint8_t)ch;                                                 //   Rzutuj znak na kod bajtowy (0–255).
                                                                              //
    if (c < fnt->first_char || c > fnt->last_char)                            //   Jeśli kod znaku jest poza zakresem obsługiwanym przez czcionkę
      c = fnt->first_char;                                                    //   użyj znaku domyślnego (pierwszy w zakresie).
                                                                              //
    uint16_t index   = (uint16_t)(c - fnt->first_char);                       //   Indeks znaku w tabelach czcionki.
    uint8_t  glyph_w = w_max;                                                 //   Domyślna szerokość znaku (maksymalna).
                                                                              //
    if (fnt->widths != NULL)                                                  //   Jeśli istnieje tabela indywidualnych szerokości znaków
    {                                                                         //   spróbuj pobrać dokładną szerokość tego znaku.
      uint8_t w_eff = fnt->widths[index];                                     //   Efektywna szerokość znaku z tabeli widths[].
      if (w_eff > 0 && w_eff <= w_max)                                        //   Jeśli szerokość ma sensowną wartość (1..w_max)
        glyph_w = w_eff;                                                      //   to przyjmij ją jako szerokość znaku.
    }                                                                         //
                                                                              //
    width = (uint16_t)(width + glyph_w);                                      //   Dodaj szerokość znaku do sumarycznej szerokości.
                                                                              //
    if (*p)                                                                   //   Jeśli to nie był ostatni znak (następny != '\0' dodaj odstęp między znakami.
      width = (uint16_t)(width + spacing);                                    //
  }                                                                           // Koniec pętli po znakach.
                                                                              //
  return width;                                                               // Zwróć policzoną szerokość tekstu w pikselach.
}

void OLED_SetFont(const OLED_Font_t *font)                                    //---------------------------------------------------------------
{                                                                             //--------------- USTAW AKTUALNĄ CZCIONKĘ EKRANU ----------------
                                                                              //---------------------------------------------------------------
  if (font != NULL)                                                           // Jeśli przekazano wskaźnik na czcionkę:
  {                                                                           //
    OLED_CurrentFont = font;                                                  //   Ustaw tę czcionkę jako bieżącą.
  }                                                                           //
  else                                                                        // Jeśli font == NULL:
  {                                                                           //
    OLED_CurrentFont = &Font_5x7;                                             //   Ustaw domyślną czcionkę 5x7.
  }
}

const OLED_Font_t* OLED_GetFont(void)                                         //---------------------------------------------------------------
{                                                                             //---------- ZWRÓĆ AKTUALNĄ USTAWIONĄ CZCIONKĘ EKRANU -----------
                                                                              //---------------------------------------------------------------
  return OLED_CurrentFont;                                                    // Zwróć wskaźnik na aktualną czcionkę.
}

void OLED_DrawChar(uint8_t x, uint8_t y, char c)                              //---------------------------------------------------------------
{                                                                             //---------------- NARYSUJ POJEDYNCZY ZNAK ASCII ----------------
                                                                              //---------------------------------------------------------------
  const OLED_Font_t *font = OLED_CurrentFont;                                 // Pobierz bieżącą czcionkę.
                                                                              //
  if (font == NULL) return;                                                   // Jeśli brak fontu – nic nie rysuj.
  if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;                            // Jeśli (x,y) poza ekranem – wyjdź.
                                                                              //
  uint8_t ch = (uint8_t)c;                                                    // Rzutuj znak na uint8_t.
                                                                              //
  if (ch < font->first_char || ch > font->last_char)                          // Jeśli znak spoza zakresu fontu:
    ch = font->first_char;                                                    //   Podmień na znak domyślny (np. spacja).
                                                                              //
  uint8_t  h              = font->height;                                     // Wysokość znaku w pikselach.
  uint8_t  bytes_per_col  = (uint8_t)((h + 7U) / 8U);                         // Bajtów na kolumnę (1 dla h≤8, 2 dla 9..16).
  uint8_t  w_max          = font->width;                                      // Maks. szerokość glifu (z generatora).
  uint16_t bytes_per_char = (uint16_t)bytes_per_col * w_max;                  // Całkowita liczba bajtów na znak.
  uint16_t index = (uint16_t)(ch - font->first_char);                         // Indeks znaku w tablicy fontu.
  uint8_t glyph_w = w_max;                                                    // Domyślnie przyjmij maksymalną szerokość znaku.
                                                                              //
  if (font->widths != NULL)                                                   // Jeśli font ma tablicę szerokości.
  {                                                                           //
    uint8_t w_eff = font->widths[index];                                      //   Szerokość efektywna dla danego znaku.
    if (w_eff > 0 && w_eff <= w_max)                                          //   Jeśli szerokość mieści się w sensownym zakresie:
      glyph_w = w_eff;                                                        //     Ustaw efektywną szerokość znaku.
  }                                                                           //
                                                                              //
  const uint8_t *glyph = font->data + (index * bytes_per_char);               // Wskaźnik na dane glifu w tablicy fontu.
                                                                              //
  for (uint8_t col = 0; col < glyph_w; col++)                                 // Pętla po rzeczywistych kolumnach znaku.
  {                                                                           //
    for (uint8_t row = 0; row < h; row++)                                     // Pętla po wierszach znaku.
    {                                                                         //
      uint8_t px = (uint8_t)(x + col);                                        //   Współrzędna X aktualnego piksela.
      uint8_t py = (uint8_t)(y + row);                                        //   Współrzędna Y aktualnego piksela.
                                                                              //
      if (px >= OLED_WIDTH || py >= OLED_HEIGHT) continue;                    //   Jeśli wyjdziemy poza ekran – pomiń piksel:
                                                                              //
      uint8_t page_index = (uint8_t)(row / 8U);                               //   Który bajt (strona) w kolumnie.
      uint8_t bit_index  = (uint8_t)(row % 8U);                               //   Który bit w tym bajcie.
      uint8_t columnByte = glyph[col * bytes_per_col + page_index];           //   Bajt danych dla tej kolumny/strony.
                                                                              //
      if (columnByte & (1U << bit_index))                                     // Jeśli bit ustawiony – piksel = 1:
      {                                                                       //
        OLED_DrawPixel(px, py, 1);                                            //   Rysuj biały piksel (ON).
      }                                                                       //
      else                                                                    // W przeciwnym wypadku piksel = 0:
      {                                                                       //
        OLED_DrawPixel(px, py, 0);                                            //   Rysuj czarny piksel (OFF).
      }
    }
  }
                                                                              // Jedna kolumna odstępu za znakiem (letter spacing = 1px).
  uint8_t spacingX = (uint8_t)(x + glyph_w);                                  // Kolumna X przeznaczona na odstęp za glifem.
  if (spacingX < OLED_WIDTH)                                                  // Jeśli nie wychodzi poza ekran:
  {                                                                           //
    for (uint8_t row = 0; row < h; row++)                                     //   Iteracja po wierszach znaku.
    {                                                                         //
      uint8_t py = (uint8_t)(y + row);                                        //     Współrzędna Y dla odstępu.
      if (py < OLED_HEIGHT)                                                   //     Sprawdź, czy w granicach ekranu:
      {                                                                       //
        OLED_DrawPixel(spacingX, py, 0);                                      //       Czyść kolumnę odstępu na czarno.
      }
    }
  }
}

uint8_t OLED_GetCenteredX(const char *txt)                                    //---------------------------------------------------------------
{                                                                             //----- Zwróć X tak, aby tekst był wyśrodkowany w poziomie ------
                                                                              //---------------------------------------------------------------
  const OLED_Font_t *font = OLED_GetFont();                                   // Pobierz aktualną czcionkę.
  if (!font || !txt) return 0;                                                // Brak czcionki lub tekstu -> 0.

  uint16_t text_w = OLED_TextWidth(font, txt);                                // Użyj funkcji liczącej szerokość (tę z poprzedniej wersji).
  if (text_w >= OLED_WIDTH) return 0;                                         // Tekst szerszy niż ekran -> zacznij od lewej.

  return (uint8_t)((OLED_WIDTH - text_w) / 2U);                               // Wyśrodkowany X.
}

uint8_t OLED_GetCenteredY(void)                                               //---------------------------------------------------------------
{                                                                             //------ Zwróć Y tak, aby tekst był wyśrodkowany w pionie -------
                                                                              //---------------------------------------------------------------
  const OLED_Font_t *font = OLED_GetFont();
  if (!font) return 0;

  if (font->height >= OLED_HEIGHT) return 0;                                  // Czcionka wyższa niż ekran -> od góry.

  return (uint8_t)((OLED_HEIGHT - font->height) / 2U);
}

void OLED_DrawString(uint8_t x, uint8_t y, const char *txt,const char *c)     //---------------------------------------------------------------
{                                                                             //------ WYŚWIETL ŁAŃCUCH ZNAKÓW ASCII NA EKRANIE (TEKST) -------
                                                                              //---------------------------------------------------------------
  const OLED_Font_t *font = OLED_GetFont();                                   // Pobierz aktualną czcionkę.
  if (!font || !txt) return;                                                  // Jeśli brak czcionki lub tekstu – wyjście.

  uint8_t pos_x = x;                                                          // Pozycja startowa X.
  uint8_t pos_y = y;                                                          // Pozycja startowa Y.
  uint16_t text_w = 0;
  uint8_t  cx = 0;
  uint8_t  cy = 0;

  if (c && c[0] == 'C')                                                       // Jest tryb centrowania?
  {
                                                                              // Czy ten tryb wymaga centrowania w poziomie (X)?
    int center_x_mode = 0;
    int center_y_mode_screen = 0;
    int center_y_mode_page   = 0;
    uint8_t page_index       = 0;                                             // 0..7 dla stron 1..8.

    if (c[1] == 'X')
    {
      center_x_mode = 1;                                                      // "CX" lub "CXY" -> centrowanie X.
      if (c[2] == 'Y')                                                        // "CXY"
      {
        center_y_mode_screen = 1;
      }
    }
    else if (c[1] == 'Y' && c[2] == '\0')                                     // "CY"
    {
      center_y_mode_screen = 1;
    }
    else if (c[1] >= '1' && c[1] <= '8' && c[2] == '\0')                      // "C1".."C8"
    {
      center_x_mode      = 1;                                                 // C1..C8 też centrowanie po X.
      center_y_mode_page = 1;                                                 // Y centrowany w wybranej stronie.
      page_index         = (uint8_t)(c[1] - '1');                             // '1'->0, '2'->1, ..., '8'->7.
    }
    else if (c[1] == 'X' && c[2] == 'Y' && c[3] == '\0')                      // "CXY" (gdyby ktoś pisał inaczej)
    {
      center_x_mode        = 1;
      center_y_mode_screen = 1;
    }

    //----------------------------------------------------------------------
    //  CENTROWANIE PO X (CAŁY EKRAN)
    //----------------------------------------------------------------------
    if (center_x_mode)
    {
      text_w = OLED_TextWidth(font, txt);                                     // Oblicz szerokość tekstu.
      if (text_w >= OLED_WIDTH)                                               // Tekst szerszy niż ekran -> zaczynaj od 0.
      {
        cx = 0;
      }
      else
      {
        cx = (uint8_t)((OLED_WIDTH - text_w) / 2U);                           // Środek ekranu w poziomie.
      }

      if (x == 0)
      {
        pos_x = cx;                                                           // Czyste centrowanie.
      }
      else
      {
        pos_x = (uint8_t)(cx + x);                                            // Centrowanie + offset X.
      }
    }

    //----------------------------------------------------------------------
    //  CENTROWANIE PO Y – CAŁY EKRAN
    //----------------------------------------------------------------------
    if (center_y_mode_screen)
    {
      if (font->height >= OLED_HEIGHT)                                        // Czcionka wyższa niż ekran -> od góry.
      {
        cy = 0;
      }
      else
      {
        cy = (uint8_t)((OLED_HEIGHT - font->height) / 2U);                    // Środek ekranu w pionie.
      }

      if (y == 0)
      {
        pos_y = cy;
      }
      else
      {
        pos_y = (uint8_t)(cy + y);
      }
    }

    //----------------------------------------------------------------------
    //  CENTROWANIE PO Y – W OBRĘBIE STRONY (C1..C8)
    //----------------------------------------------------------------------
    if (center_y_mode_page)
    {
      uint8_t page_y0 = (uint8_t)(page_index * 8U);                           // Początek strony w pikselach Y (0,8,16,...).

      if (font->height >= 8U)                                                 // Jeśli czcionka wyższa/równa stronie.
      {
        cy = page_y0;                                                         // Rysuj od góry strony.
      }
      else
      {
        uint8_t diff = (uint8_t)((8U - font->height) / 2U);                   // Wycentrowanie w obrębie 8 px.
        cy = (uint8_t)(page_y0 + diff);
      }

      if (y == 0)
      {
        pos_y = cy;
      }
      else
      {
        pos_y = (uint8_t)(cy + y);
      }
    }
  }

  //---------------------------------------------------------------------------
  //  WŁAŚCIWE RYSOWANIE TEKSTU OD (pos_x, pos_y)
  //---------------------------------------------------------------------------
  uint8_t w_max    = font->width;                                             // Maksymalna szerokość znaku.
  uint8_t spacing  = font->letter_spacing;                                    // Odstęp między znakami zdefiniowany w czcionce.
  uint8_t cursor_x = pos_x;                                                   // Bieżąca pozycja X.

  while (*txt)
  {
    char ch = *txt++;                                                         // Pobierz znak i przesuwaj wskaźnik łańcucha.

    uint8_t c = (uint8_t)ch;
    if (c < font->first_char || c > font->last_char)                          // Znak spoza zakresu – zamiana na znak domyślny.
    {
      c = font->first_char;
    }

    uint16_t index = (uint16_t)(c - font->first_char);                        // Indeks znaku w tablicy czcionki.

    uint8_t glyph_w = w_max;                                                  // Podstawowa szerokość znaku do przesuwania kursora.
    if (font->widths != NULL)
    {
      uint8_t w_eff = font->widths[index];                                    // Efektywna szerokość znaku.
      if (w_eff > 0 && w_eff <= w_max)
      {
        glyph_w = w_eff;
      }
    }

    if (cursor_x + glyph_w > OLED_WIDTH) break;                               // Sprawdź, czy znak zmieści się jeszcze w linii.
      

    OLED_DrawChar(cursor_x, pos_y, (char)c);                                  // Narysuj znak.

    cursor_x = (uint8_t)(cursor_x + glyph_w + spacing);                       // Przesuń kursor o szerokość znaku + odstęp z czcionki.

    if (cursor_x >= OLED_WIDTH) break;                                        // Jeśli kursor wyszedł poza ekran – przerwij.
      
  }
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------- FUNKCJE KONWERSJI LICZB NA NAPISY (INT / UINT / FLOAT / DOUBLE) ------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void OLED_IntToString(int32_t value, char *buf)                        //---------------------------------------------------------------
{                                                                             //-------------- KONWERSJA LICZBY INT32_T NA TEKST --------------
                                                                              //---------------------------------------------------------------
  char tmp[12];                                                               // Bufor tymczasowy na cyfry (max 11 znaków + znak końca).
  int  i = 0;                                                                 // Indeks bieżącej pozycji w buforze tymczasowym.
  uint32_t v;                                                                 // Zmienna na wartość bez znaku.
                                                                              //
  if (value < 0)                                                              // Jeśli liczba ujemna.
  {                                                                           //
    *buf++ = '-';                                                             // Dodaj minus na początek wyniku.
    v = (uint32_t)(-value);                                                   // Przekształć na wartość dodatnią (wartość bez znaku).
  }                                                                           //
  else                                                                        // Jeśli liczba dodatnia.
  {                                                                           //
    v = (uint32_t)value;                                                      // Przepisz wartość do zmiennej bez znaku.
  }                                                                           //
                                                                              //
  do                                                                          //
  {                                                                           // Rozbijanie liczby na cyfry w odwrotnej kolejności.
    tmp[i++] = (char)('0' + (v % 10u));                                       // Dodaj najmłodszą cyfrę do bufora tymczasowego.
    v /= 10u;                                                                 // Podziel przez 10 (przesunięcie na kolejną cyfrę).
  } 
  while (v > 0u);                                                             // Powtarzaj, dopóki są jeszcze cyfry.
                                                                              //
  while (i > 0)                                                               // Przepisz cyfry w poprawnej kolejności do docelowego bufora.
  {                                                                           //
    *buf++ = tmp[--i];                                                        // Przenosimy od końca bufora tymczasowego.
  }                                                                           //
  *buf = '\0';                                                                // Dodaj znak końca stringa.
}

static void OLED_UIntToString(uint32_t value, char *buf)                      //---------------------------------------------------------------
{                                                                             //------------- KONWERSJA LICZBY UINT32_T NA TEKST --------------
                                                                              //---------------------------------------------------------------
  char tmp[11];                                                               // Bufor na cyfry (max 10 cyfr 32-bit + zakończenie).
  int  i = 0;                                                                 // Indeks w buforze tymczasowym.
                                                                              //
  do                                                                          //
  {                                                                           // Rozbijanie liczby na cyfry w odwrotnej kolejności.
    tmp[i++] = (char)('0' + (value % 10u));                                   // Najmłodsza cyfra do bufora.
    value /= 10u;                                                             // Przejdź do kolejnej cyfry.
  }                                                                           //
  while (value > 0u);                                                         // Powtarzaj dopóki wartość > 0.
                                                                              //
  while (i > 0)                                                               // Przepisz do bufora docelowego w prawidłowej kolejności.
  {                                                                           //
    *buf++ = tmp[--i];                                                        // Kopiuj od końca bufora tymczasowego.
  }                                                                           //
  *buf = '\0';                                                                // Zakończ string zerem.
}

static void OLED_FloatToString(float value, uint8_t precision, char *buf)     //---------------------------------------------------------------
{                                                                             //------------- KONWERSJA FLOAT NA TEKST Z PRECYZJĄ -------------
                                                                              //---------------------------------------------------------------
  if (precision > 4) precision = 4;                                           // Ogranicz ilość miejsc po przecinku do max 4.
                                                                              //
  if (value < 0.0f)                                                           // Jeśli liczba ujemna.
  {                                                                           //
    *buf++ = '-';                                                             // Dodaj znak minus.
    value = -value;                                                           // Pracuj na wartości dodatniej.
  }                                                                           //
                                                                              //
  uint32_t ipart = (uint32_t)value;                                           // Część całkowita liczby.
  float    fpart = value - (float)ipart;                                      // Część ułamkowa liczby.
                                                                              //
  char tmp[12];                                                               // Bufor tymczasowy dla części całkowitej.
  int  i = 0;                                                                 // Indeks w buforze tymczasowym.
                                                                              //
  do                                                                          //
  {                                                                           // Rozbij część całkowitą na cyfry.
    tmp[i++] = (char)('0' + (ipart % 10u));                                   // Dodaj cyfrę.
    ipart /= 10u;                                                             // Podziel przez 10.
  }                                                                           //
  while (ipart > 0u);                                                         // Dopóki są cyfry.
                                                                              //
  while (i > 0)                                                               // Przepisz część całkowitą w prawidłowej kolejności.
  {                                                                           //
    *buf++ = tmp[--i];                                                        // Przenoszenie od końca.
  }                                                                           //
                                                                              //
  if (precision > 0)                                                          // Jeśli ma być coś po przecinku.
  {                                                                           //
    *buf++ = '.';                                                             // Dodaj separator dziesiętny.
                                                                              //
    uint32_t mul = 1u;                                                        // Mnożnik 10^precision dla części ułamkowej.
    for (uint8_t p = 0; p < precision; p++)                                   // Oblicz 10^precision.
    {                                                                         //
      mul *= 10u;                                                             //
    }                                                                         //
                                                                              //
    uint32_t frac = (uint32_t)(fpart * (float)mul + 0.5f);                    // Zaokrąglona część ułamkowa jako liczba całkowita.
                                                                              //
    for (int p = (int)precision - 1; p >= 0; p--)                             // Zapisz cyfry od końca.
    {                                                                         //
      tmp[p] = (char)('0' + (frac % 10u));                                    // Wyciągaj cyfry od końca.
      frac /= 10u;                                                            // Podziel przez 10.
    }                                                                         //
    for (uint8_t p = 0; p < precision; p++)                                   // Przepisz cyfry we właściwej kolejności.
    {
      *buf++ = tmp[p];                                                        // Dodaj cyfry po przecinku.
    }
  }                                                                           //
                                                                              //
  *buf = '\0';                                                                // Zakończ string.
}

static void OLED_DoubleToString(double value, uint8_t precision, char *buf)   //---------------------------------------------------------------
{                                                                             //------------ KONWERSJA DOUBLE NA TEKST Z PRECYZJĄ -------------
                                                                              //---------------------------------------------------------------
  if (precision > 6) precision = 6;                                           // Ogranicz ilość miejsc po przecinku do max 6.
                                                                              //
  if (value < 0.0)                                                            // Jeśli liczba ujemna.
  {                                                                           //
    *buf++ = '-';                                                             // Dodaj znak minus.
    value = -value;                                                           // Pracuj na dodatniej wartości.
  }                                                                           //
                                                                              //
  uint32_t ipart = (uint32_t)value;                                           // Część całkowita liczby.
  double   fpart = value - (double)ipart;                                     // Część ułamkowa.
                                                                              //
  char tmp[16];                                                               // Bufor tymczasowy dla części całkowitej.
  int  i = 0;                                                                 // Indeks w buforze tymczasowym.
                                                                              //
  do                                                                          //
  {                                                                           // Rozbij część całkowitą na cyfry.
    tmp[i++] = (char)('0' + (ipart % 10u));                                   // Dodaj cyfrę.
    ipart /= 10u;                                                             // Podziel przez 10.
  }                                                                           //
  while (ipart > 0u);                                                         // Dopóki są cyfry.
                                                                              //
  while (i > 0)                                                               // Przepisz w prawidłowej kolejności.
  {                                                                           //
    *buf++ = tmp[--i];                                                        // Kopiuj od końca.
  }                                                                           //
                                                                              //
  if (precision > 0)                                                          // Jeśli chcemy część ułamkową.
  {                                                                           //
    *buf++ = '.';                                                             // Dodaj separator dziesiętny.
                                                                              //
    uint32_t mul = 1u;                                                        // Mnożnik 10^precision.
    for (uint8_t p = 0; p < precision; p++)                                   // Oblicz mnożnik.
    {
      mul *= 10u;                                                             //
    }
                                                                              //
    uint32_t frac = (uint32_t)(fpart * (double)mul + 0.5);                    // Zaokrąglona część ułamkowa.
                                                                              //
    for (int p = (int)precision - 1; p >= 0; p--)                             // Wypełnij bufor cyframi od końca.
    {                                                                         //
      tmp[p] = (char)('0' + (frac % 10u));                                    // Wyciągaj ostatnią cyfrę.
      frac /= 10u;                                                            // Podziel przez 10.
    }                                                                         //
    for (uint8_t p = 0; p < precision; p++)                                   // Przepisz cyfry do wyniku.
    {
      *buf++ = tmp[p];                                                        // Dodaj kolejne cyfry po przecinku.
    }
  }                                                                           //
                                                                              //
  *buf = '\0';                                                                // Zakończ string.
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------- FUNKCJE WYŚWIETLANIA WARTOŚCI NA EKRANIE (INT / UINT / FLOAT / DOUBLE) -----------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void OLED_PrintInt(int x, int y, int32_t v, const char *s)                    //---------------------------------------------------------------
{                                                                             //----------- WYŚWIETL INT32_T Z OPCJONALNYM SUFIKSEM -----------
                                                                              //---------------------------------------------------------------
  char buf[24];                                                               // Bufor na tekst liczby + sufiks.
  OLED_IntToString(v, buf);                                                   // Konwersja liczby całkowitej na string.
  if (s != NULL)                                                              // Jeśli podano sufiks (np. " V", " A").
  {                                                                           //
    strncat(buf, s, sizeof(buf) - strlen(buf) - 1);                           // Dodaj sufiks z kontrolą długości bufora.
  }                                                                           //
  OLED_DrawString((uint8_t)x, (uint8_t)y, buf, NULL);                         // Wyświetl tekst na ekranie.
}

void OLED_PrintUInt(int x, int y, uint32_t v, const char *s)                  //---------------------------------------------------------------
{                                                                             //---------- WYŚWIETL UINT32_T Z OPCJONALNYM SUFIKSEM -----------
                                                                              //---------------------------------------------------------------
  char buf[24];                                                               // Bufor na tekst liczby + sufiks.
  OLED_UIntToString(v, buf);                                                  // Konwersja liczby bez znaku na string.
  if (s != NULL)                                                              // Jeśli sufiks nie jest NULL.
  {                                                                           //
    strncat(buf, s, sizeof(buf) - strlen(buf) - 1);                           // Dopisz sufiks pilnując bufora.
  }                                                                           //
  OLED_DrawString((uint8_t)x, (uint8_t)y, buf, NULL);                         // Wyświetl wynik na ekranie.
}

void OLED_PrintFloat(int x, int y, float v, uint8_t p, const char *s)         //---------------------------------------------------------------
{                                                                             //------------ WYŚWIETL FLOAT Z PRECYZJĄ I SUFIKSEM -------------
                                                                              //---------------------------------------------------------------
  char buf[32];                                                               // Bufor na tekst liczby zmiennoprzecinkowej.
  OLED_FloatToString(v, p, buf);                                              // Konwersja float -> string z daną precyzją.
  if (s != NULL)                                                              // Jeśli sufiks istnieje.
  {                                                                           //
    strncat(buf, s, sizeof(buf) - strlen(buf) - 1);                           // Dopisz sufiks do bufora.
  }                                                                           //
  OLED_DrawString((uint8_t)x, (uint8_t)y, buf, NULL);                         // Wyświetl wynikowy string.
}

void OLED_PrintDouble(int x, int y, double v, uint8_t p, const char *s)       //---------------------------------------------------------------
{                                                                             //------------ WYŚWIETL DOUBLE Z PRECYZJĄ I SUFIKSEM ------------
                                                                              //---------------------------------------------------------------
  char buf[32];                                                               // Bufor na tekst liczby typu double.
  OLED_DoubleToString(v, p, buf);                                             // Konwersja double -> string z daną precyzją.
  if (s != NULL)                                                              // Jeśli sufiks przekazany.
  {                                                                           //
    strncat(buf, s, sizeof(buf) - strlen(buf) - 1);                           // Dopisz sufiks do bufora (bez przepełnienia).
  }                                                                           //
  OLED_DrawString((uint8_t)x, (uint8_t)y, buf, NULL);                         // Wyświetl string na wyświetlaczu.
}

void OLED_PrintFloatDef(int x, int y, float v, const char *s)                 //---------------------------------------------------------------
{                                                                             //----------- DOMYŚLNE WYŚWIETLENIE FLOAT (PREC = 3) ------------
                                                                              //---------------------------------------------------------------
  OLED_PrintFloat(x, y, v, 3, s);                                             // Wywołaj wersję z precyzją = 3.
}

void OLED_PrintDoubleDef(int x, int y, double v, const char *s)               //---------------------------------------------------------------
{                                                                             //----------- DOMYŚLNE WYŚWIETLENIE DOUBLE (PREC = 3) -----------
                                                                              //---------------------------------------------------------------
  OLED_PrintDouble(x, y, v, 3, s);                                            // Wywołaj wersję z precyzją = 3.
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------- RYSOWANIE BITMAPY / SYMBOLU Z TABLICY C ----------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void OLED_BitmCols(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *d)
{
  if (!d) return;

  uint8_t bytes_per_col = (uint8_t)((h + 7U) / 8U);   // 1 dla h<=8, 2 dla 9..16 itd.

  for (uint8_t col = 0; col < w; col++)
  {
    for (uint8_t row = 0; row < h; row++)
    {
      uint8_t px = (uint8_t)(x + col);
      uint8_t py = (uint8_t)(y + row);

      if (px >= OLED_WIDTH || py >= OLED_HEIGHT) continue;

      uint8_t page_index = (uint8_t)(row / 8U);
      uint8_t bit_index  = (uint8_t)(row % 8U);
      uint8_t b = d[col * bytes_per_col + page_index];

      if (b & (1U << bit_index))
      {
        OLED_DrawPixel(px, py, 1);   // ON
      }
      else
      {
        OLED_DrawPixel(px, py, 0);   // OFF (czyści tło)
      }
    }
  }
}

void OLED_Symbol(uint8_t x, uint8_t y, const OLED_Symbol_t *s, const char *c)
{
  if (!s || !s->data) return;

  uint8_t pos_x = x;
  uint8_t pos_y = y;

  uint8_t w = s->width;                                                     // Szerokość symbolu.
  uint8_t h = s->height;                                                    // Wysokość symbolu.

  //---------------------------------------------------------------------------
  //  INTERPRETACJA PARAMETRU "center" (tak samo jak przy stringu)
  //---------------------------------------------------------------------------
  if (c && c[0] == 'C')
  {
    int center_x_mode        = 0;
    int center_y_mode_screen = 0;
    int center_y_mode_page   = 0;
    uint8_t page_index       = 0;

    if (c[1] == 'X')
    {
      center_x_mode = 1;                                                      // "CX" lub "CXY"
      if (c[2] == 'Y' && c[3] == '\0')                              // "CXY"
      {
        center_y_mode_screen = 1;
      }
    }
    else if (c[1] == 'Y' && c[2] == '\0')                           // "CY"
    {
      center_y_mode_screen = 1;
    }
    else if (c[1] >= '1' && c[1] <= '8' && c[2] == '\0')       // "C1".."C8"
    {
      center_x_mode      = 1;                                                 // C1..C8 centrowanie po X.
      center_y_mode_page = 1;                                                 // Y w obrębie strony.
      page_index         = (uint8_t)(c[1] - '1');                        // '1'->0, '2'->1, ..., '8'->7.
    }

    //----------------------------------------------------------------------
    //  CENTROWANIE PO X (globalne)
    //----------------------------------------------------------------------
    if (center_x_mode)
    {
      uint8_t cx;
      if (w >= OLED_WIDTH)                                                    // Symbol szerszy niż ekran.
      {
        cx = 0;
      }
      else
      {
        cx = (uint8_t)((OLED_WIDTH - w) / 2U);                                // Środek ekranu.
      }
      if (x == 0)
      {
        pos_x = cx;                                                           // Czyste centrowanie.
      }
      else
      {
        pos_x = (uint8_t)(cx + x);                                            // Środek + offset X.
      }
    }

    //----------------------------------------------------------------------
    //  CENTROWANIE PO Y – cały ekran
    //----------------------------------------------------------------------
    if (center_y_mode_screen)
    {
      uint8_t cy;
      if (h >= OLED_HEIGHT)                                                   // Za wysoki symbol -> od góry.
      {
        cy = 0;
      }
      else
      {
        cy = (uint8_t)((OLED_HEIGHT - h) / 2U);                               // Środek ekranu w pionie.
      }
      if (y == 0)
      {
        pos_y = cy;
      }
      else
      {
        pos_y = (uint8_t)(cy + y);
      }
    }

    //----------------------------------------------------------------------
    //  CENTROWANIE PO Y – w obrębie strony (C1..C8)
    //----------------------------------------------------------------------
    if (center_y_mode_page)
    {
      uint8_t page_y0 = (uint8_t)(page_index * 8U);                           // Początek strony w Y.
      uint8_t cy;

      if (h >= 8U)                                                            // Symbol wyższy/equal strony.
      {
        cy = page_y0;                                                         // Rysuj od góry strony.
      }
      else
      {
        uint8_t diff = (uint8_t)((8U - h) / 2U);                              // Wycentrowanie w 8 px.
        cy = (uint8_t)(page_y0 + diff);
      }
      if (y == 0)
      {
        pos_y = cy;
      }
      else
      {
        pos_y = (uint8_t)(cy + y);
      }
    }
  }

  //---------------------------------------------------------------------------
  //  RYSOWANIE SYMBOLU W WYLICZONYM MIEJSCU
  //---------------------------------------------------------------------------
  OLED_BitmCols(pos_x, pos_y, w, h, s->data);
}

void OLED_Line3D(int x0, int y0, int x1, int y1)
{
  int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
  int sx = (x0 < x1) ? 1 : -1;
  int dy = (y1 > y0) ? (y0 - y1) : (y1 - y0);                                 // Ujemne |dy|.
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx + dy;

  for (;;)
  {
    if (x0 >= 0 && x0 < (int)OLED_WIDTH && y0 >= 0 && y0 < (int)OLED_HEIGHT)
    {
      OLED_DrawPixel((uint8_t)x0, (uint8_t)y0, 1);
    }

    if (x0 == x1 && y0 == y1) break;

    int e2 = err << 1;

    if (e2 >= dy)
    {
      err += dy;
      x0  += sx;
    }
    if (e2 <= dx)
    {
      err += dx;
      y0  += sy;
    }
  }
}

void OLED_Cube3D(float ky, float kx, int16_t x, int16_t y, float s, const char *c)
{
  const float dist = 6.0f;                                                   // trochę dalej kamera
  float scale      = s;                                                   // rozmiar kostki podawany w parametrach

  float sinA = sinf(ky);
  float cosA = cosf(ky);
  float sinB = sinf(kx);
  float cosB = cosf(kx);

  int16_t tmp_x[8];
  int16_t tmp_y[8];

  //---------------------------------------------------------------------------
  //  PIERWSZE PRZEJŚCIE – PROJEKCJA WOKÓŁ (0,0) I WYZNACZENIE BOUNDING BOX
  //---------------------------------------------------------------------------
  int16_t minX =  32767;
  int16_t maxX = -32768;
  int16_t minY =  32767;
  int16_t maxY = -32768;

  for (uint8_t i = 0; i < 8; i++)
  {
    float x0 = g_cube_vertices[i].x;
    float y0 = g_cube_vertices[i].y;
    float z0 = g_cube_vertices[i].z;

    // obrót wokół osi Y
    float x1 =  x0 * cosA + z0 * sinA;
    float z1 = -x0 * sinA + z0 * cosA;

    // obrót wokół osi X
    float y2 =  y0 * cosB - z1 * sinB;
    float z2 =  y0 * sinB + z1 * cosB;

    float denom = (z2 + dist);
    if (denom < 0.5f) denom = 0.5f;                                          // trzymamy się z dala od kamery

    float inv = 1.0f / denom;
    float sx  = scale * x1 * inv;                                            // na razie wokół (0,0)
    float sy  = -scale * y2 * inv;

    int16_t ix = (int16_t)sx;
    int16_t iy = (int16_t)sy;

    tmp_x[i] = ix;
    tmp_y[i] = iy;

    if (ix < minX) minX = ix;
    if (ix > maxX) maxX = ix;
    if (iy < minY) minY = iy;
    if (iy > maxY) maxY = iy;
  }

  int16_t obj_w = (int16_t)(maxX - minX + 1);
  int16_t obj_h = (int16_t)(maxY - minY + 1);

  //---------------------------------------------------------------------------
  //  INTERPRETACJA PARAMETRU "center" (jak w DrawString / DrawLine)
  //---------------------------------------------------------------------------
  int16_t pos_x = x;                                                         // domyślnie: rysujemy od (x,y)
  int16_t pos_y = y;

  if (c && c[0] == 'C')
  {
    int center_x_mode        = 0;
    int center_y_mode_screen = 0;
    int center_y_mode_page   = 0;
    uint8_t page_index       = 0;                                            // 0..7 dla stron 1..8

    if (c[1] == 'X')
    {
      center_x_mode = 1;                                                     // "CX" lub "CXY"
      if (c[2] == 'Y')                                                  // "CXY"
        center_y_mode_screen = 1;
    }
    else if (c[1] == 'Y' && c[2] == '\0')                          // "CY"
    {
      center_y_mode_screen = 1;
    }
    else if (c[1] >= '1' && c[1] <= '8' && c[2] == '\0')      // "C1".."C8"
    {
      center_x_mode      = 1;
      center_y_mode_page = 1;
      page_index         = (uint8_t)(c[1] - '1');                       // '1'->0, ..., '8'->7
    }
    else if (c[1] == 'X' && c[2] == 'Y' && c[3] == '\0')      // "CXY"
    {
      center_x_mode        = 1;
      center_y_mode_screen = 1;
    }

    //----------------------------------------------------------------------
    //  CENTROWANIE PO X (CAŁY EKRAN)
    //----------------------------------------------------------------------
    if (center_x_mode)
    {
      int16_t cx;
      if (obj_w >= (int16_t)OLED_WIDTH)
      {
        cx = 0;
      }
      else
      {
        cx = (int16_t)((OLED_WIDTH - obj_w) / 2);
      }

      if (x == 0)
      {
        pos_x = cx;
      }
      
      {
        pos_x = (int16_t)(cx + x);
      }
    }

    //----------------------------------------------------------------------
    //  CENTROWANIE PO Y – CAŁY EKRAN
    //----------------------------------------------------------------------
    if (center_y_mode_screen)
    {
      int16_t cy;
      if (obj_h >= (int16_t)OLED_HEIGHT)
      {
        cy = 0;
      }
      else
      {
        cy = (int16_t)((OLED_HEIGHT - obj_h) / 2);
      }

      if (y == 0)
      {
        pos_y = cy;
      }
      else
      {
        pos_y = (int16_t)(cy + y);
      }
    }

    //----------------------------------------------------------------------
    //  CENTROWANIE PO Y – W OBRĘBIE STRONY (C1..C8)
    //----------------------------------------------------------------------
    if (center_y_mode_page)
    {
      int16_t page_y0 = (int16_t)(page_index * 8);

      int16_t cy;
      if (obj_h >= 8)
      {
        cy = page_y0;
      }
      else
      {
        int16_t diff = (int16_t)((8 - obj_h) / 2);
        cy = (int16_t)(page_y0 + diff);
      }

      if (y == 0)
      {
        pos_y = cy;
      }
      else
      {
        pos_y = (int16_t)(cy + y);
      }
    }
  }

  //---------------------------------------------------------------------------
  //  PRZESUNIĘCIE WSZYSTKICH WIERZCHOŁKÓW TAK, ABY BOUNDING BOX = (pos_x,pos_y)
  //---------------------------------------------------------------------------
  int16_t dx = (int16_t)(pos_x - minX);
  int16_t dy = (int16_t)(pos_y - minY);

  int16_t proj_x[8];
  int16_t proj_y[8];

  for (uint8_t i = 0; i < 8; i++)
  {
    proj_x[i] = (int16_t)(tmp_x[i] + dx);
    proj_y[i] = (int16_t)(tmp_y[i] + dy);
  }

  //---------------------------------------------------------------------------
  //  RYSOWANIE KRAWĘDZI KOSTKI
  //---------------------------------------------------------------------------
  for (uint8_t e = 0; e < 12; e++)
  {
    uint8_t i0 = g_cube_edges[e][0];
    uint8_t i1 = g_cube_edges[e][1];
    OLED_Line3D(proj_x[i0], proj_y[i0], proj_x[i1], proj_y[i1]);
  }
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------- RYSOWANIE WYKRESU W CZASIE --------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void OLED_ScopeInit(OLED_Scope_t *s, int16_t x, int16_t y, int16_t w, int16_t h, float min_val, float max_val, uint32_t time_window_ms)
{
  s->x = x;
  s->y = y;
  s->w = w;
  s->h = h;

  s->min_val = min_val;
  s->max_val = max_val;

  s->time_window_ms = time_window_ms;
  s->pixel_dt_ms    = (w > 0) ? (time_window_ms / (uint32_t)w) : 1U;

  s->last_pixel_ms  = 0;
  s->write_idx      = 0;

  for (int i = 0; i < 128; i++)
  {
    s->samples[i] = (uint8_t)(h - 1);    // Domyślnie linia na dole
  }
}

void OLED_ScopeAddSample(OLED_Scope_t *s, float value, uint32_t now_ms)
{
  if (s->w <= 0 || s->h <= 0) return;

  // Za mało czasu – nie przesuwamy jeszcze w osi X
  if ((now_ms - s->last_pixel_ms) < s->pixel_dt_ms) return;

  s->last_pixel_ms = now_ms;

  // Ogranicz wartość do zakresu
  if (value < s->min_val) value = s->min_val;
  if (value > s->max_val) value = s->max_val;

  float norm = (value - s->min_val) / (s->max_val - s->min_val); // 0..1
  if (norm < 0.0f) norm = 0.0f;
  if (norm > 1.0f) norm = 1.0f;

  // Zamiana na wysokość piksela (0..h-1, 0 = góra)
  uint8_t y_pix = (uint8_t)((1.0f - norm) * (float)(s->h - 1));

  if (s->write_idx >= s->w) s->write_idx = 0;
  s->samples[s->write_idx] = y_pix;
  s->write_idx++;
}

void OLED_ScopeDraw(const OLED_Scope_t *s, int16_t x, int16_t y, int16_t w, int16_t h)
{
  if (!s) return;                                                             // Brak obiektu wykresu – nic nie rób.
  if (w <= 1 || h <= 1) return;                                               // Za małe okno – nie rysujemy.

  if (w > s->w) w = s->w;                                                     // Nie rysuj szerzej niż mamy próbek.
  if (h > s->h) h = s->h;                                                     // Wysokość ogranicz do wysokości logicznej wykresu.

  // Opcjonalnie ramka wokół wykresu – jak nie chcesz, wywal tę linię:
  OLED_Rect(x, y, w, h, 1);

  // write_idx wskazuje MIEJSCE na następną próbkę.
  // Najnowsza próbka jest pod indeksem (write_idx - 1 + s->w) % s->w.
  // Żeby wykres był „płynący z lewej do prawej”, zaczynamy od najstarszej:
  int16_t start_idx = s->write_idx;                                           // Tu jest NAJSTARSZA próbka.

  int16_t prev_px = -1;
  int16_t prev_py = 0;

  for (int16_t i = 0; i < w; i++)
  {
    int16_t buf_idx = (start_idx + i) % s->w;                                 // Indeks w buforze próbek.
    uint8_t y_pix   = s->samples[buf_idx];                                    // 0..(s->h - 1), 0 = góra okna wykresu.

    if (y_pix >= h) y_pix = (uint8_t)(h - 1);                                 // Na wszelki wypadek przytnij do okna.

    int16_t px = x + i;                                                       // X na ekranie.
    int16_t py = y + y_pix;                                                   // Y na ekranie (przesunięcie o początek okna).

    if (px < 0 || px >= (int16_t)OLED_WIDTH) continue;                        // Prosty clipping po X.
    if (py < 0 || py >= (int16_t)OLED_HEIGHT) continue;                       // Prosty clipping po Y.

    if (prev_px >= 0)                                                         // Jeśli mamy poprzedni punkt – połącz linią.
    {
      OLED_Line(prev_px, prev_py, px, py, 1);                              // Cienka linia między próbkami.
    }
    else                                                                      // Pierwszy punkt – tylko pojedynczy piksel.
    {
      OLED_DrawPixel((uint8_t)px, (uint8_t)py, 1);
    }

    prev_px = px;
    prev_py = py;
  }
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------- PRZEŁĄCZAJ STRONY WYŚWIETLACZA ZALEŻNIE OD CZASU ---------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void OLED_AnimacjaPrawo()
{
  if (P > 0)                                                                // Jeśli przesunięcie P jest dodatnie:
  {                                                                         //
    P = P - 4;                                                              // Zmniejsz przesunięcie w osi X o 4 piksele.
    P1 = P1 - 2;                                                            // Zmniejsz przesunięcie w osi Y P1 o 2 piksele.
  }
}