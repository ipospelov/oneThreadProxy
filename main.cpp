#include <stdio.h>
#include <cstdlib>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <zconf.h>
#include "connection_list.h"
struct addrinfo hints;
const char * rport = "80";


void set_descriptors(fd_set *readfs, fd_set *writefs, int listenfd){
    connection_list *q = head;
    FD_ZERO(readfs);
    FD_ZERO(writefs);
    FD_SET(listenfd, readfs);

    while(q != NULL){
        if((q->data_toclient < 0 && q->data_toremote<=0) || q->data_toclient <= 0 && q->data_toremote<0 ){
            remove(q);
        } else {
            if(q->data_toremote ==0)
                FD_SET(q->client_socket, readfs);
            if(q->data_toclient==0 && q->remote_socket != -1)
                FD_SET(q->remote_socket, readfs);
            if(q->data_toremote>0  && q->remote_socket != -1)
                FD_SET(q->remote_socket, writefs);
            if(q->data_toclient>0)
                FD_SET(q->client_socket, writefs);
        }
        q = q->next;
    }

}

void move_bufdata(char *buf, int last_datasize, int writed){
    memmove(buf, buf + writed, last_datasize - writed);
}
/*
void set_proxy_connection(connection_list *q){
    if(strstr(q->request_buf,"keep-alive") != NULL) {
        char *pc = strstr(q->request_buf, "Proxy-Connection");
        pc += strlen("Proxy-Connection: "); //сдвиг на значение proxy-connection
        strcpy(pc,"close");
        memcpy(pc + strlen("close"),)
    }
}*/

void set_remote_socket(connection_list *q) {
    //printf("request: %s\n",q->request_buf);
    struct addrinfo *servinfo;
    char *hostnamebegin = strstr(q->request_buf,"Host");
    char *hostnameend;
    hostnamebegin += strlen("Host: "); //сдвиг на начало имени хоста(Host: 'name')
    hostnameend = strstr(hostnamebegin,"\n");
    char *hostname = (char *)malloc(hostnameend - hostnamebegin);
    //printf("end %d begin %d\n",hostnameend,hostnamebegin);
    //printf("end %s\n",hostnameend);
    memcpy(hostname,hostnamebegin,hostnameend - hostnamebegin - 1);
    hostname[hostnameend - hostnamebegin - 1] = '\0';
    //printf("hostname: %s\n",hostname);
    //exit(1);

    q->remote_socket = socket(AF_INET, SOCK_STREAM, 0);
    getaddrinfo(hostname, rport, &hints, &servinfo);
    int res = connect(q->remote_socket, servinfo->ai_addr, sizeof(*servinfo->ai_addr));
    //printf("trying to connect to: %s\n",hostname);
    if(res != 0){
        perror("connecting to server error:");
        close(q->remote_socket);
        exit(3);
    }else{
        printf("connected to: %s\n", hostname);
    }
    set_fd_max(q->remote_socket);
    free(hostname);
    //exit(4);
}

void handle_descriptors(fd_set *readfs, fd_set *writefs){
    connection_list *q = head;
    int writed;
    while (q != NULL){
        if(q->data_toremote == 0 && FD_ISSET(q->client_socket,readfs)){
            q->request_buf_size += read(q->client_socket,q->request_buf + q->request_buf_size,sizeof(DATA_SIZE - q->request_buf_size));

            if(q->request_buf_size == 0)
                q->data_toremote = -1;

            if( strstr(q->request_buf,"\r\n\r\n") != NULL){ //если запрос получен полностью
                printf("%d\n",q->request_buf_size);
                //printf("%s\n",q->request_buf);
                //printf("reques:\n%s",q->request_buf);
                set_remote_socket(q);
                memcpy(q->buf_toremote,q->request_buf,q->request_buf_size);
                q->data_toremote = q->request_buf_size;
                q->request_buf_size = 0;
                memset(q->request_buf, 0, sizeof(q->request_buf));
            }
            /*if(q->remote_socket < 0)
                set_remote_socket(q);
            */

            /*snprintf(mes, q->data_toremote,"%.2x",q->buf_toremote);
            printf("%d\n",q->data_toremote);
            printf("%s\n",q->buf_toremote);*/

        }
        if(q->data_toclient == 0 && FD_ISSET(q->remote_socket,readfs)){
            q->data_toclient = read(q->remote_socket,q->buf_toclient,sizeof(q->buf_toclient));
            if(q->data_toclient == 0)
                q->data_toclient = -1;
        }
        if(q->data_toremote > 0 && FD_ISSET(q->remote_socket,writefs)){
            writed = write(q->remote_socket,q->buf_toremote,q->data_toremote);
            if(writed == -1)
                q->data_toclient = -1;
            else if(writed == q->data_toremote)
                q->data_toremote = 0;
            else{
                move_bufdata(q->buf_toremote,q->data_toremote,writed);
                q->data_toremote -= writed;
                printf("!!!!!!!!!!!!!!!!!!");
            }
        }
        if(q->data_toclient > 0 && FD_ISSET(q->client_socket,writefs)){
            writed = write(q->client_socket,q->buf_toclient,q->data_toclient);
            if(writed == -1)
                q->data_toremote = -1;
            else if(writed == q->data_toclient)
                q->data_toclient = 0;
            else{
                move_bufdata(q->buf_toclient,q->data_toclient,writed);
                q->data_toclient -= writed;
            }
        }

        q = q->next;
    }
}



int main(int argc, char** argv){
    if(argc != 4){
        printf("Wrong parameters");
        return 0;
    }
    int listenfd;
    int lport;
    int status;
    int new_connection_fd;
   // struct addrinfo *servinfo;
    struct sockaddr_in rhost_addr, lhost_addr, newclient_addr;
    fd_set	readfs, writefs;
    fd_max = 0;

    lport=atoi(argv[1]); //port to listen
    //rport=atoi(argv[3]);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;     // ipv4
    hints.ai_socktype = SOCK_STREAM; // TCP stream-sockets
    hints.ai_flags = AI_PASSIVE;     // put my ip automatically

/*    if ((status = getaddrinfo(argv[2], argv[3], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %sn", gai_strerror(status));
        exit(1);
    }*/


    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    set_fd_max(listenfd);

    lhost_addr.sin_family      = AF_INET;
    lhost_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    lhost_addr.sin_port        = htons(lport);

    if (bind(listenfd, (struct sockaddr *) &lhost_addr, sizeof(lhost_addr))) {  //связывание сокета с адресом и номером порта
        perror("bind error:");
        return 0;
    }

    listen(listenfd,SOMAXCONN);

    int ready_fd;

    while (true){
        set_descriptors(&readfs, &writefs, listenfd);
        ready_fd = select(fd_max, &readfs, &writefs, NULL, NULL);
        if(ready_fd == 0){
            continue;
        } else if(ready_fd < 0){
            perror("select errror:");
            exit(1);
        }
        //printf("current number of fd: %d\n",ready_fd);
        handle_descriptors(&readfs,&writefs);
        if(FD_ISSET(listenfd,&readfs)){
            socklen_t addrlen = sizeof(newclient_addr);
            new_connection_fd = accept(listenfd, (struct sockaddr *)&newclient_addr, &addrlen);
            printf("new connection from port: %d\n",newclient_addr.sin_port);
            if(new_connection_fd < 0){
                perror("accept error:");
                exit(2);
            }
            set_fd_max(new_connection_fd);
            add(new_connection_fd);
        }
    }



    //freeaddrinfo(servinfo);

}