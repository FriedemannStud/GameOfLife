# ADR-0002: Interactive UI System for Biotope Mode

**Status:** Proposed

**Date:** 2026-01-09

## 1. Context and Problem Statement

The "Competitive Biotope Mode" (described in the README, Chapter 6) requires a significant expansion of the current command-line interface (CLI). The current implementation (`main.c`) runs the simulation non-interactively with random initialization. 

**New Requirements (from Chapter 6):**
*   **Interactive Setup:** Players must be able to set the initial state of the grid (Red vs. Blue cells) by clicking with a mouse on a graphical grid.
*   **Configuration:** Users need to input simulation parameters before starting:
    *   Grid size (e.g., 100x100).
    *   Update delay (e.g., 500ms).
    *   Max initial population per team.
    *   Max rounds.
*   **Visualization:** The simulation must show the grid in real-time, distinguishing Red and Blue teams.
*   **File I/O:** Functionality to save/load initial configurations and export statistical results (`.md` files).
*   **Usability:** Needs a "GUI" with specific layout requirements (Red on right, Blue on left).

**Problem:** Standard C libraries (`stdio.h`) cannot provide mouse interaction, high-resolution graphical rendering, or GUI widgets (text boxes, buttons). We need a library or framework to handle the presentation layer while maintaining the C codebase.

## 2. Decision

We will implement the User Interface using the **Raylib** library, supplemented by **raygui** for UI controls.

**Architecture Overview:**
1.  **Library Choice:** **Raylib** is a lightweight, open-source C library for video game programming. It is chosen over others (like SDL2 or GTK) because it is extremely self-contained, has zero external dependencies on Windows (static linking), and includes a "Game Loop" architecture that fits the simulation perfectly.
2.  **UI Widgets:** We will use **raygui**, an immediate-mode GUI module for Raylib, to implement the configuration screens (Input boxes for Rows/Cols, Sliders for Speed). This avoids writing custom button/text-input logic.
3.  **Application States:** The application will be refactored into a state machine:
    *   `STATE_CONFIG`: Show input fields for settings (Section 6.1 - 6.4).
    *   `STATE_EDIT`: Display the empty grid. Allow users to place cells via Mouse Click. Enforce "Hemisphere" constraints (Red right, Blue left).
    *   `STATE_RUNNING`: The main simulation loop (rendering the grid and calling the core `update_generation` logic).
    *   `STATE_GAME_OVER`: Show statistics and save results.
4.  **Integration:** The existing core logic (arrays, update rules) in `main.c` will be separated into a "pure logic" module (e.g., `simulation.c`) to be called by the Raylib application.

## 3. Consequences of the Decision

**Positive Consequences (Advantages):**
*   **Rapid Development:** Raylib provides high-level functions for drawing grids (`DrawRectangle`), handling input (`IsMouseButtonPressed`), and managing windows, drastically reducing boilerplate code compared to raw SDL2 or OpenGL.
*   **Built-in GUI:** `raygui` solves the problem of "how to enter numbers" (configuration) without needing a complex widget toolkit.
*   **Performance:** Raylib is hardware-accelerated (OpenGL), ensuring smooth rendering of large grids (100x100+) which would flicker or lag in a standard terminal.
*   **Portability:** The same C code can compile for Windows, Linux, and even Web (WASM), satisfying the cross-platform nature of the project.

**Negative Consequences (Disadvantages):**
*   **Dependency Management:** The project now requires `raylib` (and `raygui`) to be installed or included in the build process. The `Dockerfile` will need updating to support a build environment for a graphical app (though running the GUI inside Docker is complex; the primary target for the GUI is likely the host Windows machine).
*   **Code Restructuring:** The current `main.c` mixes logic and CLI output. This must be refactored to separate the "Model" (Grid Data) from the "View/Controller" (Raylib Input/Output).

## 4. Alternatives Considered

**Alternative A: SDL2 (Simple DirectMedia Layer)**
*   *Concept:* Use SDL2 for windowing and rendering.
*   *Reason for Rejection:* SDL2 is lower-level. Implementing text input fields and buttons (for the configuration menu) requires significantly more code or third-party add-ons (like Dear ImGui). Raylib + raygui offers a more "all-in-one" solution for C.

**Alternative B: Web-Based Interface (Backend in C)**
*   *Concept:* Run the C simulation as a backend server and use a Browser for the UI.
*   *Reason for Rejection:* Adds complexity (HTTP server, JSON serialization, JavaScript frontend). The requirements implies a single application ("C-Umgebung"), and a native Desktop App provides the most responsive "Game Loop" experience.

**Alternative C: Console TUI (Text User Interface) with ncurses**
*   *Concept:* Advanced terminal UI with mouse support.
*   *Reason for Rejection:* "Graphic grid" requirement is hard to satisfy visually in a terminal (pixels are square, characters are rectangular). Resolution is limited. Visually less appealing for a "Biotope" competition.
