#ifndef OLED_H
#define OLED_H

#include "stm32f7xx.h"
#include "system_stm32f7xx.h"
#include "Zegar.h"
#include "Math.h"
#include "string.h"
#include "stdint.h"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------- USTAWIENIA OGÓLNE WYŚWIETLACZA OLED - MAKRA --------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#define OLED_I2C          I2C1                                                // Używany kontroler I2C.
#define OLED_I2C_ADDR     0x3C                                                // 7-bitowy adres OLED (0x3C lub 0x3D).
#define OLED_CTRL_CMD     0x00                                                // Co = 0, D/C# = 0 -> bajt sterujący „komenda”.
#define OLED_CTRL_DATA    0x40                                                // Co = 0, D/C# = 1 -> bajt sterujący „dane”.
#define OLED_WIDTH        128                                                 // Rozdzielczość pozioma wyświetlacza [px].
#define OLED_HEIGHT       64                                                  // Rozdzielczość pionowa wyświetlacza [px].
#define OLED_PAGES        (OLED_HEIGHT / 8)                                   // Liczba stron RAM w SSD1306 (1 strona = 8 pikseli w pionie).
#define M_PI              3.14159265358979323846                              // Stała π z dużą precyzją (jeśli nie ma jej w Math.h).
#define TWO_PI            (2.0f * (float)M_PI)                                // 2π, wygodne do obrotów / animacji.


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------- UPROSZCZONE WYŚWIETLANIE WARTOŚCI ZMIENNYCH - MAKRO ----------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---- Makro typu printf, zależnie od argumentu wybierz funkcję: całkowita / nieujemna / float / double. x, y – pozycja tekstu na ekranie, suffix – dopisek, np. " V" -----
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#define OLED_PrintValue(x, y, val, suffix) \
  _Generic((val), \
    signed char:         OLED_PrintInt, \
    short:               OLED_PrintInt, \
    int:                 OLED_PrintInt, \
    long:                OLED_PrintInt, \
    long long:           OLED_PrintInt, \
    unsigned char:       OLED_PrintUInt, \
    unsigned short:      OLED_PrintUInt, \
    unsigned int:        OLED_PrintUInt, \
    unsigned long:       OLED_PrintUInt, \
    unsigned long long:  OLED_PrintUInt, \
    float:               OLED_PrintFloatDef, \
    double:              OLED_PrintDoubleDef \
  )(x, y, val, suffix)


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------- STRUKTURA SYSTEMU CZCIONEK -----------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-- Opis czcionki używanej w funkcjach tekstowych OLED_. Dane trzymane w osobnych plikach, a tutaj tylko opis: rozmiar, zakres znaków, wskaźniki na tablice z bitmapami --
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef struct                                                                // Definicja typu struktury opisującej parametry czcionki oraz jej dane bitmapowe.
{                                                                             // (to opis „jak wygląda” pojedynczy obiekt typu OLED_Font_t – nie tworzy jeszcze obiektu).
  uint8_t        width;                                                       //   Maksymalna szerokość pojedynczego znaku w pikselach.
  uint8_t        height;                                                      //   Wysokość znaku w pikselach (liczba wierszy).
  uint8_t        first_char;                                                  //   Kod ASCII pierwszego obsługiwanego znaku w tablicy fontu.
  uint8_t        last_char;                                                   //   Kod ASCII ostatniego obsługiwanego znaku w tablicy fontu.
  const uint8_t *data;                                                        //   Wskaźnik na surowe dane glifów (tablica bajtów z bitmapami znaków).
  const uint8_t *widths;                                                      //   Wskaźnik na tablicę indywidualnych szerokości znaków lub NULL (gdy szerokość stała).
  uint8_t        letter_spacing;                                              //   Odstęp (kerning) między znakami w pikselach.
} OLED_Font_t;                                                                // Alias typu: od teraz używamy nazwy OLED_Font_t jako typu czcionki.
                                                                              // Uwaga: to jest definicja *typu* (struktury), a nie definicja konkretnej czcionki.
                                                                              //        Sama w sobie nie rezerwuje pamięci na żaden font – tylko opisuje jego strukturę.

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------- DEKLARACJE STAŁYCH OBIEKTÓW TYPU OLED_Font_t – KONKRETNE CZCIONKI ZDEFINIOWANE W INNYM PLIKU .C ---------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern const OLED_Font_t Font_5x7;                                            // Deklaracja stałego obiektu czcionki 5x7.
                                                                              // Informuje kompilator, że gdzieś w innym pliku .c znajduje się:
                                                                              //   const OLED_Font_t Font_5x7;   // (definicja obiektu, rezerwacja pamięci + inicjalizacja)

