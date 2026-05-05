#include <stdio.h>
#include <time.h>
#include <stdlib.h> // For malloc/free
#include <string.h>
#include <dirent.h>
#include "file_io.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

// Comparator for qsort to sort by timestamp descending
static int compare_protocol_info(const void *a, const void *b) {
    ProtocolInfo *pa = (ProtocolInfo *)a;  // typedef struct in file_io.h definiert
    ProtocolInfo *pb = (ProtocolInfo *)b;
    if (pb->timestamp > pa->timestamp) return 1;
    if (pb->timestamp < pa->timestamp) return -1;
    return 0;
}

// KI-Agent unterstützt
int save_grid(const char *filename, World *w, GameConfig *c) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        printf("Error saving file %s\n", filename);
        return 0;
    }
    
    // v2 Header: Version Timestamp Rows Cols MaxPop MaxRounds Delay
    // Version 2
    long timestamp = (long)time(NULL);
    fprintf(f, "2 %ld %d %d %d %d %d\n", 
            timestamp, 
            c->rows, c->cols, 
            c->max_population, 
            c->max_rounds, 
            c->delay_ms);
    
    // Save live cells only: r c team
    int stride = c->cols + 2;
    for(int r = 1; r <= c->rows; r++) {
        for(int col = 1; col <= c->cols; col++) {
            int idx = r * stride + col;
            if (w->grid[idx] != DEAD) {
                // Save coordinates relative to the active area (0-indexed)
                fprintf(f, "%d %d %d\n", r - 1, col - 1, w->grid[idx]);
            }
        }
    }
    
    fclose(f);
    printf("Saved to %s\n", filename);

#if defined(PLATFORM_WEB)
    EM_ASM({
        FS.syncfs(false, function(err) {
            if (err) {
                console.error("IndexedDB sync error:", err);
            } else {
                console.log("Biotope saved to IndexedDB");
            }
        });
    });
#endif

    return 1;
}

// KI-Agent unterstützt
int load_grid(const char *filename, World *w, GameConfig *c) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("Error opening file %s\n", filename);
        return 0; // Fail
    }
    
    int rows, cols, max_pop;
    int max_rounds = 1000; // Default legacy
    int delay_ms = 100;    // Default legacy
    long timestamp = 0;
    int version = 1;

    char line[256];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return 0;
    }
    
    // Try parsing as v2
    int items = sscanf(line, "%d %ld %d %d %d %d %d", 
                       &version, &timestamp, &rows, &cols, &max_pop, &max_rounds, &delay_ms);
                       
    if (items == 7 && version == 2) {
        printf("Detected Protocol v2. Timestamp: %ld\n", timestamp);
    } else {
        // Fallback to legacy v1
        items = sscanf(line, "%d %d %d", &rows, &cols, &max_pop);
        if (items == 3) {
            version = 1;
            printf("Detected Legacy Format (v1).\n");
        } else {
            printf("Error: Unknown file format.\n");
            fclose(f);
            return 0;
        }
    }
    
    // Check if loaded config matches current world size
    if (rows != c->rows || cols != c->cols) {
        printf("Resizing world from %dx%d to %dx%d...\n", c->rows, c->cols, rows, cols);
        free(w->grid);
        // PADDED GRID allocation: (rows + 2) * (cols + 2)
        w->grid = (int*)calloc((rows + 2) * (cols + 2), sizeof(int));
        if (!w->grid) {
            printf("Error: Failed to allocate memory for new grid size.\n");
            fclose(f);
            return 0;
        }
        w->rows = rows;
        w->cols = cols;
        c->rows = rows;
        c->cols = cols;
    }
    
    // Update config
    c->max_population = max_pop;
    c->max_rounds = max_rounds;
    c->delay_ms = delay_ms;
    
    // Clear grid (all cells including ghost borders)
    int stride = cols + 2;
    for(int i=0; i < (rows + 2) * (cols + 2); i++) w->grid[i] = DEAD;
    c->current_blue_pop = 0;
    c->current_red_pop = 0;
    
    int r_in, c_in, team;
    while (fscanf(f, "%d %d %d", &r_in, &c_in, &team) == 3) {
        if (r_in >= 0 && r_in < rows && c_in >= 0 && c_in < cols) {
            // Map 0-indexed file coordinates to padded grid indices (r+1, c+1)
            int idx = (r_in + 1) * stride + (c_in + 1);
            w->grid[idx] = team;
            if (team == TEAM_RED) c->current_red_pop++;
            if (team == TEAM_BLUE) c->current_blue_pop++;
        }
    }
    
    fclose(f);
    printf("Loaded from %s\n", filename);
    return 1; // Success
}

