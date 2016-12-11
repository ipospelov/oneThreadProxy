#define MAX_CACHE_DATA 8192
typedef struct cache_list{
    char *uri;
    int cache_size;
    cache_list *next;
    char cache[MAX_CACHE_DATA];
};
cache_list *cache_head;

bool contains_uri(char *uri){
    cache_list *q = cache_head;
    while (q != NULL){
        if(strcmp(uri,q->uri) == 0)
            return true;
    }
    return false;
}
