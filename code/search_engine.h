
#ifndef SEARCH_H
#define SEARCH_H

#include "ap_utilities.h"
#include "cache.h"

#define NUM_TOP_RESULTS 5

typedef struct CountEntry {
    float tf; // Term frequency 
    CacheObject *cache_entry; // Points to the cache object that has the keyword of this TF
    struct CountEntry *next; 
} CountEntry;

typedef struct Keyword {
    char *word;
    CountEntry *count_entry_list; // Linked list of entries
    UT_hash_handle hh;         /* makes this structure hashable */
} Keyword;

typedef struct WordCount {
    char *word;                    /* key */
    int count;
    UT_hash_handle hh;         /* makes this structure hashable */
} WordCount;


typedef struct StopWord {
    char *word;
    UT_hash_handle hh;         /* makes this structure hashable */
} StopWord;


typedef struct URLResults{
	char* urls[NUM_TOP_RESULTS];
} URLResults;

typedef struct URLTF { // URL and term frequency
	char* url;
	float tf;	
	struct URLTF *next;
} URLTF;


typedef struct URLTF_Table {
    char *url;
    float tf;
    UT_hash_handle hh;         /* makes this structure hashable */
} URLTF_Table;

void extract_keywords(HTTPResponse **response, CacheObject *cache_entry);
char *strip_content(char *data, int body_len);
StopWord *create_stop_words_set();
int is_stop_word(StopWord *stop_words, char *word);
int count_sort(WordCount *word1, WordCount *word2);
void add_count_entry_to_keyword(Keyword *curr_keyword, CountEntry *count_entry);
URLResults *find_relevant_urls(char *keywords);
URLTF *find_relevant_urls_from_single_keyword(char *keyword);
int find_num_keywords(char *keywords);
URLTF *calculate_all_relevant(URLTF **result_lists, int num_lists);
URLTF *find_inter(URLTF *l1, URLTF *l2);
URLTF *find_diff(URLTF *l1, URLTF *l2);
void merge(URLTF **inter, URLTF *not_inter);
void sort_list_by_tf(URLTF_Table **results, URLTF *all_relevant);
int tf_sort(URLTF_Table *t1, URLTF_Table *t2);


#endif /* SEARCH_H */
