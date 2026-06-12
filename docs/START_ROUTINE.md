# Start Routine: Biotope Game of Life

This guide helps you start the project quickly and get back into the workflow, even after a longer break.

---

## 1. Building the Project (Compilation)

Before you can start the app, you need to compile the source code into executable programs.

### Local (Native App for Linux/Windows)
When working directly on your machine (with Raylib installed):

#### Linux Prerequisites (One-Time Setup)
If `raylib.h` is not found during compilation, the system libraries are missing. Install them with:
```bash
sudo apt update
sudo apt install libraylib-dev  # if available in the repo
# OR (build manually if the above does not work):
sudo apt install make git cmake libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev
```
*Note: If you build Raylib manually, follow the instructions at [raylib.com](https://www.raylib.com).*

1. Open a terminal in the project root directory.
2. Run this command:
   ```bash
   make clean && make
   ```
   *Tip: `make clean` ensures no old artefacts interfere with the build.*

### For the Browser (Web/WASM)
To build the web version:
1. Run this command:
   ```bash
   make -f Makefile.wasm
   ```

---

## 2. Starting the App

### Running the Native App
After successful compilation, you can start the program:
- **Linux:** 
  ```bash
  ./build/biotope
  ```
- **Windows:** Double-click `biotope.exe` or in the terminal:
  ```bash
  build\biotope.exe
  ```

`make` produces three binaries in the `build/` folder:

| Binary | Description |
|--------|-------------|
| `build/biotope` | Interactive GUI (Raylib, requires display) |
| `build/biotope_headless` | Single match CLI: `./build/biotope_headless <red.json> <blue.json> [out.json]` |
| `build/biotope_hyper_worker` | Batch tournament CLI (for the matchmaker worker) |

### Viewing the Web Version in the Browser
The web files cannot simply be opened by double-clicking. You need a small web server:
1. Start a simple server (e.g. with Python):
   ```bash
   python3 -m http.server 8080
   ```
2. Open your browser and enter this address:
   [http://localhost:8080/biotope.html](http://localhost:8080/biotope.html)

---

## 3. Working with Docker

If you prefer not to install libraries (like Raylib or Emscripten) locally, you can use the prepared Docker environment.

1. **Start the container:**
   ```bash
   docker-compose up -d
   ```
2. **Enter the development environment:**
   ```bash
   docker-compose exec c-dev bash
   ```
3. **Build inside Docker:**
   You are now inside the system and can simply use `make` or `make -f Makefile.wasm`.

---

## Quick Check When Returning to the Project

When returning after a break, this quick workflow is recommended:

1. **Check status:** `git status` (What did I last change?)
2. **Check the task list:** Look in `docs/tasks/` for any open items (checkboxes `- [ ]`).
3. **Read the log:** In `docs/CHANGELOG.md` you can see what was last successfully completed.
4. **Build & start:** Use the commands above to ensure the current version runs stably.

---

*Good luck exploring Biotope further!*
