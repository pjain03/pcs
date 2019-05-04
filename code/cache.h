// Cache module
// key = url, value = HTTPResponse

#include "ap_utilities.h"


#define MAX_CACHE_SIZE 3

typedef struct CacheObject {
	char *url; // Key value
	HTTPResponse *response;
    time_t last_accessed;
	UT_hash_handle hh;
} CacheObject;


CacheObject *add_data_to_cache(char *url, HTTPResponse *response);
HTTPResponse *get_data_from_cache(char *url);
HTTPResponse *check_cache_capacity();
CacheObject *lru_evict();
CacheObject *mru_evict();
CacheObject *random_evict();
void evict(CacheObject *item);
void init_cache(char *eviction);
void destroy_cache();
