# Evidence-Based Investigation Report: DEV_TASKS-0017 (C-Networking)

**Date:** 28 May 2026  
**Topic:** Analysis of integration problems and runtime errors after completion of the programming work for DEV_TASKS-0017.

## 1. Management Summary
The implementation of the C-networking components required by `DEV_TASKS-0017` (Phases 4–6) and the Python backend extensions (Phases 2–3) was carried out correctly at the code level. The described error – a "minor configuration problem" with "complex causes" – is **not a bug in the C code**, but a **dependency conflict in the Docker architecture** (environment mismatch).

Additionally, a potentially unsafe string operation (`strncpy`) was identified in the C code, which causes the compiler warnings observed during the build.

## 2. Root Cause Analysis: Docker & `libcurl` Runtime Environment

### Symptom
The Python worker `backend/app/worker.py`, running inside the `matchmaker` Docker container, attempts to execute the C-written `biotope_hyper_worker` at the end of an epoch. This operation fails with the following error in the container log:
```
ERROR:epoch_worker:Hyper-Worker failed: ./build/biotope_hyper_worker: error while loading shared libraries: libcurl.so.4: cannot open shared object file: No such file or directory
```

### Cause
1. **Build process (host/c-dev):** The `make` command is run on the host system or in the `c-dev` container. Since the `-lcurl` library was added to the compilation in Phase 1, the resulting binary `biotope_hyper_worker` is dynamically linked against `libcurl.so.4`.
2. **Execution (matchmaker):** The compiled binary is mounted into the `matchmaker` container via the Docker volume (`./build:/app/build`).
3. **Configuration gap:** The `matchmaker` container is based on the `python:3.11-slim` image (a minimal Debian). The `docker-compose.yml` only installs `libgomp1` on startup, but **not** `libcurl4`. Since the dynamic library is missing, the operating system refuses to execute the C binary.

### Evidence
An `ldd` check inside the `matchmaker` container confirms the missing dependency:
```bash
$ docker compose exec matchmaker ldd ./build/biotope_hyper_worker
        libcurl.so.4 => not found
```

## 3. Code Quality & Memory Safety (`network_io.c`)

The review of the new module `src/io/network_io.c` (Phases 5 & 6) shows high code quality that meets the project guidelines:
- **Thread Safety:** The shared states `g_leaderboard` and `g_highlights` are correctly protected with `PTHREAD_MUTEX_INITIALIZER`. The getters copy data via `memcpy` inside the mutex lock.
- **Memory Management (Valgrind):** Memory allocated for `libcurl` (via `realloc` in the `write_callback`) is correctly freed with `free(chunk.data)` at thread end. Memory allocated by `cJSON_Parse` is cleaned up with `cJSON_Delete(root)`. No memory leaks are apparent.
- **Error handling:** HTTP response codes and failed JSON parsing are caught.

## 4. Compiler Warnings (`main_hyper.c`)

### Symptom
Warnings occur when compiling the hyper-worker:
```
src/apps/hyper/main_hyper.c:110:25: warning: '__builtin_strncpy' output may be truncated copying 63 bytes from a string of length 63 [-Wstringop-truncation]
```

### Cause
In `main_hyper.c` line 110, an attempt is made to copy a player ID into the struct `HighlightEntry`:
```c
strncpy(my_highlights[pos].player_id_red, m == 0 ? competitors[i].player_id : competitors[j].player_id, 63);
```
The array `player_id_red` is 64 bytes long. A `strncpy` with length 63 guarantees *no* null-termination if the source string is exactly 63 characters long. GCC warns about this (`-Wstringop-truncation`), as garbage data could be read in the worst case.

## 5. Actionable Next Steps (Proposed Solution)

To fully complete the integration of DEV_TASKS-0017, the following steps must be taken:

1. **Update Docker Compose (fix for libcurl):**
   In `docker-compose.yml`, under the `matchmaker` service, update the `command` string to install `libcurl4`:
   ```yaml
   command: /bin/bash -c "pip install -r requirements.txt && apt-get update && apt-get install -y libgomp1 libcurl4 && python3 app/worker.py"
   ```

2. **Fix compiler warning (strncpy fix):**
   In `src/apps/hyper/main_hyper.c`, add explicit null-termination:
   ```c
   strncpy(my_highlights[pos].player_id_red, m == 0 ? competitors[i].player_id : competitors[j].player_id, 63);
   my_highlights[pos].player_id_red[63] = '\0';
   strncpy(my_highlights[pos].player_id_blue, m == 0 ? competitors[j].player_id : competitors[i].player_id, 63);
   my_highlights[pos].player_id_blue[63] = '\0';
   ```

3. **Restart the containers:**
   ```bash
   docker compose restart matchmaker
   ```

After these adjustments, the tasks in `DEV_TASKS-0017` will be successfully completable.

---

## 6. Resolution

**Status: Resolved.**

Both measures described in Section 5 have been implemented:

1. **libcurl4 fix (docker-compose.yml):** The `matchmaker` service installs `libcurl4` via `apt-get` on startup. The error "cannot open shared object file" no longer occurs.

2. **strncpy fix (main_hyper.c):** The `strncpy` calls were replaced with `snprintf` using `sizeof(...)`, guaranteeing null-termination and resolving the GCC warning (`-Wstringop-truncation`).