extern const OLED_Font_t Font_8x8;                                            // Deklaracja stałego obiektu czcionki 8x8.
                                                                              // Typ: OLED_Font_t, kwalifikator const – tylko do odczytu (np. w FLASH).

extern const OLED_Font_t Font_BankGothic_Md_BT_13px;                          // Deklaracja stałego obiektu czcionki BankGothic 13 px.


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------- STRUKTURA SYSTEMU SYMBOLI -----------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------- Symbol (ikona) rysowany z gotowej bitmapy. Dane są ułożone kolumnami - zgodnie z organizacją pamięci SSD1306 ------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef struct
{
  uint8_t        width;                                                       // Szerokość symbolu w pikselach.
  uint8_t        height;                                                      // Wysokość symbolu w pikselach.
  const uint8_t *data;                                                        // Dane: Kolumny, LSB = górny piksel.
} OLED_Symbol_t;

extern const OLED_Symbol_t Symbol_Termo_Pelny;                                // Deklaracja symbolu Symbol_Termo_Pelny.
extern const OLED_Symbol_t Symbol_Termo_Pusty;                                // Deklaracja symbolu Symbol_Termo_Pusty.
extern const OLED_Symbol_t Symbol_Instagram;                                  // Deklaracja symbolu Symbol_Instagram.
extern const OLED_Symbol_t Symbol_MTOPrograms;                                // Deklaracja symbolu Symbol_MTOPrograms.
extern const OLED_Symbol_t Symbol_Facebook;                                   // Deklaracja symbolu Symbol_Facebook.
extern const OLED_Symbol_t Symbol_YouTube;                                    // Deklaracja symbolu Symbol_YouTube.
extern const OLED_Symbol_t Symbol_VSCode;                                     // Deklaracja symbolu Symbol_VSCode.
extern const OLED_Symbol_t Symbol_Stopien;                                    // Deklaracja symbolu Symbol_Stopien.
extern const OLED_Symbol_t Discord_Symbol;                                    // Deklaracja symbolu Discord_Symbol.
extern const OLED_Symbol_t Symbol_GPT;                                        // Deklaracja symbolu Symbol_GPT.
extern const OLED_Symbol_t Symbol_ST;                                         // Deklaracja symbolu Symbol_ST.


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------- STRUKTURA OBIEKTU 3D - SZEŚCIAN --------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------- Prosty model kostki 3D używany w animacji obracającego się sześcianu --------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef struct                                                                // Definicja nowego typu danych – struktury 3D.
{                                                                             //
  float x;                                                                    //   Składowa X jednego punktu w przestrzeni 3D.
  float y;                                                                    //   Składowa Y jednego punktu w przestrzeni 3D.
  float z;                                                                    //   Składowa Z jednego punktu w przestrzeni 3D.
} Vec3;                                                                       // Nazwa typu: Vec3 (wektor 3-wymiarowy).

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------ Tablica stałych obiektów typu Vec3 – wierzchołki jednostkowego sześcianu (-1..+1 w każdej osi) -----------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static const Vec3 g_cube_vertices[8] =                                        // Tablica 8 struktur Vec3 – każdy element = jeden wierzchołek kostki.
{                                                                             //
  {-1.0f, -1.0f, -1.0f},                                                      //   0: lewy  dół, przód   (X-, Y-, Z-)
  { 1.0f, -1.0f, -1.0f},                                                      //   1: prawy dół, przód   (X+, Y-, Z-)
  { 1.0f,  1.0f, -1.0f},                                                      //   2: prawy góra, przód  (X+, Y+, Z-)
  {-1.0f,  1.0f, -1.0f},                                                      //   3: lewy  góra, przód  (X-, Y+, Z-)
  {-1.0f, -1.0f,  1.0f},                                                      //   4: lewy  dół, tył     (X-, Y-, Z+)
  { 1.0f, -1.0f,  1.0f},                                                      //   5: prawy dół, tył     (X+, Y-, Z+)
  { 1.0f,  1.0f,  1.0f},                                                      //   6: prawy góra, tył    (X+, Y+, Z+)
  {-1.0f,  1.0f,  1.0f}                                                       //   7: lewy  góra, tył    (X-, Y+, Z+)
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------ Lista krawędzi sześcianu – każda para zawiera indeksy dwóch wierzchołków, które łączy linia --------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static const uint8_t g_cube_edges[12][2] =                                    // Tablica 12 krawędzi; każda krawędź = dwa indeksy z g_cube_vertices.
{                                                                             //
  {0, 1}, {1, 2}, {2, 3}, {3, 0},                                             //   Przednia ściana (front) – 4 krawędzie.
  {4, 5}, {5, 6}, {6, 7}, {7, 4},                                             //   Tylna  ściana (back)  – 4 krawędzie.
  {0, 4}, {1, 5}, {2, 6}, {3, 7}                                              //   Połączenia pionowe między frontem a tyłem – 4 krawędzie.
};


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------- STRUKTURA WYKRESU W CZASIE -----------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//--------------- Prosty oscyloskop „w czasie rzeczywistym” rysowany na OLED. Próbki są skalowane do wysokości h i przesuwane w prawo w miarę upływu czasu ----------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef struct                                                                // Definicja *kształtu* struktury oraz jednocześnie deklaracja tej struktury.
{                                                                             // (od tego miejsca kompilator wie, że struktura ma poniższe pola w takiej kolejności).
  int16_t   x;                                                                //   Pozycja X lewego górnego rogu obszaru wykresu.
  int16_t   y;                                                                //   Pozycja Y lewego górnego rogu obszaru wykresu.
  int16_t   w;                                                                //   Szerokość wykresu w pikselach (liczba kolumn).
  int16_t   h;                                                                //   Wysokość wykresu w pikselach (skalowanie osi Y).
  float     min_val;                                                          //   Minimalna wartość sygnału (np. 0.0 V) – zakres osi Y.
  float     max_val;                                                          //   Maksymalna wartość sygnału (np. 5.0 V).
  uint32_t  time_window_ms;                                                   //   Całkowita szerokość okna czasowego wykresu w ms (np. 2000 ms = 2 s).
  uint32_t  last_pixel_ms;                                                    //   Znacznik czasu, kiedy został dodany ostatni nowy piksel na wykresie.
  uint32_t  pixel_dt_ms;                                                      //   Ile ms odpowiada jednemu pikselowi w osi X (gęstość próbkowania na ekranie).
  uint8_t   write_idx;                                                        //   Indeks kolumny (0..w-1), w której zapisywana będzie następna próbka.
  uint8_t   samples[128];                                                     //   Bufor próbek przeskalowanych do wysokości 0..h-1 (maks. 128 kolumn wykresu).
} OLED_Scope_t;                                                               // Nadanie aliasu typu: od teraz można pisać OLED_Scope_t zamiast „struct { ... }”.
                                                                              // Uwaga: to jest *definicja typu*, a nie definicja zmiennej – nie rezerwuje pamięci
                                                                              //        na żaden konkretny obiekt, tylko opisuje jego strukturę i nazwę typu.

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------- DEKLARACJA ZMIENNEJ GLOBALNEJ TYPU OLED_Scope_t – PRAWIDZIWA DEFINICJA BĘDZIE W PLIKU .C -----------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern OLED_Scope_t Wykres_Napiecia;                                          // Deklaracja zewnętrzna zmiennej globalnej.
                                                                              // Mówi kompilatorowi: „gdzieś w innym pliku .c istnieje obiekt
                                                                              //  o nazwie Wykres_Napiecia i typie OLED_Scope_t”.
                                                                              // To NIE jest definicja – nie tworzy obiektu, nie zajmuje pamięci.
                                                                              // Prawdziwa definicja musi się znaleźć raz w jednym pliku .c, np.:
                                                                              //
                                                                              //   OLED_Scope_t Wykres_Napiecia;   // <- definicja (bez 'extern')
                                                                              //
                                                                              // Wtedy:
                                                                              //   - w pliku .c z powyższą linią tworzony jest obiekt (rezerwuje się pamięć),
                                                                              //   - w innych plikach .c używamy 'extern OLED_Scope_t Wykres_Napiecia;'
                                                                              //     aby mieć do niego dostęp (tylko deklaracja, bez duplikowania definicji).


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------- DEKLARACJA ZMIENNYCH GLOBALNYCH --------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern uint8_t P;
extern uint8_t P1;


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------- DEKLARACJA FUNKCJI GLOBALNYCH ---------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------- KONTROLA WYŚWIETLACZA ------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
void OLED_Inicjalizacja(void);                                                // Inicjalizacja kontrolera OLED (zakłada skonfigurowane I2C)
void OLED_Clear(void);                                                        // Wyczyść cały ekran (wszystkie piksele = 0)
void OLED_ClearPage(uint8_t page);                                            // Wyczyść jedną stronę (page = 0..7)
void OLED_BufferClear(void);                                                  // Wyczyść bufor RAM (bez wysyłania do wyświetlacza)
void OLED_UpdateScreen(void);                                                 // Wyślij cały bufor do wyświetlacza

//----------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------- LINIE --------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
void OLED_HLine(uint8_t x0, uint8_t x1, uint8_t y, uint8_t color);            // Rysuj poziomą linię od x0 do x1 na wskazanej wysokości y.
void OLED_VLine(uint8_t x, uint8_t y0, uint8_t y1, uint8_t color);            // Rysuj pionową linię od y0 do y1 w kolumnie x.
void OLED_Line(int x0, int y0, int x1, int y1, uint8_t color);                // Rysuj linię między punktami (x0,y0) i (x1,y1).


//----------------------------------------------------------------------------------------------------------------------
//------------------------------------------------- PROSTOKĄT - OBRYS --------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//  x, y      – Współrzędne lewego górnego rogu prostokąta w pikselach.
//  w         – Szerokość prostokąta w pikselach (liczona w poziomie od x).
//  h         – Wysokość  prostokąta w pikselach (liczona w pionie od y).
//  c         – Kolor piksela: 0 = zgaś, 1 = zapal.
void OLED_Rect(int x, int y, int w, int h, uint8_t c);


//----------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------- KOŁO - OBRYS ----------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//  x, y        – Współrzędne środka okręgu w pikselach.
//  r           – Promień okręgu w pikselach (r > 0).
//  c           – Kolor piksela: 0 = zgaś, 1 = zapal.
void OLED_Circle(int x, int y, int r, uint8_t c);


//----------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------- TRÓJKĄT - OBRYS ---------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//  x1, y1        – Współrzędne pierwszego wierzchołka trójkąta w pikselach.
//  x2, y2        – Współrzędne drugiego wierzchołka trójkąta w pikselach.
//  x3, y3        – Współrzędne trzeciego wierzchołka trójkąta w pikselach.
//  c             – Kolor piksela: 0 = zgaś, 1 = zapal.
void OLED_Triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint8_t c);


