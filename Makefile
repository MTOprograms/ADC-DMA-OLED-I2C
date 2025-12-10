#------------------------------------------------------------------------------------------------------------------------------------------------------------------------- 
#------------------------------------------------------------------------- Konfiguracja projektu -------------------------------------------------------------------------
#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-include Procesor.mk#                                                         # Dołącz plik z konfiguracją procesora, brak pliku nie powoduje błędu.

BUILD_ROOT = Build#                                                           # Główny katalog zawierający wynik kompilacji.
BUILD_DIR = $(BUILD_ROOT)/$(BUILD_TYPE)#                                      # Podkatalog budowania zależny od typu (Debug/Release).
SYSTEM_DIR = System/System#                                                   # Katalog z plikami systemowymi.
STARTUP_DIR = System/Startup#                                                 # Katalog z plikami startup.
LINKER_DIR = System/Linker#                                                   # Katalog z plikiem linkera.
SRC_DIR = Source#                                                             # Katalog z własnymi plikami źródłowymi C.
                                                                              # Katalogi ze ścieżkami nagłówków:
C_INCLUDES += -IInclude#                                                      #   Katalog z własnymi nagłówkami projektu.
C_INCLUDES += -ISystem/CMSIS/Device/ST/$(FAMILY)/Include#                     #   Katalog z nagłówkami rodziny MCU, np. STM32F7xx.
C_INCLUDES += -ISystem/CMSIS/Include#                                         #   Katalog z nagłówkami CMSIS.

OBJ_DIR = $(BUILD_DIR)/Obj#                                                   # Katalog na pliki obiektowe (.o).
DEP_DIR = $(BUILD_DIR)/Dep#                                                   # Katalog na pliki zależności (.d).
ASM_DIR = $(BUILD_DIR)/Asm#                                                   # Katalog na wygenerowane pliki asemblera (.s).
LST_DIR = $(BUILD_DIR)/List#                                                  # Katalog na listingi kompilacji (.lst).

