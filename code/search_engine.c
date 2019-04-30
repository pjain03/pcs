#include "search_engine.h"

// Globals
 Keyword *keywords_table = NULL; // Initializing keywords hashtable



void extract_keywords(HTTPResponse **response, CacheObject *cache_entry) {
    char *clean_body = NULL;
    int word_len;
    unsigned int num_words;
    WordCount *vocab = NULL;    // Initialize hashtable 
    WordCount *curr = NULL;
    WordCount *ptr = NULL;
    WordCount *temp = NULL;

    char *curr_word;
    StopWord *stop_words = create_stop_words_set();

    // Stripping out any binary characters or non-alphabetical characters
    clean_body = strip_content((*response)->body, (*response)->body_length);
    fprintf(stderr, "%s", clean_body);
    // Get the first word
    curr_word = strtok(clean_body, " ");

    // Create vocabulary of this document (hashtable of all unique words and their count)
    while (curr_word != NULL) {
        word_len = strlen(curr_word);
        if (word_len > 2) { // Only store words that are more than two character long - gives more meaningful results
            // Stop word removal, which is removing most common words in English
            if(!is_stop_word(stop_words, curr_word)) { // Only continue if it is not a stop word. Ignore stop words
                // Find the word first
                HASH_FIND_STR(vocab, curr_word, curr);
                if (curr == NULL) {
                    // If word not found, add to hashtable 
                    curr = malloc(sizeof(WordCount));
                    curr->word = (char *)malloc(word_len + 1);
                    memcpy(curr->word, curr_word, word_len);
                    curr->word[word_len] = '\0';                    
                    curr->count = 1; 
                    HASH_ADD_KEYPTR(hh, vocab, curr->word, word_len, curr);
                } else {
                    curr->count = curr->count + 1;
                }
            }
        }
        curr_word = strtok(NULL, " ");
    }

    
    num_words = HASH_COUNT(vocab); // Number of unique words
        fprintf(stderr, "num words is %d\n", num_words);
    // Find top most common words in vocabulary
    HASH_SORT(vocab, count_sort); // Sort by most common
    int i; 
    for(ptr = vocab, i = 0; ptr != NULL && i < 5; ptr = (ptr->hh.next), i++) {
        Keyword *curr_keyword;
        CountEntry *count_entry = malloc(sizeof(CountEntry));
        // Normalize the count (term frequency) by dividing by number of unique words in the data = size of table
        count_entry->tf = (float) ptr->count / (float) num_words;
        count_entry->cache_entry = cache_entry;
        count_entry->next = NULL;
        fprintf(stderr, "keyword is %s\n", ptr->word);
        fprintf(stderr, "count is %d\n", ptr->count);
        fprintf(stderr, "adding a tf of %f\n", count_entry->tf);
        fprintf(stderr, "linking to cache entry %s\n", count_entry->cache_entry->url);
        // Find keyword in the keywords table
        HASH_FIND_STR(keywords_table, ptr->word, curr_keyword);
        if (curr_keyword == NULL) {
            // New keyword, so add an entry into the keywords table
            curr_keyword = malloc(sizeof(Keyword));
            if ((curr_keyword->word = (char *)malloc(strlen(ptr->word))) == NULL) {
                error_out("Couldn't malloc!");
            }
            memcpy(curr_keyword->word, ptr->word, strlen(ptr->word));


            curr_keyword->count_entry_list = count_entry;

            HASH_ADD_KEYPTR(hh, keywords_table, curr_keyword->word, strlen(curr_keyword->word), curr_keyword);
        } else {
            // Already used keyword, so add the cache entry into the linked list

            add_count_entry_to_keyword(curr_keyword, count_entry);
        }
    }


    // Free each word, then free hashtable
    HASH_ITER(hh, vocab, curr, temp) {
        free(curr->word);       // Free the word
        HASH_DEL(vocab, curr);  // Delete it (curr advances to next) 
        free(curr);             // Free the struct 
    }


    free(clean_body);
}


