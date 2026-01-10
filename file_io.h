#ifndef FILE_IO_H
#define FILE_IO_H

#include "game_logic.h"
#include "gui.h" // For GameConfig struct

// KI-Agent unterstützt
int save_grid(const char *filename, World *w, GameConfig *c);
// KI-Agent unterstützt
int load_grid(const char *filename, World *w, GameConfig *c);
// KI-Agent unterstützt
void export_stats_md(const char *filename, GameConfig *c, int winner);

#endif // FILE_IO_H
