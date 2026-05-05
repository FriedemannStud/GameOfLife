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
SOURCES = main.c game_logic.c gui.c file_io.c
HEADERS = game_logic.h gui.h file_io.h

# 4. Standard-Ziel
all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(SOURCES) $(CFLAGS) $(LDFLAGS) -o $(TARGET)

# 5. Aufräumen
clean:
	rm -f $(TARGET) biotope.exe biotope

# 6. Hilfs-Info
help:
	@echo "Erkanntes System: $(OS)"
	@echo "Verfügbare Befehle:"
	@echo "  make        - Kompiliert das Projekt"
	@echo "  make clean  - Löscht die ausführbare Datei"
	@echo "  make help   - Zeigt diese Hilfe an"Makefile: ;