// Strips data to only include alphabetical characters and disregard any thing
// within < > as that is just HTML tags not relevant to the actual data
char *strip_content(char *data, int body_len) {
    char *new_data = NULL;
    char *stripped_data = NULL;
    int len = 0;
    int tag = 0;

    // Malloc-ing body length space for now, will be size body length
    // but stripping out non alphabetical characters will make size smaller
    if ((new_data = (char *) malloc(body_len)) == NULL) {
        error_out("Couldn't malloc!");
    }

    for (int i = 0; i < body_len; i++) {
        
        if (data[i] == '<') {
            tag = 1;
        } else if (data[i] == '>') {
            tag = 0;
        }
        // Between ranges of A ~ Z (65 ~ 90) or a ~ z (97 ~ 122)
        else if (((data[i] >= 65 && data[i] <= 90) || (data[i] >= 97 && data[i] <= 122))
              && tag == 0) { // Characters are within range and not in a tag
            // Copy character over
            new_data[len] = data[i];
            len++;
        } else {
            if (i != 0) { // Don't add in repeating spaces
                if (new_data[len - 1] != ' ') {
                    new_data[len] = ' '; 
                    len++;
                }
            }
        }
    }


    // Put stripped data into right length buffer
    if ((stripped_data = (char *) malloc(len + 1)) == NULL) { // + 1 for null terminator
        error_out("Couldn't malloc!");
    }
 
    memcpy(stripped_data, new_data, len);
    // Add null terminator to end
    stripped_data[len] = '\0';
    len++;

    free(new_data);

    return stripped_data;
}


StopWord *create_stop_words_set() {
    StopWord *stop_words = NULL;    // Initialize hashtable 

    const char *stop_words_list[STOP_WORDS_LIST_SIZE] = {"i", "me", "my", "myself", "we", "our", "ours", "ourselves", "you", "you're", "you've", "you'll", "you'd", "your", "yours", "yourself", "yourselves", "he", "him", "his", "himself", "she", "she's", "her", "hers", "herself", "it", "it's", "its", "itself", "they", "them", "their", "theirs", "themselves", "what", "which", "who", "whom", "this", "that", "that'll", "these", "those", "am", "is", "are", "was", "were", "be", "been", "being", "have", "has", "had", "having", "do", "does", "did", "doing", "a", "an", "the", "and", "but", "if", "or", "because", "as", "until", "while", "of", "at", "by", "for", "with", "about", "against", "between", "into", "through", "during", "before", "after", "above", "below", "to", "from", "up", "down", "in", "out", "on", "off", "over", "under", "again", "further", "then", "once", "here", "there", "when", "where", "why", "how", "all", "any", "both", "each", "few", "more", "most", "other", "some", "such", "no", "nor", "not", "only", "own", "same", "so", "than", "too", "very", "s", "t", "can", "will", "just", "don", "don't", "should", "should've", "now", "d", "ll", "m", "o", "re", "ve", "y", "ain", "aren", "aren't", "couldn", "couldn't", "didn", "didn't", "doesn", "doesn't", "hadn", "hadn't", "hasn", "hasn't", "haven", "haven't", "isn", "isn't", "ma", "mightn", "mightn't", "mustn", "mustn't", "needn", "needn't", "shan", "shan't", "shouldn", "shouldn't", "wasn", "wasn't", "weren", "weren't", "won", "won't", "wouldn", "wouldn't"};

    // Make list into hashtable (set) for O(1) lookup
    for (int i = 0; i < STOP_WORDS_LIST_SIZE; i++) {
        StopWord *curr;

        curr = malloc(sizeof(StopWord));
        curr->word = (char *)malloc(strlen(stop_words_list[i]));
        memcpy(curr->word, stop_words_list[i], strlen(stop_words_list[i]));
        HASH_ADD_KEYPTR(hh, stop_words, curr->word, strlen(stop_words_list[i]), curr); 
    }

    return stop_words;
}


int is_stop_word(StopWord *stop_words, char *word) {
    StopWord *temp;

    HASH_FIND_STR(stop_words, word, temp);  

    if (temp == NULL) {
        return 0; // False, not a stop word. Could not be found in set
    }

    return 1; // True, it is a stop word    
}

int count_sort(WordCount *word1, WordCount *word2) {
    /* compare a to b (cast a and b appropriately)
    * return (int) -1 if (a < b)
    * return (int)  0 if (a == b)
    * return (int)  1 if (a > b)
    */

   return word2->count - word1->count;
}
int tf_sort(URLTF_Table *t1, URLTF_Table *t2) {
    /* compare a to b (cast a and b appropriately)
    * return (int) -1 if (a < b)
    * return (int)  0 if (a == b)
    * return (int)  1 if (a > b)
    */

   return t2->tf - t1->tf;
}


void add_count_entry_to_keyword(Keyword *curr_keyword, CountEntry *count_entry) {
    CountEntry *curr = curr_keyword->count_entry_list;

    // Traverse until end of list
    while (curr->next != NULL) {
        curr = curr->next;
    }

    curr->next = count_entry;
}


