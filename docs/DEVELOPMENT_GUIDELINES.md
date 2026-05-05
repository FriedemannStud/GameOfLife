# **Development Guidelines Summary**

This document defines the documentation strategy for the Biotope GameOfLife project, ensuring consistency, clarity, and effective collaboration.

## **1. Centralized Documentation**
All project documentation is stored in the `docs/` directory with the following structure:
```
GameOfLife/
├── docs/
│   ├── adr/ (Architectural Decision Records)
│   ├── specs/ (Feature Specifications)
│   ├── tasks/ (Task tracking with checkboxes)
│   ├── tech_design/ (Technical Implementation Details)
│   ├── CODING_STYLE.md
│   ├── PROJECT_BRIEFING.md
│   ├── DEVELOPMENT_GUIDELINES.md
│   └── CHANGELOG.md (Log of major changes)
└── ... (project code: .c, .h, Makefile)
```

## **2. Documentation Types & Purpose**
| **Document** | **Focus** | **Content** | **Lifecycle** |
|--------------|-----------|------------|--------------|
| **ADRs** | *Why* behind major decisions | Context, decision, rationale, consequences | Immutable once implemented |
| **DEV_SPEC** | *What* is the feature | Requirements, User Stories, UI Mockups | Updated during design phase |
| **DEV_TASKS** | *How* to implement | Step-by-step checklist using Markdown checkboxes (`- [ ]`) | Updated continuously |
| **CHANGELOG** | *What* changed and when | Chronological history of major steps | Updated after completion |

## **3. Interlinking Strategy**
- **DEV_TASKS → ADR/SPEC:** Link tasks to relevant design documents.
- **CHANGELOG → TASKS:** Reference completed tasks in the log.

## **4. Developer Workflow**
1. **Before starting a major change:**
   - Check if an ADR is needed; create it if required.
   - Create a `DEV_SPEC` for requirements and a `DEV_TASKS` for the implementation plan.
2. **During development:**
   - Update task status in `DEV_TASKS-xxxx.md` and check off completed items.
3. **Before completion:**
   - Test thoroughly (compile with `make`, verify logic).
4. **Upon completion:**
   - Update `CHANGELOG.md` with details.
   - Mark tasks as "Done" in the relevant task files.

## **5. Testing Guidelines**
### **Principles**
- Test early, often, and reproducibly.
- Use small test drivers for logic and interactive verification for UI.

### **Execution**
1. **Build project:**
   ```bash
   make clean && make
   ```
2. **Run application:**
   ```bash
   ./biotope
   ```
3. **Expected outcome:** No compilation warnings, stable execution, and correct Game of Life rules.

### **Test Types**
| **Type** | **Purpose** | **Scope** | **Best Practices** |
|----------|------------|-----------|------------------|
| **Logic Verification** | Verify GOL rules | `game_logic.c` | Use `test_logic.c` (if available) or small known patterns. |
| **Interactive Test** | Validate UI/UX | `gui.c`, Raylib HUD | Manually check buttons, sliders, and state transitions. |
| **Regression Test** | Ensure old features work | Entire App | Verify saving/loading still works after logic changes. |

## **6. Code Quality**
- **Rule:** All code **MUST** comply with `docs/CODING_STYLE.md`.
- **Action:** Ensure adherence before committing (naming, AI comments, memory management).

## **7. UI/UX & State Machine Architecture (Raylib-Centric)**

The application follows a State-Machine-based UI paradigm using Raylib. This ensures a clean separation of different application phases and a focused user experience.

**The 3 Pillars of UI:**

1. **The Application States:**
   - **Principle:** The app operates in distinct states (e.g., `STATE_CONFIG`, `STATE_RUNNING`, `STATE_PAUSED`). 
   - **Benefit:** Simplifies input handling and rendering logic by only dealing with one context at a time.

2. **The HUD (Heads-Up Display):**
   - **Principle:** Information and controls are overlaid on the simulation grid using semi-transparent panels.
   - **Structure:** Top bar for status, bottom bar for controls/navigation.

3. **Optimized Grid Rendering:**
   - **Principle:** Use Texture-based rendering for the grid to ensure high FPS even with large populations.
   - **Structure:** Update a pixel buffer and upload it to the GPU once per frame instead of drawing thousands of individual rectangles.

This summary retains all key information while tailoring it to the C/Raylib architecture.