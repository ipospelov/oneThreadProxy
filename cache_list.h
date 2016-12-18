#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
//#include <sys/types.h>

//#include "connection_list.h"
//#include "header.h"
#define MAX_CACHE_DATA 28192
struct cache_list{
    char *uri;
    int cache_size;
    cache_list *next;
    char cache[MAX_CACHE_DATA];
    int expected_cache_size;
};
cache_list *cache_head = NULL;

void casche_size(){
    cache_list *q = cache_head;
    int k;
    while (q != NULL){
        k++;
        q = q->next;
    }
    printf("cache size : %d\n",k);
}

bool cache_contains_uri(char *uri){
    //return false;
    cache_list *q = cache_head;
    while (q != NULL){
        //printf("compairing strs: %s, %s\n",uri,q->uri);
        if(strcmp(uri,q->uri) == 0) {
            //printf("yey\n");
            return true;
        }
        q = q->next;
    }
    return false;
}

char * get_uri(char * request){
    char * result;
    char *uri_begin = strstr(request," ") + 1;
    char *uri_end = strstr(uri_begin," ");
    result = (char*)malloc(uri_end - uri_begin + 1);
    memcpy(result,uri_begin, uri_end - uri_begin);
    result[uri_end - uri_begin] = '\0';
    //printf("URI: %s\n",result);
    return result;
}

void put_to_cachelist(cache_list * q){
    if(cache_head == NULL){
        q->next = NULL;
        //printf("first\n");
    }else{
        q->next = cache_head;
        //printf("next\n");
    }
    cache_head = q;
}

cache_list * get_cache_page(char * uri){
    cache_list * q = cache_head;
    while(q != NULL){
        if(strcmp(q->uri,uri) == 0)
            return q;
        q = q->next;
    }
    return NULL;
}

int get_content_length(char * response) {
    char *lengthbegin = strstr(response, "Content-Length");
    lengthbegin += strlen("Content-Length: ");
    int length = atoi(lengthbegin);
    //printf("length: %d\n",length);
    return length;
}
