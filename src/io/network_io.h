#ifndef NETWORK_IO_H
#define NETWORK_IO_H

#include <stdbool.h>
#include <stdint.h>

// KI-Agent unterstützt: Network data structures for Kiosk Mode

#define MAX_LEADERBOARD_ENTRIES 20
#define MAX_NAME_LENGTH 32
#define GRID_SIZE_8X8 64

typedef struct {
    char name[MAX_NAME_LENGTH];
    int elo;
    float win_rate;
} LeaderboardEntry;

typedef struct {
    LeaderboardEntry entries[MAX_LEADERBOARD_ENTRIES];
    int count;
    bool is_ready; // Flag indicating data is fresh and ready to consume
} LeaderboardData;

typedef struct {
    char participant_red[MAX_NAME_LENGTH];
    char participant_blue[MAX_NAME_LENGTH];
    int seed_red[GRID_SIZE_8X8];   // 0 or 1
    int seed_blue[GRID_SIZE_8X8];  // 0 or 1
    char metric_reason[64];        // e.g., "Longest Match", "Highest Volatility"
} MatchHighlight;

typedef struct {
    MatchHighlight matches[4];     // Assume top 4 highlights for Multicam
    int count;
    bool is_ready;
} HighlightData;

// API functions
void network_init(void);
void network_cleanup(void);

// Async triggers
void network_fetch_leaderboard_async(void);
void network_fetch_highlights_async(void);

// Thread-safe getters
bool network_get_leaderboard(LeaderboardData* out_data);
bool network_get_highlights(HighlightData* out_data);

#endif // NETWORK_IO_H
