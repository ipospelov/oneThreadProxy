#include "cache_list.h"

#define DATA_SIZE 4096
typedef struct connection_list{
    int client_socket, remote_socket;
    int data_toremote, data_toclient;
    char buf_toremote[DATA_SIZE];
    char buf_toclient[DATA_SIZE];
    connection_list *next, *prev;
    char request_buf[DATA_SIZE];
    int request_buf_size;
    cache_list *server_answer;
};

connection_list *head, *tail;
int fd_max;

void set_fd_max(int fd){
    if(fd_max < fd+1)
        fd_max = fd + 1;
}

void add(int new_connection_fd){
    connection_list *q = (connection_list*)malloc(sizeof(connection_list));
    q->client_socket = new_connection_fd;
    q->remote_socket = -1;
    q->prev = NULL;
    if(head != NULL) {
        q->next = head;
        head->prev = q;
    }else{
        q->next = NULL;
        tail = q;
    }
    q->data_toclient = 0;
    q->data_toremote = 0;
    memset(q->buf_toremote, 0, sizeof(q->buf_toremote));
    memset(q->buf_toclient, 0, sizeof(q->buf_toclient));
    memset(q->request_buf, 0, sizeof(q->request_buf));
    q->request_buf_size = 0;
    head = q;
}

int list_size(){
    connection_list *q = head;
    int rez = 0;
    while(q != NULL){
        rez++;
        q=q->next;
    }
    return rez;
}

void remove(connection_list *q){
    //int k =0;
    //printf("size of list: %d \n",list_size());
    //printf("removing %d\n",q);
    if(q == head && q == tail) {
        head = NULL;
        tail = NULL;
    } else if(q == head) {
        head = head->next;
        head->prev = NULL;
    } else if(q == tail) {
        tail = tail->prev;
        tail->next = NULL;
    } else {
        q->prev->next = q->next;
        q->next->prev = q->prev;
    }
    close(q->client_socket);
    close(q->remote_socket);
    free(q);
}