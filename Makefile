# Makefile für das Biotope Game of Life (Cross-Platform: Windows/Linux)
# KI-Agent unterstützt

# 1. Plattform-Erkennung
ifeq ($(OS),Windows_NT)
    # --- Windows Konfiguration ---
    TARGET = biotope.exe
    LDFLAGS = -lraylib -lopengl32 -lgdi32 -lwinmm -lpthread
else
    # --- Linux/Docker Konfiguration ---
    TARGET = biotope
    # Linux benötigt oft zusätzliche Bibliotheken für Grafik/Threads
    LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
endif

# 2. Compiler Variablen
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O3 -fopenmp

# 3. Dateilisten
SOURCES = main.c game_logic.c renderer.c app_state_manager.c file_io.c cJSON.c
HEADERS = game_logic.h renderer.h app_state_manager.h core_types.h config.h file_io.h cJSON.h
HEADLESS_SOURCES = main_headless.c game_logic.c file_io.c cJSON.c
HYPER_SOURCES = main_hyper.c game_logic.c file_io.c cJSON.c

# 4. Standard-Ziel
all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(SOURCES) $(CFLAGS) $(LDFLAGS) -o $(TARGET)

headless: $(HEADLESS_SOURCES) $(HEADERS)
	$(CC) $(HEADLESS_SOURCES) $(CFLAGS) -o biotope_headless -lm -lpthread

hyper: $(HYPER_SOURCES) $(HEADERS)
	$(CC) $(HYPER_SOURCES) $(CFLAGS) -o biotope_hyper_worker -lm -lpthread -fopenmp

# 5. Aufräumen
clean:
	rm -f $(TARGET) biotope.exe biotope biotope_headless biotope_hyper_worker
	touch biotope_headless && chmod +x biotope_headless

# 6. Hilfs-Info
help:
	@echo "Erkanntes System: $(OS)"
	@echo "Verfügbare Befehle:"
	@echo "  make        - Kompiliert das Projekt"
	@echo "  make clean  - Löscht die ausführbare Datei"
	@echo "  make help   - Zeigt diese Hilfe an"Makefile: ;
