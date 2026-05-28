#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "../src/io/network_io.h"

// KI-Agent unterstützt: Network validation test for live system integration

int main(void) {
    printf("=== Starting Network Integration Test ===\n");

    network_init();

    // 1. Fetch Leaderboard
    printf("Fetching leaderboard asynchronously...\n");
    network_fetch_leaderboard_async();

    LeaderboardData lb = { 0 };
    int attempts = 0;
    bool success = false;
    while (attempts < 50) { // 5 seconds timeout
        usleep(100000); // 100ms
        if (network_get_leaderboard(&lb)) {
            success = true;
            break;
        }
        attempts++;
    }

    assert(success);
    assert(lb.count > 0);
    printf("Leaderboard PASSED. Received %d entries:\n", lb.count);
    for (int i = 0; i < lb.count; i++) {
        printf("  %d. %s (%d Elo, win rate %.2f%%)\n", i + 1, lb.entries[i].name, lb.entries[i].elo, lb.entries[i].win_rate);
    }

    // 2. Fetch Highlights
    printf("Fetching highlights asynchronously...\n");
    network_fetch_highlights_async();

    HighlightData hd = { 0 };
    attempts = 0;
    success = false;
    while (attempts < 50) { // 5 seconds timeout
        usleep(100000); // 100ms
        if (network_get_highlights(&hd)) {
            success = true;
            break;
        }
        attempts++;
    }

    assert(success);
    assert(hd.count > 0);
    printf("Highlights PASSED. Received %d highlight matches:\n", hd.count);
    for (int i = 0; i < hd.count; i++) {
        printf("  Match %d: %s vs %s (%s)\n", i + 1, hd.matches[i].participant_red, hd.matches[i].participant_blue, hd.matches[i].metric_reason);
    }

    network_cleanup();

    printf("=== All Network Integration Tests PASSED ===\n");
    return 0;
}
