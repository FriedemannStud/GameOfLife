# Technical Design: Integrated Simulation Protocol and Replay System

**Status:** Proposed
**Date:** 2026-01-17

## 1. Introduction

This document describes the technical architecture for the "Integrated Simulation Protocol and Replay System". The goal is to extend the current `GameOfLife` application to support comprehensive logging of simulation starts, ensure reproducibility, and provide a user-friendly interface for loading past simulations. This design addresses requirements defined in **DEV_SPEC-0003** and **ADR-0003**.

## 2. System Architecture

The feature touches upon three main areas of the application:
1.  **File I/O Layer (`file_io.c/h`)**: Enhanced to read/write the extended protocol format.
2.  **Core Game Logic (`gui.c`)**: State machine updates to handle the new `STATE_LOAD` and automatic saving triggers.
3.  **File System Interface (`main.c` / `file_io.c`)**: New functionality to list and sort directory contents.

### 2.1 Component Diagram

```mermaid
graph TD
    User[User Input] --> GUI[GUI Module (gui.c)]
    GUI -->|Transition to STATE_RUNNING| Saver[Auto-Save Logic]
    GUI -->|Transition to STATE_LOAD| Browser[File Browser UI]
    
    Saver -->|Write Protocol| FileIO[File I/O Module (file_io.c)]
    Browser -->|List Files| FileIO
    Browser -->|Read Metadata| FileIO
    Browser -->|Load Protocol| FileIO
    
    FileIO -->|Read/Write| FS[File System (.bio files)]
```

## 3. Detailed Component Design

### 3.1 Extended Protocol Format (`.bio` v2)

We will extend the `.bio` format while maintaining backwards compatibility where possible.

**Format Specification:**
The file is text-based.
*   **Line 1 (Header):** `VERSION TIMESTAMP ROWS COLS MAX_POP MAX_ROUNDS DELAY_MS`
    *   `VERSION`: Integer, e.g., `2`. (Legacy files start with `ROWS` directly).
    *   `TIMESTAMP`: Long integer (Unix epoch).
*   **Body:** Same as existing logic (Lines containing `Row Col Team`).
*   **Footer (Optional):** Key-value pairs starting with `#` for results.

**Backward Compatibility Strategy:**
*   When reading line 1, we check if the first number is small (e.g., < 10 for version) or large (e.g., > 10 for Rows, assuming sensible grid defaults).
*   *Refined approach:* The legacy format is `ROWS COLS MAX_POP` (3 integers). The new format will have 7 integers. `fscanf` return value check will determine the format version.
    *   If `fscanf` returns 3: Treat as Legacy (Version 1). Default `max_rounds=1000`, `delay=100`.
    *   If `fscanf` returns 7: Treat as Version 2.

### 3.2 File I/O Module Enhancements

**New Functions in `file_io.h`:**
```c
// Struct to hold file metadata for the browser
typedef struct {
    char filename[256];
    long timestamp;
    int rows, cols;
    int max_rounds;
} ProtocolInfo;

// Lists .bio files in a directory. Returns count.
// Caller must free the list.
int list_protocol_files(const char *dir_path, ProtocolInfo **out_list);

// Loads just the header to preview metadata
int load_protocol_metadata(const char *filename, ProtocolInfo *info);
```

**Modified Functions:**
*   `save_grid(...)`: Adds new parameters to the signature or extracts them from `GameConfig`.
*   `load_grid(...)`: Updates `GameConfig` with the new fields (`max_rounds`, `delay_ms`).

### 3.3 State Machine Changes (`gui.c`)

**New State: `STATE_LOAD`**
*   **Entry:** Call `list_protocol_files`. Sort result by timestamp (descending).
*   **Render:**
    *   Draw a scrollable list of filenames.
    *   Highlight selected index.
    *   Draw a "Preview Panel" showing details of the selected file (`ProtocolInfo`).
*   **Input:**
    *   `UP/DOWN`: Change selection index.
    *   `ENTER`: Call `load_grid` with selected file, free list, transition to `STATE_EDIT`.
    *   `ESC/Q`: Free list, transition back to `STATE_CONFIG` (or previous state).

**Automatic Save Trigger:**
*   Inside `STATE_EDIT`, when `KEY_ENTER` is pressed:
    1.  Generate filename: `biotope_results/run_<timestamp>.bio`.
    2.  Call `save_grid`.
    3.  Transition to `STATE_RUNNING`.

### 3.4 Directory Listing Implementation (Win32)

Since the environment is Windows (`win32`), we will use `<windows.h>` and `FindFirstFile` / `FindNextFile` for directory traversal if `dirent.h` is not available or reliable.
*   *Note:* MinGW usually provides `dirent.h`. We will try `dirent.h` first for portability, falling back to WinAPI if strictly necessary. Given the context, `dirent.h` is preferred for simpler code.

## 4. Data Models

### 4.1 ProtocolInfo Struct
Used for passing file data to the UI.

```c
typedef struct {
    char filename[128]; // e.g., "run_20260117_120000.bio"
    char filepath[256]; // Full relative path
    long timestamp;     // For sorting
    
    // Preview Data
    int rows;
    int cols;
    int max_rounds;
    int max_population;
} ProtocolInfo;
```

## 5. Security and Performance

### 5.1 Performance
*   **File Listing:** Listing thousands of files might be slow.
    *   *Mitigation:* We will cap the displayed list to the most recent 100 files if performance becomes an issue. For MVP, we load all.
*   **Rendering:** The file list rendering will use simple text rendering. No complex textures.

### 5.2 Security
*   **Path Traversal:** The loader should strictly prefix filenames with `biotope_results/` or `./` and not allow arbitrary paths from user input (though user input is only via selection, not text entry).
*   **Buffer Overflows:** Standard C strictness. Use `snprintf` instead of `sprintf`. Check bounds when reading filenames.

## 6. Implementation Plan

1.  **Phase 1: File I/O Core**
    *   Modify `save_grid` to write v2 header.
    *   Modify `load_grid` to detect v1 vs v2 and parse accordingly.
    
2.  **Phase 2: Directory Helper**
    *   Implement `list_protocol_files` using `dirent.h`.
    *   Implement helper to parse timestamp from filename or file stat.

3.  **Phase 3: GUI - Auto-Save**
    *   Inject saving logic into `STATE_EDIT` transition.

4.  **Phase 4: GUI - Browser**
    *   Implement `STATE_LOAD` rendering loop.
    *   Connect input handling to navigation.

## 7. API / Function Signatures (Draft)

```c
// file_io.h

// Writes extended format
int save_grid_extended(const char *filename, World *w, GameConfig *c);

// Reads extended format, handles backward compat
int load_grid_extended(const char *filename, World *w, GameConfig *c);

// Returns number of files found. 
// 'count' is updated.
// Returns array of ProtocolInfo. Caller must free.
ProtocolInfo* get_protocol_list(int *count);
```
