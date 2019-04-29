// Cache module
#include "cache.h"

// Global
CacheObject *cache = NULL;
FILE *cache_log;
char *eviction_policy;

void init_cache(char *eviction) {

    cache_log = fopen("cache.log", "w");
    if (cache_log == NULL) {
        fprintf(stderr, "Could not create cache logging file\n");
    }
    eviction_policy = eviction;
    fprintf(cache_log, "Eviction policy: %s\n\n", eviction);
    fflush(cache_log);
}

/* Returns NULL if data cannot be found */
HTTPResponse *get_data_from_cache(char *url) {
    CacheObject *curr;
    time_t s = time(NULL);
    struct tm* current_time = localtime(&s); 

    HASH_FIND_STR(cache, url, curr);

    if (curr != NULL) {
        fprintf(cache_log, "%02d:%02d:%02d FETCH %s\n", current_time->tm_hour, 
           current_time->tm_min, 
           current_time->tm_sec, url);
        fflush(cache_log);
        curr->last_accessed = time(NULL);
        return curr->response;
    }
    
    return NULL; 
}


CacheObject *add_data_to_cache(char *url, HTTPResponse *response) {
    CacheObject *curr = NULL;
    time_t s = time(NULL);
    struct tm* current_time = localtime(&s); 

    HASH_FIND_STR(cache, url, curr); // Check if already in cache - if yes, probably want to modify age?

    if (curr == NULL) { // Data is not in cache yet
        if (HASH_COUNT(cache) >= MAX_CACHE_SIZE) { // Cache is full, must evict something 
            // TODO: look for stale items first?
            if (eviction_policy == NULL) {
                lru_evict(); // LRU default
            } else {
              if (strcmp(eviction_policy, "lru") == 0) {
                    lru_evict();
                } else if (strcmp(eviction_policy, "mru") == 0) {
                    mru_evict();
                } else if (strcmp(eviction_policy, "random") == 0) {
                    random_evict();
                } else { // LRU default
                    lru_evict();
                }              
            }
        }

        // Add to cache
        curr = malloc(sizeof(CacheObject));
        curr->url = strdup(url);
        curr->response = response;
        curr->last_accessed = time(NULL);
        fprintf(cache_log, "%02d:%02d:%02d ADD %s\n", current_time->tm_hour, 
           current_time->tm_min, 
           current_time->tm_sec, url);
        fflush(cache_log);
        HASH_ADD_KEYPTR(hh, cache, curr->url, strlen(curr->url), curr);

    }

    return curr;
}

void lru_evict() {
    CacheObject *curr, *lru, *tmp;
    time_t s = time(NULL);
    struct tm* current_time = localtime(&s); 

    // Set the first object to the first entry in cache
    HASH_ITER(hh, cache, lru, tmp) {
        break;
    }   

    for (curr = cache; curr != NULL; curr = curr->hh.next) {
        if (curr->last_accessed < lru->last_accessed) {
            lru = curr;
        }        
    } 
   

    fprintf(cache_log, "%02d:%02d:%02d EVICT %s \n", current_time->tm_hour, 
           current_time->tm_min, 
           current_time->tm_sec, lru->url);
    fflush(cache_log);
    HASH_DEL(cache, lru);
    free_response(lru->response);
    free(lru);   
}

void mru_evict() {
    CacheObject *curr, *mru, *tmp;
    time_t s = time(NULL);
    struct tm* current_time = localtime(&s); 

    // Set the first object to the first entry in cache
    HASH_ITER(hh, cache, mru, tmp) {
        break;
    }   

    for (curr = cache; curr != NULL; curr = curr->hh.next) {
        if (curr->last_accessed > mru->last_accessed) {
            mru = curr;
        }        
    } 
   

    fprintf(cache_log, "%02d:%02d:%02d EVICT %s \n", current_time->tm_hour, 
           current_time->tm_min, 
           current_time->tm_sec, mru->url);
    fflush(cache_log);
    HASH_DEL(cache, mru);
    free_response(mru->response);
    free(mru);   
}

void random_evict() {
    CacheObject *curr, *random, *tmp;
    time_t s = time(NULL);
    struct tm* current_time = localtime(&s); 

    // Set the first object to the first entry in cache
    HASH_ITER(hh, cache, random, tmp) {
        break;
    }

    int count;
    count = HASH_COUNT(cache);
    srand(time(0)); // Use current time as seed for random generator 
    int n = rand() % count;
    fprintf(stderr, "--- RANDOM IS %d", n);
    for (curr = cache; curr != NULL; curr = curr->hh.next) {
        if (n > 0) {
            fprintf(stderr, "changing\n");
            random = curr;
        } 
        n--;
    } 
   

    fprintf(cache_log, "%02d:%02d:%02d EVICT %s \n", current_time->tm_hour, 
           current_time->tm_min, 
           current_time->tm_sec, random->url);
    fflush(cache_log);
    HASH_DEL(cache, random);
    free_response(random->response);
    free(random);   
}

void destroy_cache() {
    fclose(cache_log);
}


