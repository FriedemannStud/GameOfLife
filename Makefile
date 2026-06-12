# Makefile für das Biotope Game of Life (Clean Architecture)
# KI-Agent unterstützt

# 1. Platform Detection
ifeq ($(OS),Windows_NT)
    PLATFORM = WINDOWS
    TARGET_NAME = biotope.exe
    LDFLAGS = -lraylib -lopengl32 -lgdi32 -lwinmm -lpthread -lcurl
else
    PLATFORM = LINUX
    TARGET_NAME = biotope
    LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -lcurl
endif

# 2. Directory Structure
SRC_DIR = src
BUILD_DIR = build
CORE_DIR = $(SRC_DIR)/core
GUI_DIR = $(SRC_DIR)/gui
IO_DIR = $(SRC_DIR)/io
APPS_DIR = $(SRC_DIR)/apps
VENDOR_DIR = $(SRC_DIR)/vendor

# 3. Compiler Variables
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O3 -fopenmp
# Include paths
INCLUDES = -I$(CORE_DIR) -I$(GUI_DIR) -I$(IO_DIR) -I$(VENDOR_DIR)/cJSON -I$(SRC_DIR)

# 4. Binaries
BIN_GUI = $(BUILD_DIR)/$(TARGET_NAME)
BIN_HEADLESS = $(BUILD_DIR)/biotope_headless
BIN_HYPER = $(BUILD_DIR)/biotope_hyper_worker

# 5. Object Files
# We identify the objects needed for each target
OBJ_CORE = $(BUILD_DIR)/core/game_logic.o
OBJ_IO = $(BUILD_DIR)/io/file_io.o $(BUILD_DIR)/io/network_io.o
OBJ_VENDOR = $(BUILD_DIR)/vendor/cJSON/cJSON.o
OBJ_GUI_MODULES = $(BUILD_DIR)/gui/renderer.o $(BUILD_DIR)/gui/app_state_manager.o

# Objects for specific apps
OBJ_APP_GUI = $(BUILD_DIR)/apps/gui/main.o
OBJ_APP_HEADLESS = $(BUILD_DIR)/apps/headless/main_headless.o
OBJ_APP_HYPER = $(BUILD_DIR)/apps/hyper/main_hyper.o

# 6. Targets
WORKER_BIN_DIR = worker_bin

all: $(BIN_GUI) $(BIN_HEADLESS) $(BIN_HYPER)

# Rules for binaries
$(BIN_GUI): $(OBJ_APP_GUI) $(OBJ_CORE) $(OBJ_GUI_MODULES) $(OBJ_IO) $(OBJ_VENDOR)
	$(CC) $(OBJ_APP_GUI) $(OBJ_CORE) $(OBJ_GUI_MODULES) $(OBJ_IO) $(OBJ_VENDOR) $(CFLAGS) $(LDFLAGS) -o $@

$(BIN_HEADLESS): $(OBJ_APP_HEADLESS) $(OBJ_CORE) $(OBJ_IO) $(OBJ_VENDOR)
	$(CC) $(OBJ_APP_HEADLESS) $(OBJ_CORE) $(OBJ_IO) $(OBJ_VENDOR) $(CFLAGS) -o $@ -lm -lpthread -lcurl

$(BIN_HYPER): $(OBJ_APP_HYPER) $(OBJ_CORE) $(OBJ_IO) $(OBJ_VENDOR)
	$(CC) $(OBJ_APP_HYPER) $(OBJ_CORE) $(OBJ_IO) $(OBJ_VENDOR) $(CFLAGS) -o $@ -lm -lpthread -fopenmp -lcurl
	@mkdir -p $(WORKER_BIN_DIR)
	cp $@ $(WORKER_BIN_DIR)/biotope_hyper_worker

# Pattern rule for object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 7. Cleanup
clean:
	rm -rf $(BUILD_DIR) $(WORKER_BIN_DIR)

# 8. Help
help:
	@echo "Plattform: $(PLATFORM)"
	@echo "Verfügbare Befehle:"
	@echo "  make        - Kompiliert alle Targets in $(BUILD_DIR)/ und kopiert Worker-Binary nach $(WORKER_BIN_DIR)/"
	@echo "  make clean  - Löscht das $(BUILD_DIR)/ Verzeichnis"
	@echo "  make help   - Zeigt diese Hilfe an"
	@echo ""
	@echo "Wichtig: Immer 'make' vor 'docker-compose up' ausführen!"