#------------------------------------------------------------------------------------------------------------------------------------------------------------------------- 
#---------------------------------------------------------------------------- Pliki źródłowe -----------------------------------------------------------------------------
#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
C_SRC   = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SYSTEM_DIR)/*.c)#           # Wszystkie pliki C z katalogu Source oraz System\System.
ASM_SRC = $(STARTUP_DIR)/$(STARTUP_FILE)#                                     # Plik/plik startupowy assemblera.

#------------------------------------------------------------------------------------------------------------------------------------------------------------------------- 
#-------------------------------------------------------------------- Flagi budowania (Debug/Release) --------------------------------------------------------------------
#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
C_DEFS = -D$(DEVICE)#                                                         # Podstawowa definicja z nazwą urządzenia (np. -DSTM32F767xx).

ifeq ($(BUILD_TYPE),Debug)#                                                   # Gałąź wykonywana, gdy BUILD_TYPE == Debug.
    C_DEFS += -DDEBUG#                                                        #   Dodaj makro DEBUG dla kompilatora (np. włączanie dodatkowych assertów/logów).
    OPT = -Og#                                                                #   Opcje optymalizacji: -Og = optymalizacja przyjazna debugowaniu.
    DEBUG = 1#                                                                #   Flaga pomocnicza ustawiająca tryb debug dla późniejszych warunków.

else ifeq ($(BUILD_TYPE),Release)#                                            # Gałąź wykonywana, gdy BUILD_TYPE == Release.
    C_DEFS += -DNDEBUG#                                                       #   Dodaj makro NDEBUG (wyłącza np. standardowe assert() w C).
    OPT = -O2#                                                                #   Opcje optymalizacji: mocniejsza optymalizacja kodu pod Release.
    DEBUG = 0#                                                                #   Flaga pomocnicza ustawiająca tryb release dla późniejszych warunków.
endif

#------------------------------------------------------------------------------------------------------------------------------------------------------------------------- 
#------------------------------------------------------------------- Narzędzia kompilacji i konwersji --------------------------------------------------------------------
#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
PREFIX = arm-none-eabi-#                                                      # Prefiks dla narzędzi toolchainu ARM (gcc, objcopy, size itd.).
CC = $(PREFIX)gcc#                                                            # Kompilator C – arm-none-eabi-gcc.
AS = $(PREFIX)gcc -x assembler-with-cpp#                                      # Kompilator assemblera – gcc z przełącznikiem assembler-with-cpp (preprocesor C).
CP = $(PREFIX)objcopy#                                                        # Narzędzie do konwersji plików obiektowych (objcopy).
SZ = $(PREFIX)size#                                                           # Narzędzie do wyświetlania rozmiarów sekcji i całego programu.
HEX = $(CP) -O ihex#                                                          # Komenda do generowania pliku .hex z .elf (format Intel HEX).
BIN = $(CP) -O binary#                                                        # Komenda do generowania pliku .bin z .elf (surowy obraz binarny).

#------------------------------------------------------------------------------------------------------------------------------------------------------------------------- 
#-------------------------------------------------------------------- Flagi kompilatora i assemblera ---------------------------------------------------------------------
#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
CPU_FLAGS = -mcpu=$(CPU) -mthumb#                                             # Flagi CPU: ustawienie rdzenia (np. cortex-m7) i trybu Thumb.

ifeq ($(USE_FPU),1)#                                                          # Warunkowe ustawienie FPU – jeśli USE_FPU == 1, używamy sprzętowego FPU.
    FPU_FLAGS = -mfpu=fpv4-sp-d16 -mfloat-abi=hard#                           #   Ustaw FPU na fpv4-sp-d16 + ABI „hard” (przekazywanie floatów w rejestrach FPU).
else #                                                                        # W przeciwnym wypadku (brak FPU lub nie używamy) – konfigurujemy ABI soft.
    FPU_FLAGS = -mfloat-abi=soft#                                             #   ABI „soft” – operacje zmiennoprzecinkowe realizowane programowo, bez FPU.
endif#

MCU = $(CPU_FLAGS) $(FPU_FLAGS)#                                              # Połączone flagi CPU + FPU – gotowy zestaw opcji dla kompilatora i linkera.

                                                                              # Bazowe flagi kompilatora ASM:
ASFLAGS  = $(MCU)#                                                            #   Architektura CPU + FPU.
ASFLAGS += $(OPT)#                                                            #   Poziom optymalizacji (Debug/Release).
ASFLAGS += -Wall#                                                             #   Włącz wszystkie podstawowe ostrzeżenia.
ASFLAGS += -fdata-sections -ffunction-sections#                               #   Umieszczaj funkcje i dane w osobnych sekcjach.

                                                                              # Bazowe flagi kompilatora C:
CFLAGS  = $(MCU)#                                                             #   Architektura CPU + FPU.
CFLAGS += -std=gnu11#                                                         #   Standard języka C (GNU C11).
CFLAGS += --specs=nano.specs#                                                 #   Użycie „lekkiej” biblioteki standardowej (newlib-nano).
CFLAGS += $(C_DEFS)#                                                          #   Makra kompilatora (DEVICE, DEBUG/NDEBUG).
CFLAGS += $(C_INCLUDES)#                                                      #   Ścieżki do nagłówków projektu i CMSIS.
CFLAGS += $(OPT)#                                                             #   Poziom optymalizacji (Debug/Release).
CFLAGS += -Wall#                                                              #   Włącz wszystkie podstawowe ostrzeżenia.
CFLAGS += -ffunction-sections -fdata-sections#                                #   Umieszczaj funkcje i dane w osobnych sekcjach.

ifeq ($(DEBUG),1)#                                                            # Dodatkowe flagi kompilatora, gdy włączony jest tryb DEBUG.
    CFLAGS += -g3 -gdwarf-4#                                                  #   Dodaj szczegółowe informacje debug (poziom -g3, format DWARF-4).
    CFLAGS += -fstack-usage#                                                  #   Generuj pliki .su z informacją o zużyciu stosu przez każdą funkcję.
endif

DEPFLAGS = -MMD -MP#                                                          # Flagi generowania plików zależności (.d); -MF dodamy w regule.

#------------------------------------------------------------------------------------------------------------------------------------------------------------------------- 
#-------------------------------------------------------------------- Ustawienia linkera i biblioteki --------------------------------------------------------------------
#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
LIBS  = -lc#                                                                  # Lista bibliotek do dołączenia:
LIBS += -lm#                                                                  #   standardowa C + matematyczna
LIBS += -lnosys#                                                              #   „nosys” (stub syscalli).

                                                                              # Konfiguracja flag linkera:
LDFLAGS  = $(MCU)#                                                            #   Architektura CPU + FPU dla linkera.
LDFLAGS += -T$(LINKER_DIR)/$(LDSCRIPT)#                                       #   Ścieżka do skryptu linkera (.ld).
LDFLAGS += --specs=nosys.specs#                                               #   Użycie „nosys” – proste stuby dla system calli.
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref#                        #   Wygeneruj plik map z odniesieniami.
LDFLAGS += -Wl,--print-memory-usage#                                          #   Wypisz zużycie pamięci po linkowaniu.
LDFLAGS += -Wl,--gc-sections#                                                 #   Usuń nieużywane sekcje kodu/danych.
LDFLAGS += -static -u _printf_float#                                          #   Linkowanie statyczne + wsparcie printf dla float.
LDFLAGS += -Wl,--start-group $(LIBS) -Wl,--end-group#                         #   Grupowanie bibliotek dla poprawnej rezolucji symboli.
LDFLAGS += --specs=nano.specs#                                                #   Użycie „lekkiej” implementacji stdlib (newlib-nano).

#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#------------------------------------------------------------------ Ustawienia wyjścia (dane w konsoli) ------------------------------------------------------------------
#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
ifeq ($(VERBOSE),1)#                                                          # Normalny tekst w konsoli przy kompilacji: VERBOSE = 1.
    Q :=
else #                                                                        # Skrócony tekst w konsoli przy kompilacji: VERBOSE = 0.
    Q := @
endif

#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------- Budowanie -------------------------------------------------------------------------------
#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
C_OBJ   = $(addprefix $(OBJ_DIR)/,$(notdir $(C_SRC:.c=.o)))                   # Lista plików obiektowych C w katalogu OBJ_DIR.
ASM_OBJ = $(addprefix $(OBJ_DIR)/,$(basename $(notdir $(ASM_SRC))).o)         # Plik obiektowy z assemblera (startup) w katalogu OBJ_DIR.
OBJECTS = $(C_OBJ) $(ASM_OBJ)                                                 # Wszystkie obiekty (C + assembler) połączone w jedną listę.

vpath %.c $(SRC_DIR) $(SYSTEM_DIR)                                            # Ścieżki wyszukiwania plików .c – Source i System\System.
vpath %.s $(STARTUP_DIR)                                                      # Ścieżki wyszukiwania plików .s – System\Startup.

.PHONY: all dirs Wyczysc_Debug Wyczysc_Release Wyczysc_Wszystko               # Deklaracja celów „phony”.

                                                                              # Zbierz pliki wyjściowe do wygenerowania:
ALL_OUTPUTS  = $(BUILD_DIR)/$(TARGET).elf#                                    #   Plik ELF.
ALL_OUTPUTS += $(BUILD_DIR)/$(TARGET).hex#                                    #   Plik HEX.
ALL_OUTPUTS += $(BUILD_DIR)/$(TARGET).bin#                                    #   Plik BIN.
ALL_OUTPUTS += $(BUILD_DIR)/$(TARGET).list#                                   #   Plik listing (disassembly).
all: $(ALL_OUTPUTS)#                                                          # Główne zadanie – buduj wszystkie pliki wyjściowe.

                                                                              # Lista katalogów do utworzenia:
DIRS  = $(BUILD_ROOT)#                                                        #   Katalog główny Build.
DIRS += $(BUILD_DIR)#                                                         #   Katalog pośredni Debug/Release.
DIRS += $(OBJ_DIR)#                                                           #   Katalog na pliki .o.
DIRS += $(DEP_DIR)#                                                           #   Katalog na pliki .d.
DIRS += $(ASM_DIR)#                                                           #   Katalog na pliki .s.
DIRS += $(LST_DIR)#                                                           #   Katalog na pliki .lst.
DCMD = $(Q)for %%D in ($(DIRS)) do @if not exist "%%D" mkdir "%%D"            #   Zbierz wszystkie katalogi.
dirs:#                                                                        # Zadanie - Tworzenie wszystkich katalogów.
	$(DCMD)

#------------------------------------------------------------------------------------------------------------------------------------------
#------------------------------------------ Kompilacja plików .c do .o, generowanie .s, .d, .lst ------------------------------------------
#------------------------------------------------------------------------------------------------------------------------------------------
$(OBJ_DIR)/%.o: %.c Makefile | dirs
	@echo [Kompilacja]  $<
	$(Q)$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF"$(DEP_DIR)/$(@F:.o=.d)" -Wa,-a,-ad,-alms=$(LST_DIR)/$(@F:.o=.lst) $< -o $@
	$(Q)$(CC) $(CFLAGS) -S -o $(ASM_DIR)/$(@F:.o=.s) $<

#------------------------------------------------------------------------------------------------------------------------------------------
#------------------------------------------------ Kompilacja plików startupowych .s do .o -------------------------------------------------
#------------------------------------------------------------------------------------------------------------------------------------------
$(OBJ_DIR)/%.o: $(STARTUP_DIR)/%.s Makefile | dirs
	@echo.
	@echo [Kompilacja]  $<
	$(Q)$(AS) -c $(ASFLAGS) $< -o $@

#------------------------------------------------------------------------------------------------------------------------------------------
#-------------------------------------------------------- Linkowanie ELF + rozmiar --------------------------------------------------------
#------------------------------------------------------------------------------------------------------------------------------------------
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	@echo.
	@echo [Linkowanie]  $@
	$(Q)$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(Q)$(SZ) $@
	@echo.

#------------------------------------------------------------------------------------------------------------------------------------------
#-------------------------------------------------------- Generowanie .bin z .elf ---------------------------------------------------------
#------------------------------------------------------------------------------------------------------------------------------------------
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | dirs
	@echo [BIN] $@
	$(Q)$(BIN) $< $@

#------------------------------------------------------------------------------------------------------------------------------------------
#-------------------------------------------------------- Generowanie .hex z .elf ---------------------------------------------------------
#------------------------------------------------------------------------------------------------------------------------------------------
$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | dirs
	@echo [HEX] $@
	$(Q)$(HEX) $< $@

#------------------------------------------------------------------------------------------------------------------------------------------
#------------------------------------------------- Tworzenie pliku listingu z pliku .elf --------------------------------------------------
#------------------------------------------------------------------------------------------------------------------------------------------
$(BUILD_DIR)/%.list: $(BUILD_DIR)/%.elf | dirs
	@echo [LST] $@
	$(Q)$(PREFIX)objdump -h -S $< > $@
	@echo.

#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#-------------------------------------------------------------------- Czyszczenie katalogów budowania --------------------------------------------------------------------
#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Wyczysc_Debug:                                                                # Usuwanie katalogu Build\Debug.
	@echo [Kasowanie] Folder: Build/Debug
	@if exist Build\Debug rmdir /s /q Build\Debug
	@echo.

Wyczysc_Release:                                                              # Usuwanie katalogu Build\Release.
	@echo [Kasowanie] Folder: Build/Release
	@if exist Build\Release rmdir /s /q Build\Release
	@echo.

Wyczysc_Wszystko:                                                             # Usuwanie obu katalogów Build\Debug i Build\Release.
	@echo [Kasowanie] Folder: Build/Debug + Build\Release