// Main entry point function
URLResults *find_relevant_urls(char *keywords) {
	URLResults *final_results;
	char* curr_word;
    int num_keywords;
    URLTF *single_keyword_results_list;
    URLTF *curr;
    URLTF *all_relevant;
    URLTF_Table *results = NULL;
    int idx = 0;
    int num_lists = 0;

    // Do the same text parsing as the body
    keywords = strip_content(keywords, strlen(keywords));

    // Making a copy of keywords because find_num_keywords uses strtok which changes the string
    char copy[strlen(keywords)];
    strcpy(copy, keywords);
    num_keywords = find_num_keywords(copy);

    // Each keyword will generate a list of URLs (URLTF)
    URLTF *result_lists[num_keywords];

    fprintf(stderr, "number of keywords is %d\n", num_keywords);
    fprintf(stderr, "keywords are %s\n", keywords);
    // Go through each keyword one by one, generate the url list for each, and add the url list into results_list
	curr_word = strtok(keywords, " ");
	while (curr_word != NULL) {
		fprintf(stderr, "keyword searching for is %s\n", curr_word);
		single_keyword_results_list = find_relevant_urls_from_single_keyword(curr_word);
		
        // Add the result list from one keyword into the array of results
        result_lists[idx] = single_keyword_results_list;
        if (result_lists[idx] != NULL) {
            num_lists++;
        }

        idx++;
		curr_word = strtok(NULL, " ");
	}

    fprintf(stderr, "calculating relevant urls now\n");
    // Algorithm to aggregate the key words results, giving more weight to those documents that contain multiple keywords
    all_relevant = calculate_all_relevant(result_lists, num_lists);

    // Sort the list so most relevant is on top and put it into URLTF_table results
    sort_list_by_tf(&results, all_relevant);


    // Put the first five URLs into final_results
    final_results = malloc(sizeof(URLResults));
    int i;
    URLTF_Table *s;

    for(s = results, i = 0; s != NULL && i < NUM_TOP_RESULTS; s = s->hh.next, i++) {
        fprintf(stderr, "tf %f: url %s\n", s->tf, s->url);
        final_results->urls[i] = malloc(strlen(s->url) + 1); // + 1 for null terminator
        memcpy(final_results->urls[i], s->url, strlen(s->url));
        final_results->urls[i][strlen(s->url)] = '\0';
    }

    while (i < NUM_TOP_RESULTS) {
        // If final results isn't fully populated, point urls[i] = NULL
        final_results->urls[i] = NULL;
        i++;
    }

    return final_results;
}

URLTF *find_relevant_urls_from_single_keyword(char *keyword) {
	Keyword *found_keyword;
	URLTF *results;
	URLTF *curr = NULL;
	CountEntry *entry;
	int url_len;
	HASH_FIND_STR(keywords_table, keyword, found_keyword); 
	int i = 0;

	if (found_keyword != NULL) {
		fprintf(stderr, "Found keyword in keywords_table\n");
		entry = found_keyword->count_entry_list;
		// Add entries, up to NUM_TOP_RESULTS, into url list
		while(entry != NULL && i < NUM_TOP_RESULTS) {
			// Add the entry into the head of the url list
			curr = malloc(sizeof(URLTF)); 
			curr->tf = found_keyword->count_entry_list->tf;
			url_len = strlen(found_keyword->count_entry_list->cache_entry->url);
			curr->url = malloc(url_len + 1); // + 1 for null terminator
			memcpy(curr->url, found_keyword->count_entry_list->cache_entry->url, url_len);
			curr->url[url_len] = '\0';
            curr->next = results;
			results = curr;
			i++; 
			fprintf(stderr, "keyword found from %s\n", curr->url);
			fprintf(stderr, "with a tf of %f\n", curr->tf);
			entry = entry->next;
		}
	} else {
		results = NULL; // keyword is not in keywords_table
	}

	return results;
}


int find_num_keywords(char *keywords) {
    char *word;
    int count = 0; 

	word = strtok(keywords, " ");

    while (word != NULL) {
        count++;
        word = strtok(NULL, " ");
    }

    return count;
}


URLTF *calculate_all_relevant(URLTF **result_lists, int num_lists) {
    URLTF *inter = result_lists[0]; // Start with the first list
    URLTF *not_inter = NULL;
    URLTF *total = result_lists[0]; // Start with the first list

    if (num_lists > 1) { // If only one list, then those are the only relevant urls
        for (int i = 1; i < num_lists; i++) {
            total = inter;
            fprintf(stderr, "about to find inter\n");
            inter = find_inter(total, result_lists[i]);
            not_inter = find_diff(total, result_lists[i]);

            // Add the not_inter list behind the inter list (inter becomes total list)
            merge(&inter, not_inter);
        }
    }

    return total;
}