//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------- PROSTOKĄT – WYPEŁNIONY -----------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//  x, y          – Współrzędne lewego górnego rogu prostokąta w pikselach.
//  w             – Szerokość prostokąta w pikselach (liczona w poziomie od x).
//  h             – Wysokość  prostokąta w pikselach (liczona w pionie od y).
//  p             – Poziom wypełnienia w procentach (0–100).
//  d             – Kierunek wypełniania prostokąta:
//                  'L' / 'l' – wypełnianie od lewej, 'R' / 'r' – wypełnianie od prawej.
//                  'T' / 't' – wypełnianie od góry,  'B' / 'b' - wypełnianie od dołu.
//  c             – Kolor piksela: 0 = zgaś, 1 = zapal.
void OLED_FillRect(int x, int y, int w, int h, uint8_t p, char d, uint8_t c);


//----------------------------------------------------------------------------------------------------------------------
//------------------------------------------------- KOŁO – WYPEŁNIONY --------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//  x, y            – Współrzędne środka okręgu w pikselach.
//  r               – Promień okręgu w pikselach (r > 0).
//  p               – Poziom wypełnienia w procentach (0–100).
//  d               – Kierunek wypełniania koła względem bounding-boxa:
//                    'L' / 'l' – wypełnianie od lewej, 'R' / 'r' – wypełnianie od prawej.
//                    'T' / 't' – wypełnianie od góry,  'B' / 'b' - wypełnianie od dołu.
//  c               – Kolor piksela: 0 = zgaś, 1 = zapal.
void OLED_FillCircle(int x, int y, int r, uint8_t p, char d, uint8_t c);


