### **ADR-0013: Mobile-First Web Draft Editor**

**Status:** accepted

**Date:** 2026-05-22

#### **1. Context and Problem Statement**

With the transition of "Biotope" to a global multiplayer platform, we need a mechanism for users to design and submit their own 8x8 patterns. The original plan (implementing this in the Raylib-based C GUI via WASM) was found to be suboptimal for the primary target device: **Smartphones**.

A Raylib/WASM application is relatively heavy for mobile browsers and presents significant challenges for standard UI interactions like text input (nicknames) and responsive touch-based grid editing. To ensure frictionless community participation, we need a solution that is lightweight, touch-optimized, and accessible via a simple URL.

#### **2. Decision**

We will implement the **Draft Mode (Level Editor)** as a standalone, **mobile-first Web Interface** using standard Web technologies (HTML5, CSS3, Vanilla JavaScript), instead of integrating it into the C/Raylib codebase.

Key technical components of this decision:
- **Decoupled Architecture:** The editor is a lightweight micro-frontend, separate from the main Raylib simulation viewer.
- **Mobile-First UX:** The 8x8 grid will be implemented using CSS Grid/Flexbox, optimized for touch interaction on small screens.
- **Native Web Input:** Standard HTML `<input>` fields for nicknames, leveraging mobile-native keyboards and accessibility features.
- **Client-Side Validation:** JavaScript-based enforcement of "Fair Play" rules (8x8 bounding box, max 24 cells) to provide instant user feedback.
- **Backend Integration:** Direct use of the browser's `fetch()` API to submit configurations to the FastAPI `POST /api/v1/submit_config` endpoint.
- **Identity Persistence:** Use of `localStorage` to persist a unique `player_id` (UUID) across sessions.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**
- **Superior Mobile UX:** Significantly better touch responsiveness and ease of text input compared to a C-based UI.
- **Performance:** Instant loading times due to minimal asset weight (no WASM binary or Raylib library required for the editor).
- **Ease of Development:** Faster iteration and styling using standard Web dev tools and CSS.
- **Accessibility:** Better support for screen readers and OS-level accessibility features.

**Negative Consequences (Disadvantages):**
- **Visual Divergence:** Requires effort to maintain visual consistency between the HTML editor and the Raylib simulation (e.g., matching colors and shaders).
- **Code Duplication:** Business rules (8x8 limit, 24 cells) must be implemented in JS (frontend) and Python (backend), whereas a C-based WASM editor could have potentially shared logic with the simulation engine.

#### **4. Alternatives Considered**

- **Integrated C/Raylib Editor (Original Plan):**
    - *Pros:* Shared codebase with the simulation.
    - *Cons:* Poor mobile UX, heavy load times, difficult text input on mobile.
- **Standalone Mobile App (Native):**
    - *Pros:* Best performance.
    - *Cons:* High friction (requires app store download); contradicts the "onboarding-via-URL" goal.
- **Pure JSON API (No UI):**
    - *Pros:* Zero frontend work.
    - *Cons:* Not accessible to non-technical users.