URLTF *find_inter(URLTF *l1, URLTF *l2) {
    // Create a hashtable of l1, and search through l2 to find entries of same URL
    URLTF_Table *url_set = NULL; // The hashtable that acts as a set
    URLTF_Table *url_table_entry;
    URLTF *curr = l1;
    URLTF *inter = NULL;
    URLTF *new_entry = NULL;

    // Place the urls into the set
    while (curr != NULL) {
        fprintf(stderr, "adding url to set %s\n", curr->url);
        url_table_entry = malloc(sizeof(URLTF_Table));
        url_table_entry->url = malloc(strlen(curr->url) + 1);
        url_table_entry->tf = curr->tf;

        memcpy(url_table_entry->url, curr->url, strlen(curr->url));
        url_table_entry->url[strlen(curr->url)] = '\0';

        HASH_ADD_KEYPTR(hh, url_set, url_table_entry->url, strlen(curr->url), url_table_entry);
        curr = curr->next;
    }

    // Check for intersection of the set and list with O(1) lookup

    curr = l2;
    while (curr != NULL) {
        HASH_FIND_STR(url_set, curr->url, url_table_entry);
        fprintf(stderr, "currently trying to find %s in set\n",curr->url);
        // If found match in set
        if (url_table_entry != NULL) {
            fprintf(stderr, "found match in set \n");
            new_entry = malloc(sizeof(URLTF));
            new_entry->url = malloc(strlen(curr->url) + 1);
            new_entry->tf = (curr->tf * url_table_entry->tf) / 2; // New tf is the average
            memcpy(new_entry->url, curr->url, strlen(curr->url));
            new_entry->url[strlen(curr->url)] = '\0';
            // Add to the inter list at the head
            new_entry->next = inter;
            inter = new_entry;
        } 
        curr = curr->next;
    }

    if (inter == NULL) {
        fprintf(stderr, "inter is NULL\n");

    } else {fprintf(stderr, "first url of found intersection is %s\n", inter->url);}
    
    return inter;
}


URLTF *find_diff(URLTF *l1, URLTF *l2) {
    // Create a hashtable of l1, and search through l2 to find entries of same URL
    URLTF_Table *url_set = NULL; // The hashtable that acts as a set
    URLTF_Table *url_table_entry;
    URLTF *curr = l1;
    URLTF *diff = NULL;
    URLTF *new_entry = NULL;

    // Place the urls into the set
    while (curr != NULL) {
        url_table_entry = malloc(sizeof(URLTF_Table));
        url_table_entry->url = malloc(strlen(curr->url) + 1);
        url_table_entry->tf = curr->tf;

        memcpy(url_table_entry->url, curr->url, strlen(curr->url));
        url_table_entry->url[strlen(url_table_entry->url)] = '\0';

        HASH_ADD_KEYPTR(hh, url_set, url_table_entry->url, strlen(url_table_entry->url), url_table_entry);
        curr = curr->next;
    }

    // Check for intersection of the set and list with O(1) lookup

    curr = l2;

    while (curr != NULL) {
        HASH_FIND_STR(url_set, curr->url, url_table_entry);

        // If did not find match in set
        if (url_table_entry == NULL) {
            new_entry = malloc(sizeof(URLTF));
            new_entry->url = malloc(strlen(curr->url) + 1);
            new_entry->tf = curr->tf;
            memcpy(new_entry->url, curr->url, strlen(curr->url));
            new_entry->url[strlen(curr->url)] = '\0';
            // Add to the inter list at the head
            new_entry->next = diff;
            diff = new_entry;
        }
        curr = curr->next;
    }

    return diff;
}


void merge(URLTF **inter, URLTF *not_inter) {
    URLTF *curr = (*inter);

    if ((*inter) == NULL && not_inter != NULL) {
        (*inter) = not_inter;
    } else if ((*inter) != NULL && not_inter == NULL) {

    } else { // both are nonempty lists

        while (curr->next != NULL) {
            curr = curr->next;
        }

        curr->next = not_inter;
    }

}

void sort_list_by_tf(URLTF_Table **results, URLTF *all_relevant) {
    URLTF *curr = all_relevant;
    URLTF_Table *url_table_entry;
    // Put all the relevant entries into the results hashtable
    while (curr != NULL) {
        fprintf(stderr, "putting entries into results hashtable, %lu\n", strlen(curr->url));
        url_table_entry = malloc(sizeof(URLTF_Table));
        url_table_entry->url = malloc(strlen(curr->url) + 1);
        url_table_entry->tf = curr->tf;

        memcpy(url_table_entry->url, curr->url, strlen(curr->url));
        url_table_entry->url[strlen(curr->url)] = '\0';
        fprintf(stderr, "--------- ending length is %lu\n", strlen(url_table_entry->url));
        HASH_ADD_KEYPTR(hh, (*results), url_table_entry->url, strlen(url_table_entry->url), url_table_entry);
        curr = curr->next;
    }

    HASH_SORT((*results), tf_sort); // Sort by most common
}
