#include <cerrno>
#include "connection_list.h"
struct addrinfo hints;
const char * rport = "80";
int listenfd;

void set_remote_socket(connection_list *q) {
    if(q->remote_socket != -1)
        close(q->remote_socket);
    struct addrinfo *servinfo;
    char *hostnamebegin = strstr(q->request_buf,"Host");
    char *hostnameend;
    hostnamebegin += strlen("Host: "); //сдвиг на начало имени хоста(Host: 'name')
    hostnameend = strstr(hostnamebegin,"\n");
    char *hostname = (char *)malloc(hostnameend - hostnamebegin);
    memcpy(hostname,hostnamebegin,hostnameend - hostnamebegin - 1);
    hostname[hostnameend - hostnamebegin - 1] = '\0';
    q->remote_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    getaddrinfo(hostname, rport, &hints, &servinfo);
    int res = connect(q->remote_socket, servinfo->ai_addr, sizeof(*servinfo->ai_addr));

    if(res != 0){
        if(errno != EINPROGRESS) {
            perror("connecting to server error:");
            close(q->remote_socket);
            exit(3);
        }
    }
    set_fd_max(q->remote_socket);
    free(hostname);
    freeaddrinfo(servinfo);
}
void change_connection_type(connection_list *q){
    char *connectiontypebegin = strstr(q->request_buf,"Proxy-Connection");
    if( connectiontypebegin == NULL)
        return;
    connectiontypebegin += strlen("Proxy-Connection: ");
    strncpy(connectiontypebegin,"close",strlen("close"));
    memmove(connectiontypebegin + strlen("close"),connectiontypebegin + strlen("keep-alive"),q->request_buf + q->request_buf_size - connectiontypebegin - strlen("keep-alive"));
    q->request_buf_size-= 5;//(strlen("keep-alive") - strlen("close"));
}

void move_bufdata(char *buf, int last_datasize, int writed){
    memmove(buf, buf + writed, last_datasize - writed);
}
void handle_descriptors(fd_set *readfs, fd_set *writefs){
    connection_list *q = head;
    int writed;
    int readed;
    while (q != NULL){
        if(q->data_toremote == 0 && FD_ISSET(q->client_socket,readfs)){
            readed = read(q->client_socket,q->request_buf + q->request_buf_size, DATA_SIZE - q->request_buf_size);
            if(readed == 0) {
                q->data_toremote = -1;
                q = q->next;
                continue;
            }
            q->request_buf_size+=readed;

            if( strstr(q->request_buf,"\r\n\r\n") != NULL){ //если запрос получен полностью
                set_remote_socket(q);
                change_connection_type(q);
                char *uri = get_uri(q->request_buf);
                if( strstr(q->request_buf,"GET") != NULL && !cache_contains_uri(uri) && q->server_answer == NULL){ //если это гет-запрос
                    q->server_answer = (cache_list *) malloc(sizeof(cache_list));
                    q->server_answer->cache_size = 0;
                    q->server_answer->next = NULL;
                    q->server_answer->uri = uri;

           /*         if( !cache_contains_uri(uri) && q->server_answer == NULL) { //если нет в кэше
                        q->server_answer = (cache_list *) malloc(sizeof(cache_list));
                        q->server_answer->cache_size = 0;
                        q->server_answer->next = NULL;
                        q->server_answer->uri = uri;
                    }else{

                    }*/
                        //      q->server_answer = put_to_cachelist(uri);
                }
                memset(q->buf_toremote, 0, sizeof(q->buf_toremote)); //необязательно?
                memcpy(q->buf_toremote, q->request_buf, q->request_buf_size);
                q->data_toremote = q->request_buf_size;
                q->request_buf_size = 0;
                memset(q->request_buf, 0, sizeof(q->request_buf));
               // }
            }

        }
        if(q->data_toclient == 0 && FD_ISSET(q->remote_socket,readfs)){
            q->data_toclient = read(q->remote_socket,q->buf_toclient,sizeof(q->buf_toclient));
            char *test;
            if(q->server_answer != NULL) {
                memcpy(q->server_answer->cache + q->server_answer->cache_size, q->buf_toclient, q->data_toclient);
                q->server_answer->cache_size += q->data_toclient;
                if(q->server_answer->cache_size > 200 && (test = strstr(q->server_answer->cache,"Content-Length")) != NULL) {
                    int content_length = get_content_length(q->server_answer->cache);
                    content_length += strstr(q->server_answer->cache, "\r\n\r\n") - q->server_answer->cache + strlen("\r\n\r\n");
                    //printf("content-length: %d cache data size: %d\n",content_length,q->server_answer->cache_size);
                    if(content_length == q->server_answer->cache_size){
                        put_to_cachelist(q->server_answer);
                        printf("OK\n");
                        q->server_answer = NULL;
                   /*     cache_list *t = cache_head;
                        while (t != NULL){
                            printf("%s\n", t->uri);
                            t = t->next;
                        }*/
                    }
                    if(content_length > MAX_CACHE_DATA){
                        printf("free\n");
                        free(q->server_answer);
                        q->server_answer = NULL;
                    }
                }else{
                    free(q->server_answer);
                    q->server_answer = NULL;
                }
            }
            if(q->data_toclient == 0) {
                q->data_toclient = -1;
            }
        }
        if(q->data_toremote > 0 && FD_ISSET(q->remote_socket,writefs)){
            //printf("writing to: %d\n",q->remote_socket);
            writed = write(q->remote_socket,q->buf_toremote,q->data_toremote);
            if(writed == -1)
                q->data_toclient = -1;
            else if(writed == q->data_toremote) {
                q->data_toremote = 0;
            } else {
                move_bufdata(q->buf_toremote,q->data_toremote,writed);
                q->data_toremote -= writed;
            }
        }
        if(q->data_toclient > 0 && FD_ISSET(q->client_socket,writefs)){
            writed = write(q->client_socket,q->buf_toclient,q->data_toclient);
            if(writed == -1)
                q->data_toremote = -1;
            else if(writed == q->data_toclient) {
                q->data_toclient = 0;
                //remove(q);
            } else {
                move_bufdata(q->buf_toclient,q->data_toclient,writed);
                q->data_toclient -= writed;
            }
        }

        q = q->next;
    }
}