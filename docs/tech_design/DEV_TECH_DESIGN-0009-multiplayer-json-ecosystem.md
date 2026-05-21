# Technical Design: Unified JSON Persistence (Hard Cut Strategy)

This document outlines the strategy to replace the legacy `.bio` format with a unified JSON format across the entire Biotope project, as discussed in the context of **ADR-0009** and **DEV_SPEC-0009**.

---

### 1. The Goal: Architectural Purity
Following the user's directive, we will perform a **Hard Cut** migration. The legacy `.bio` format is declared obsolete and will be completely removed from the project. This ensures maximum simplicity and prevents any architectural redundancy.

### 2. Implementation Strategy: The "Hard Cut"

#### 2.1 Native JSON Core
- **Library Integration:** We will integrate the `cJSON` library into the C codebase.
- **Unified `file_io.c`:**
    - `save_grid()`: Completely rewritten to generate JSON based on the **ADR-0009** schema. Files will now use the `.json` extension.
    - `load_grid()`: Completely rewritten to parse JSON files using `cJSON`. It will no longer support legacy `.bio` text parsing.

#### 2.2 System-Wide Update
1. **Auto-Saves:** The auto-save logic in `gui.c` (triggered on simulation start) will be updated to use the `.json` extension.
2. **File Browser:** The `list_protocol_files` function in `file_io.c` will be updated to filter exclusively for `.json` files.
3. **Metadata Display:** The UI preview panel in `STATE_LOAD` will be updated to extract metadata from the new JSON structure.

### 3. Workflow for Implementation

1.  **Step 1: JSON Library Setup**
    - Add `cJSON.c/h` to the project.
    - Update `Makefile` and `Makefile.wasm` to include the library.
2.  **Step 2: Rewrite Persistence Layer**
    - Replace the contents of `save_grid` and `load_grid` in `file_io.c` with the new JSON logic.
    - Ensure all coordinates are handled as **relative** to the bounding box.
3.  **Step 3: UI & Event Integration**
    - Update `gui.c` constants and keys (e.g., `KEY_S`, `KEY_ENTER`) to reflect the new file naming and structure.
4.  **Step 4: Cleanup**
    - Remove legacy header parsing and `sscanf` logic from `file_io.c`.

### 4. Benefits
- **Zero Technical Debt:** No legacy code remains in the project.
- **Unified Logic:** The same code handles local files and multiplayer API payloads.
- **Future Proof:** The metadata-rich JSON format is easily extendable for future tournament features.

---

### 🎓 Für den Informatik-Studenten (Das Konzept)
Dies ist ein **"Breaking Change"** mit dem Ziel der **architektonischen Reinheit**. Manchmal ist es besser, alte Zöpfe radikal abzuschneiden, statt Komplexität durch Abwärtskompatibilität anzuhäufen – besonders wenn die alten Daten (wie hier) nicht geschäftskritisch sind. Wir reduzieren die Komplexität des Codes erheblich und schaffen eine saubere, moderne Basis für unser Multiplayer-System.

// KI-Agent unterstützt: Hard cut migration strategy for absolute format unity.