//----------------------------------------------------------------------------------------------------------------------
//------------------------------------------------ TRÓJKĄT – WYPEŁNIONY ------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//  x1, y1            – Współrzędne pierwszego wierzchołka trójkąta w pikselach.
//  x2, y2            – Współrzędne drugiego wierzchołka trójkąta w pikselach.
//  x3, y3            – Współrzędne trzeciego wierzchołka trójkąta w pikselach.
//  p                 – Poziom wypełnienia w procentach (0–100).
//  d                 – Kierunek wypełniania trójkąta względem bounding-boxa:
//                      'L' / 'l' – wypełnianie od lewej, 'R' / 'r' – wypełnianie od prawej.
//                      'T' / 't' – wypełnianie od góry,  'B' / 'b' - wypełnianie od dołu.
//  c                 – Kolor piksela: 0 = zgaś, 1 = zapal.
void OLED_FillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint8_t p, char d, uint8_t c);


//----------------------------------------------------------------------------------------------------------------------
//--------------------------------------------- WYCINEK KOŁA – WYPEŁNIONY ----------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//  x, y               – Współrzędne środka koła w pikselach.
//  r                  – Promień koła w pikselach (r > 0).
//  p                  – Poziom wypełnienia sektora w procentach (0–100).
//  k                  – Kąt początkowy sektora w stopniach (0–359).
//  d                  – Kierunek narastania sektora:
//                       'C' / 'c' – zgodnie z ruchem wskazówek zegara (Clockwise),
//                       'A' / 'a' lub inny znak – przeciwnie do ruchu wskazówek zegara (Counter-Clockwise).
//  c                  – Kolor piksela: 0 = zgaś, 1 = zapal.
void OLED_FillCirclePie(int x, int y, int r, uint8_t p, int k, char d, uint8_t c);