// KI-Agent unterstützt
int list_protocol_files(const char *dir_path, ProtocolInfo **out_list) {
    DIR *d = opendir(dir_path);
    if (!d) {
        printf("Error: Could not open directory %s\n", dir_path);
        return 0;
    }

    struct dirent *dir;
    int count = 0;
    int capacity = 10;
    ProtocolInfo *list = malloc(capacity * sizeof(ProtocolInfo));

    while ((dir = readdir(d)) != NULL) {
        // Filter for .bio extension
        char *ext = strrchr(dir->d_name, '.');
        if (ext && strcmp(ext, ".bio") == 0) {
            ProtocolInfo info;
            snprintf(info.filename, sizeof(info.filename), "%s", dir->d_name);
            snprintf(info.filepath, sizeof(info.filepath), "%s/%s", dir_path, dir->d_name);
            
            if (load_protocol_metadata(info.filepath, &info)) {
                if (count >= capacity) {
                    capacity *= 2;
                    list = realloc(list, capacity * sizeof(ProtocolInfo));
                }
                list[count++] = info;
            }
        }
    }
    closedir(d);

    if (count > 0) {
        qsort(list, count, sizeof(ProtocolInfo), compare_protocol_info);
    }

    *out_list = list;
    return count;
}

// KI-Agent unterstützt
void append_protocol_result(const char *filename, int winner, int red, int blue) {
    FILE *f = fopen(filename, "a"); // "a" for append
    if (!f) return;
    
    fprintf(f, "\n#RESULTS\n");
    fprintf(f, "WINNER %d\n", winner);
    fprintf(f, "RED %d\n", red);
    fprintf(f, "BLUE %d\n", blue);
    
    fclose(f);
    printf("Appended results to %s\n", filename);
}

// KI-Agent unterstützt
int load_protocol_metadata(const char *filename, ProtocolInfo *info) {
    FILE *f = fopen(filename, "r");
    if (!f) return 0;

    // Initialize defaults
    info->has_results = 0;
    info->winner = 0;
    info->final_red = 0;
    info->final_blue = 0;

    char line[256];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return 0;
    }

    int version;
    int items = sscanf(line, "%d %ld %d %d %d %d", 
                       &version, &info->timestamp, &info->rows, &info->cols, 
                       &info->max_population, &info->max_rounds);

    if (items == 6 && version == 2) {
        // Header OK
    } else {
        // Try legacy
        items = sscanf(line, "%d %d %d", &info->rows, &info->cols, &info->max_population);
        if (items == 3) {
            info->timestamp = 0;
            info->max_rounds = 1000;
        } else {
            fclose(f);
            return 0; // Unknown format
        }
    }

    // Scan for #RESULTS footer
    // We continue reading line by line until end
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "#RESULTS", 8) == 0) {
            info->has_results = 1;
        }
        if (info->has_results) {
            if (strncmp(line, "WINNER", 6) == 0) sscanf(line, "WINNER %d", &info->winner);
            if (strncmp(line, "RED", 3) == 0) sscanf(line, "RED %d", &info->final_red);
            if (strncmp(line, "BLUE", 4) == 0) sscanf(line, "BLUE %d", &info->final_blue);
        }
    }

    fclose(f);
    return 1;
}

// KI-Agent unterstützt
void export_stats_md(const char *filename, GameConfig *c, int winner) {
    FILE *f = fopen(filename, "w");
    if (!f) return; 
    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    fprintf(f, "# Biotope Game Results\n\n");
    fprintf(f, "**Date:** %02d.%02d.%04d %02d:%02d\n\n", 
            t->tm_mday, t->tm_mon+1, t->tm_year+1900, t->tm_hour, t->tm_min);
            
    fprintf(f, "## Configuration\n");
    fprintf(f, "- Grid: %dx%d\n", c->rows, c->cols);
    fprintf(f, "- Max Population: %d\n", c->max_population);
    fprintf(f, "- Max Rounds: %d\n\n", c->max_rounds);
    
    fprintf(f, "## Final Score\n");
    fprintf(f, "- **Red Team:** %d\n", c->current_red_pop);
    fprintf(f, "- **Blue Team:** %d\n\n", c->current_blue_pop);
    
    fprintf(f, "## Result\n");
    if (winner == TEAM_RED) fprintf(f, "**Winner: RED TEAM**\n");
    else if (winner == TEAM_BLUE) fprintf(f, "**Winner: BLUE TEAM**\n");
    else fprintf(f, "**DRAW**\n");
    
    fclose(f);
    printf("Stats exported to %s\n", filename);
}