// Cache module
#include "cache.h"

// Global
CacheObject *cache = NULL;


/* Returns NULL if data cannot be found */
HTTPResponse *get_data_from_cache(char *url) {
    CacheObject *curr;

    HASH_FIND_STR(cache, url, curr);

    if (curr != NULL) {
        fprintf(stderr, "Getting data from cache\n");
        curr->last_accessed = time(NULL);
        return curr->response;
    }
    
    return NULL; 
}


void add_data_to_cache(char *url, HTTPResponse *response) {
    CacheObject *curr = NULL;

    HASH_FIND_STR(cache, url, curr); // Check if already in cache - if yes, probably want to modify age?

    if (curr == NULL) { // Data is not in cache yet
        if (HASH_COUNT(cache) >= MAX_CACHE_SIZE) { // Cache is full, must evict something 
            // TODO: look for stale items first?
            lru_evict();
        }

        // Add to cache
        curr = malloc(sizeof(CacheObject));
        curr->url = strdup(url);
        curr->response = response;
        curr->last_accessed = time(NULL);
        fprintf(stderr, "Adding object %s to cache\n", url);
        HASH_ADD_KEYPTR(hh, cache, curr->url, strlen(curr->url), curr);

    }
}

void lru_evict() {
    CacheObject *curr, *lru, *tmp;

    // Set the first object to the first entry in cache
    HASH_ITER(hh, cache, lru, tmp) {
        break;
    }   

    for (curr = cache; curr != NULL; curr = curr->hh.next) {
        if (curr->last_accessed < lru->last_accessed) {
            lru = curr;
        }        
    } 
   
    HASH_DEL(cache, lru);
    free_response(lru->response);
    free(lru);   
}
