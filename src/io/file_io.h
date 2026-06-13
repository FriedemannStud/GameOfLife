#ifndef FILE_IO_H
#define FILE_IO_H

#include <time.h>
#include "core_types.h"
#include "game_logic.h"

// Struct to hold file metadata for the browser
typedef struct {
    char filename[MAX_FILENAME_LENGTH]; // e.g., "run_20260117_120000.bio"
    char filepath[MAX_PATH_LENGTH]; // Full relative path
    time_t timestamp;   // For sorting

    // Preview Data
    int rows;
    int cols;
    int max_rounds;
    int max_population;

    // Result Data (Optional)
    int has_results;    // 1 if simulation finished and results are appended
    int winner;         // 0=Draw, 1=Red, 2=Blue
    int final_red;
    int final_blue;
} ProtocolInfo;

// KI-Agent unterstützt
int save_grid(const char *filename, World *w, GameConfig *c);
// KI-Agent unterstützt
int load_grid(const char *filename, World *w, GameConfig *c);
// KI-Agent unterstützt
void append_protocol_result(const char *filename, GameConfig *c, int winner);

// KI-Agent unterstützt: JSON I/O
bool load_config_from_json(const char* filepath, GameConfig* config);
bool initialize_world_from_file(const char* filepath, World* world, Team team, char* out_player_id, char* out_nickname);
int parse_batch_file(const char* filepath, Competitor** competitors, int* count, int* max_gen);
int save_batch_results(const char* filepath, RankingScore* scores, int count, double cpu_time_used, HighlightEntry* highlights, int highlight_count);
void save_headless_results(const char* filepath, const char* winner, int gens, const char* rp_id, const char* rp_nick, int rp_pop, const char* bp_id, const char* bp_nick, int bp_pop);

// Lists .bio files in a directory. Returns count.
// Caller must free the list.
int list_protocol_files(const char *dir_path, ProtocolInfo **out_list);

// Loads just the header to preview metadata
int load_protocol_metadata(const char *filename, ProtocolInfo *info);

#endif // FILE_IO_H
