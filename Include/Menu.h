#ifndef OLED_1_MENU_H_
#define OLED_1_MENU_H_

#include "ADC.h"
#include "OLED.h"

typedef enum
{
  LED_ZIELONA = 0,    // PB0
  LED_NIEBIESKA = 1,  // PB7
  LED_CZERWONA = 2,   // PB14
} LED_t;

#define LED_ZIELONA_ON()    (GPIOB -> BSRR = GPIO_BSRR_BS0)
#define LED_ZIELONA_OFF()   (GPIOB -> BSRR = GPIO_BSRR_BR0)
#define LED_NIEBIESKA_ON()  (GPIOB -> BSRR = GPIO_BSRR_BS7)
#define LED_NIEBIESKA_OFF() (GPIOB -> BSRR = GPIO_BSRR_BR7)
#define LED_CZERWONA_ON()   (GPIOB -> BSRR = GPIO_BSRR_BS14)
#define LED_CZERWONA_OFF()  (GPIOB -> BSRR = GPIO_BSRR_BR14)

extern uint8_t Menu_AktualnaStrona;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------- DEKLARACJA FUNKCJI GLOBALNYCH ---------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern void Menu_ZmianaStronyCzas(uint32_t Czas);                                         // Przełącz stronę po okreslonym czasie.
extern void Menu_UstawDiode(LED_t LED);                                                   // Włącz wskazaną diodę LED.
extern void Menu_Strona0(void);                                                           // Deklaracja funkcji Strona0().
extern void Menu_Strona1(void);                                                           // Deklaracja funkcji Strona1().
extern void Menu_Strona2(void);                                                           // Deklaracja funkcji Strona2().
extern void Menu_Strona3(void);                                                           // Deklaracja funkcji Strona3().
extern void Menu_Strona4(void);                                                           // Deklaracja funkcji Strona4().
extern void Menu_Strona5(void);                                                           // Deklaracja funkcji Strona5().
extern void Menu_Strona6(void);                                                           // Deklaracja funkcji Strona6().
extern void Menu_Strona7(void);                                                           // Deklaracja funkcji Strona7().
#endif