//----------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------- PIXELE I TEKST ---------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t c);                         // Rysuj pojedynczy piksel w buforze ekranu w podanych współrzędnych.
void OLED_DrawChar(uint8_t x, uint8_t y, char c);                             // Rysuj pojedynczy znak przy użyciu aktualnego fontu.
void OLED_DrawString(uint8_t x, uint8_t y, const char *txt, const char *c);   // Rysuj łańcuch znaków od pozycji (x,y) z opcjonalnym wyrównaniem (centrowaniem).
uint8_t OLED_GetCenteredX(const char *txt);                                   // Zwróć X tak, aby tekst był wyśrodkowany w poziomie.
uint8_t OLED_GetCenteredY(void);                                              // Zwraóć Y tak, aby tekst był wyśrodkowany w pionie względem wysokości ekranu.
const OLED_Font_t* OLED_GetFont(void);                                        // Zwraca wskaźnik do aktualnie ustawionej czcionki.
void OLED_SetFont(const OLED_Font_t *font);                                   // Ustawia aktualnie używaną czcionkę do wyświetlania tekstu.
void OLED_PrintInt(int x, int y, int32_t v, const char *s);                   // Wypisuje liczbę całkowitą ze znakiem w pozycji (x,y) z opcjonalnym sufiksem.
void OLED_PrintUInt(int x, int y, uint32_t v, const char *s);                 // Wypisuje liczbę całkowitą bez znaku w pozycji (x,y) z opcjonalnym sufiksem.
void OLED_PrintFloat(int x, int y, float v, uint8_t p, const char *s);        // Wypisuje wartość float w pozycji (x,y) z podaną precyzją i opcjonalnym sufiksem.
void OLED_PrintDouble(int x, int y, double v, uint8_t p, const char *s);      // Wypisuje wartość double w pozycji (x,y) z podaną precyzją i opcjonalnym sufiksem.
void OLED_PrintFloatDef(int x, int y, float v, const char *s);                // Wypisuje float w pozycji (x,y) z domyślną precyzją i opcjonalnym sufiksem.
void OLED_PrintDoubleDef(int x, int y, double v, const char *s);              // Wypisuje double w pozycji (x,y) z domyślną precyzją i opcjonalnym sufiksem.


void OLED_BitmCols(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *d);   // Rysuj bitmapę o wymiarach w×h zapisaną kolumnami w buforze d w pozycji (x,y).
void OLED_Symbol(uint8_t x, uint8_t y, const OLED_Symbol_t *s, const char *c);      // Rysuj symbol opisany strukturą s w pozycji (x,y), opcjonalnie z podpisem c.
void OLED_Line3D(int x0, int y0, int x1, int y1);                                   // Rysuj odcinek linii (rzut 3D) między punktami.
void OLED_Cube3D(float ky, float kx, int16_t x, int16_t y, float s, const char *c); // Rysuj obracający się sześcian 3D o skali s w punkcie (x,y) z opisem c.


void OLED_ScopeInit(OLED_Scope_t *s, int16_t x, int16_t y, int16_t w, int16_t h, float min_val, float max_val, uint32_t time_window_ms);     // Inicjalizuje strukturę wykresu „oscyloskopu” w obszarze (x,y,w,h) z zakresem i oknem czasu.
void OLED_ScopeAddSample(OLED_Scope_t *s, float value, uint32_t now_ms);        // Dodaje nową próbkę value z czasem now_ms do bufora wykresu.
void OLED_ScopeDraw(const OLED_Scope_t *s, int16_t x, int16_t y, int16_t w, int16_t h); // Rysuje aktualny wykres s w zadanym prostokącie (x,y,w,h).

void OLED_AnimacjaPrawo(void);                                                  // Przełącza zawartość ekranu z animacją przesuwu w prawo.
#endif  
