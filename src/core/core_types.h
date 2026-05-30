#ifndef CORE_TYPES_H
#define CORE_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

// KI-Agent unterstützt: Refactoring core types

// Team definitions
#define DEAD 0
#define TEAM_RED 1
#define TEAM_BLUE 2
#define MAX_ROUNDS 1000

typedef enum {
    TEAM_NONE = DEAD,
    TEAM_1 = TEAM_RED,
    TEAM_2 = TEAM_BLUE
} Team;

typedef enum {
    STATE_PUZZLE,   // Tutorial/Onboarding
    STATE_CONFIG,
    STATE_EDIT_RED,
    STATE_EDIT_BLUE,
    STATE_IGNITION, // Dramatic reveal countdown
    STATE_LOAD,     // For browsing protocol archive
    STATE_RUNNING,
    STATE_OBSERVER,
    STATE_FINISHED, 
    STATE_GAME_OVER,
    STATE_KIOSK_MODE
} AppState;

// KI-Agent unterstützt: Session origin tracking for back-navigation (ADR-0020)
typedef enum {
    ORIGIN_NONE,           // No active session (Kiosk or Config idle)
    ORIGIN_INTERACTIVE,    // User entered via [P] -> Config -> Edit -> Ignition
    ORIGIN_KIOSK_REPLAY    // User clicked a Multicam quadrant
} SessionOrigin;

typedef struct {
    int level;
    int target_pop;
    char hint[MAX_HINT_LENGTH];
} PuzzleConfig;

typedef struct {
    int rows;
    int cols;
    int delay_ms;
    int max_population;
    int max_rounds;
    // Current counters needed for UI display
    int current_red_pop;
    int current_blue_pop;
    int current_round;
    // UI & Logic State
    bool is_paused;
    // Telemetry Arrays (Dynamically allocated based on max_rounds)
    int *history_red_pop;
    int *history_blue_pop;
    int history_count;
} GameConfig;

typedef struct {
    int *grid; // Pointer to flat array: row-major order
    int rows;
    int cols;

    // Spatial Partitioning (NEW for Epic Scale)
    unsigned char *chunk_map; // 1D array: 1 = active, 0 = dead
    int chunk_rows;
    int chunk_cols;
} World;

// KI-Agent unterstützt: Results of a single match
typedef struct {
    int winner; // 1 = Red, 2 = Blue, 0 = Draw
    int red_final_pop;
    int blue_final_pop;
    int stable_at_generation; // 0 if never reached stable state
    long activity_sum;        // Total births and deaths during match
    int total_generations;    // Actual generations simulated
} MatchResult;

// KI-Agent unterstützt: For tracking top matches within the worker
typedef struct {
    uint64_t seed_red;
    uint64_t seed_blue;
    double score; // Metric value (duration or activity)
    char player_id_red[64];
    char player_id_blue[64];
} HighlightEntry;

// KI-Agent unterstützt: Competitor structure for batch processing
typedef struct {
    char player_id[64];
    int cells[LOCAL_GRID_SIZE][LOCAL_GRID_SIZE]; // 8x8 local grid for the starting pattern
    int living_cells;
} Competitor;

// KI-Agent unterstützt: Ranking result for a player
typedef struct {
    char player_id[64];
    double total_score;
    int matches_played;
    int wins;
    int draws;
    int losses;
    long long sum_stable_gen;
} RankingScore;

// KI-Agent unterstützt: SimulationContext encapsulating match instances
typedef struct {
    int rows;
    int cols;
    int current_generation;
    int max_generations;
    World *world_a;
    World *world_b;
    World *current_world;
    World *next_world;
    
    char participant_red[64];
    char participant_blue[64];
    
    bool is_active;
} SimulationContext;

void init_simulation_context(SimulationContext *ctx, int rows, int cols);
void free_simulation_context(SimulationContext *ctx);

#endif // CORE_TYPES_H