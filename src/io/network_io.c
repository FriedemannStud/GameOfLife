#include "network_io.h"
#include <pthread.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../vendor/cJSON/cJSON.h"

// KI-Agent unterstützt: Thread-safe networking module using libcurl

#define BACKEND_URL "http://localhost:8000"

static LeaderboardData g_leaderboard;
static HighlightData g_highlights;
static pthread_mutex_t g_network_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    char* data;
    size_t size;
} ResponseBuffer;

static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    ResponseBuffer* mem = (ResponseBuffer*)userp;

    char* ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) {
        /* Out of memory! */
        fprintf(stderr, "Network: Out of memory (realloc failed)\n");
        return 0; 
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

static void* fetch_leaderboard_thread(void* arg) {
    (void)arg;
    CURL* curl_handle;
    CURLcode res;
    ResponseBuffer chunk = { .data = malloc(1), .size = 0 };

    curl_handle = curl_easy_init();
    if (curl_handle) {
        char url[256];
        snprintf(url, sizeof(url), "%s/api/leaderboard", BACKEND_URL);
        
        curl_easy_setopt(curl_handle, CURLOPT_URL, url);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&chunk);
        curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        res = curl_easy_perform(curl_handle);

        if (res == CURLE_OK) {
            long response_code;
            curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
            
            if (response_code == 200) {
                cJSON* root = cJSON_Parse(chunk.data);
                if (root) {
                    cJSON* lb_array = cJSON_GetObjectItemCaseSensitive(root, "leaderboard");
                    if (cJSON_IsArray(lb_array)) {
                        pthread_mutex_lock(&g_network_mutex);
                        g_leaderboard.count = 0;
                        int size = cJSON_GetArraySize(lb_array);
                        for (int i = 0; i < size && i < MAX_LEADERBOARD_ENTRIES; i++) {
                            cJSON* item = cJSON_GetArrayItem(lb_array, i);
                            cJSON* name = cJSON_GetObjectItemCaseSensitive(item, "name");
                            cJSON* wr   = cJSON_GetObjectItemCaseSensitive(item, "win_rate");
                            cJSON* w    = cJSON_GetObjectItemCaseSensitive(item, "wins");
                            cJSON* d    = cJSON_GetObjectItemCaseSensitive(item, "draws");
                            cJSON* l    = cJSON_GetObjectItemCaseSensitive(item, "losses");
                            cJSON* asg  = cJSON_GetObjectItemCaseSensitive(item, "avg_stable_generation");
                            cJSON* seed = cJSON_GetObjectItemCaseSensitive(item, "seed");

                            if (cJSON_IsString(name) && cJSON_IsNumber(wr)) {
                                strncpy(g_leaderboard.entries[i].name, name->valuestring, MAX_NAME_LENGTH - 1);
                                g_leaderboard.entries[i].name[MAX_NAME_LENGTH - 1] = '\0';
                                g_leaderboard.entries[i].win_rate = (float)wr->valuedouble;
                                g_leaderboard.entries[i].wins   = cJSON_IsNumber(w) ? w->valueint : 0;
                                g_leaderboard.entries[i].draws  = cJSON_IsNumber(d) ? d->valueint : 0;
                                g_leaderboard.entries[i].losses = cJSON_IsNumber(l) ? l->valueint : 0;
                                g_leaderboard.entries[i].avg_stable_generation = cJSON_IsNumber(asg) ? (float)asg->valuedouble : 0.0f;

                                // KI-Agent unterstützt: Parse dense 8x8 start config for icon (ADR-0025)
                                for (int b = 0; b < GRID_SIZE_8X8; b++)
                                    g_leaderboard.entries[i].seed[b] = 0;
                                if (cJSON_IsArray(seed)) {
                                    int n = cJSON_GetArraySize(seed);
                                    for (int b = 0; b < GRID_SIZE_8X8 && b < n; b++)
                                        g_leaderboard.entries[i].seed[b] = cJSON_GetArrayItem(seed, b)->valueint;
                                }
                                g_leaderboard.count++;
                            }
                        }
                        g_leaderboard.is_ready = true;
                        pthread_mutex_unlock(&g_network_mutex);
                        printf("Leaderboard: Parsed %d entries\n", g_leaderboard.count);
                    } else {
                        printf("Leaderboard: Unexpected JSON format (no array)\n");
                    }
                    cJSON_Delete(root);
                } else {
                    printf("Leaderboard: JSON Parse Error\n");
                }
            } else {
                printf("Leaderboard: Server returned HTTP %ld\n", response_code);
            }
        } else {
            fprintf(stderr, "Leaderboard: curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl_handle);
    }
    
    free(chunk.data);
    return NULL;
}

static void* fetch_highlights_thread(void* arg) {
    (void)arg;
    CURL* curl_handle;
    CURLcode res;
    ResponseBuffer chunk = { .data = malloc(1), .size = 0 };

    curl_handle = curl_easy_init();
    if (curl_handle) {
        char url[256];
        snprintf(url, sizeof(url), "%s/api/epoch/highlights", BACKEND_URL);
        
        curl_easy_setopt(curl_handle, CURLOPT_URL, url);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&chunk);
        curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        res = curl_easy_perform(curl_handle);

        if (res == CURLE_OK) {
            long response_code;
            curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);

            if (response_code == 200) {
                cJSON* root = cJSON_Parse(chunk.data);
                if (root) {
                    cJSON* h_array = cJSON_GetObjectItemCaseSensitive(root, "highlights");
                    if (cJSON_IsArray(h_array)) {
                        pthread_mutex_lock(&g_network_mutex);
                        g_highlights.count = 0;
                        int size = cJSON_GetArraySize(h_array);
                        cJSON* eid = cJSON_GetObjectItemCaseSensitive(root, "epoch_id");
                        if (cJSON_IsString(eid)) {
                            strncpy(g_highlights.epoch_id, eid->valuestring, sizeof(g_highlights.epoch_id) - 1);
                            g_highlights.epoch_id[sizeof(g_highlights.epoch_id) - 1] = '\0';
                        } else {
                            g_highlights.epoch_id[0] = '\0';
                        }
                        for (int i = 0; i < size && i < MAX_HIGHLIGHT_MATCHES; i++) {  // KI-Agent unterstützt: named constant — single source of truth (ADR-0024)
                            cJSON* item = cJSON_GetArrayItem(h_array, i);
                            cJSON* red_name = cJSON_GetObjectItemCaseSensitive(item, "red_name");
                            cJSON* blue_name = cJSON_GetObjectItemCaseSensitive(item, "blue_name");
                            cJSON* red_seed = cJSON_GetObjectItemCaseSensitive(item, "red_seed");
                            cJSON* blue_seed = cJSON_GetObjectItemCaseSensitive(item, "blue_seed");
                            cJSON* metric = cJSON_GetObjectItemCaseSensitive(item, "metric_type");

                            if (cJSON_IsString(red_name) && cJSON_IsString(blue_name) && 
                                cJSON_IsArray(red_seed) && cJSON_IsArray(blue_seed)) {
                                
                                strncpy(g_highlights.matches[i].participant_red, red_name->valuestring, MAX_NAME_LENGTH - 1);
                                g_highlights.matches[i].participant_red[MAX_NAME_LENGTH - 1] = '\0';
                                strncpy(g_highlights.matches[i].participant_blue, blue_name->valuestring, MAX_NAME_LENGTH - 1);
                                g_highlights.matches[i].participant_blue[MAX_NAME_LENGTH - 1] = '\0';
                                
                                // KI-Agent unterstützt: Translate internal metric IDs to human-readable labels (ADR-0021)
                                if (cJSON_IsString(metric)) {
                                    const char *raw = metric->valuestring;
                                    const char *label = raw; // fallback: show as-is
                                    if (strcmp(raw, "activity_sum") == 0)  label = "MOST VOLATILE";
                                    else if (strcmp(raw, "duration") == 0) label = "LONGEST MATCH";
                                    strncpy(g_highlights.matches[i].metric_reason, label, 63);
                                    g_highlights.matches[i].metric_reason[63] = '\0';
                                }

                                for (int b = 0; b < 64 && b < cJSON_GetArraySize(red_seed); b++) {
                                    g_highlights.matches[i].seed_red[b] = cJSON_GetArrayItem(red_seed, b)->valueint;
                                }
                                for (int b = 0; b < 64 && b < cJSON_GetArraySize(blue_seed); b++) {
                                    g_highlights.matches[i].seed_blue[b] = cJSON_GetArrayItem(blue_seed, b)->valueint;
                                }
                                g_highlights.count++;
                            }
                        }
                        g_highlights.is_ready = true;
                        pthread_mutex_unlock(&g_network_mutex);
                        printf("Highlights: Parsed %d matches\n", g_highlights.count);
                    } else {
                        printf("Highlights: Unexpected JSON format (no array)\n");
                    }
                    cJSON_Delete(root);
                } else {
                    printf("Highlights: JSON Parse Error\n");
                }
            } else {
                printf("Highlights: Server returned HTTP %ld\n", response_code);
            }
        } else {
            fprintf(stderr, "Highlights: curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl_handle);
    }
    
    free(chunk.data);
    return NULL;
}

void network_init(void) {
    curl_global_init(CURL_GLOBAL_ALL);
    
    pthread_mutex_lock(&g_network_mutex);
    g_leaderboard.is_ready = false;
    g_highlights.is_ready = false;
    pthread_mutex_unlock(&g_network_mutex);
    
    printf("Network Module Initialized (libcurl)\n");
}

void network_cleanup(void) {
    curl_global_cleanup();
    pthread_mutex_destroy(&g_network_mutex);
    printf("Network Module Cleaned Up\n");
}

// Scaffolding for getters (Thread-safe)
bool network_get_leaderboard(LeaderboardData* out_data) {
    bool ready = false;
    pthread_mutex_lock(&g_network_mutex);
    if (g_leaderboard.is_ready) {
        memcpy(out_data, &g_leaderboard, sizeof(LeaderboardData));
        g_leaderboard.is_ready = false;
        ready = true;
    }
    pthread_mutex_unlock(&g_network_mutex);
    return ready;
}

bool network_get_highlights(HighlightData* out_data) {
    bool ready = false;
    pthread_mutex_lock(&g_network_mutex);
    if (g_highlights.is_ready) {
        memcpy(out_data, &g_highlights, sizeof(HighlightData));
        g_highlights.is_ready = false;
        ready = true;
    }
    pthread_mutex_unlock(&g_network_mutex);
    return ready;
}

// Scaffolding for async triggers (to be implemented in Phase 5 & 6)
void network_fetch_leaderboard_async(void) {
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, fetch_leaderboard_thread, NULL) != 0) {
        fprintf(stderr, "Failed to create leaderboard fetch thread\n");
    } else {
        pthread_detach(thread_id);
    }
}

void network_fetch_highlights_async(void) {
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, fetch_highlights_thread, NULL) != 0) {
        fprintf(stderr, "Failed to create highlights fetch thread\n");
    } else {
        pthread_detach(thread_id);
    }
}
