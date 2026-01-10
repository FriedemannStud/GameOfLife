#include <stdio.h>
#include <time.h>
#include "file_io.h"

// KI-Agent unterstützt
int save_grid(const char *filename, World *w, GameConfig *c) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        printf("Error saving file %s\n", filename);
        return 0;
    }
    
    // Header: Rows Cols MaxPop
    fprintf(f, "%d %d %d\n", c->rows, c->cols, c->max_population);
    
    // Save live cells only: r c team
    for(int i=0; i < w->rows * w->cols; i++) {
        if (w->grid[i] != DEAD) {
            int r = i / w->cols;
            int c_idx = i % w->cols;
            fprintf(f, "%d %d %d\n", r, c_idx, w->grid[i]);
        }
    }
    
    fclose(f);
    printf("Saved to %s\n", filename);
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
    if (fscanf(f, "%d %d %d", &rows, &cols, &max_pop) != 3) {
        fclose(f);
        return 0;
    }
    
    // Check if loaded config matches current world size (simplification)
    // Ideally we would resize the world here, but for now we enforce matching size
    // or we just warn. Let's assume user configured size correctly or we just update config
    // if we haven't allocated yet. But here world is already allocated in GUI.
    // So we only load cells if dimensions match.
    if (rows != c->rows || cols != c->cols) {
        printf("Error: File dimensions (%dx%d) do not match current config (%dx%d)\n", rows, cols, c->rows, c->cols);
        fclose(f);
        return 0;
    }
    
    // Clear current grid
    for(int i=0; i<w->rows * w->cols; i++) w->grid[i] = DEAD;
    c->current_blue_pop = 0;
    c->current_red_pop = 0;
    
    int r, c_idx, team;
    while (fscanf(f, "%d %d %d", &r, &c_idx, &team) == 3) {
        if (r >= 0 && r < rows && c_idx >= 0 && c_idx < cols) {
            int idx = r * cols + c_idx;
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
