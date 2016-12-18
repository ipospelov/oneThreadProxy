#include "header.h"
/*
void handle(int sig)
{
    connection_list *q = head;
    while(q != NULL){
        connection_list *t = q->next;
        remove(q);
        q = t;
    }
    close(listenfd);
    exit(0);
}*/
void set_descriptors(fd_set *readfs, fd_set *writefs, int listenfd){
    connection_list *q = head;
    FD_ZERO(readfs);
    FD_ZERO(writefs);
    FD_SET(listenfd, readfs);

    while(q != NULL){
        connection_list *t = q->next;
        if((q->data_toclient < 0 && q->data_toremote<=0) || q->data_toclient <= 0 && q->data_toremote<0 ){
            //printf("removing client socket: %d, server socket: %d\n",q->client_socket,q->remote_socket);
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
        q = t;
    }
}


int main(int argc, char** argv){
    if(argc != 2){
        printf("Wrong parameters");
        return 0;
    }
    int lport;
    int status;
    int new_connection_fd;
    struct sockaddr_in lhost_addr, newclient_addr;
    fd_set	readfs, writefs;
    fd_max = 0;

    //signal(SIGTERM, handle);
    lport=atoi(argv[1]); //port to listen

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;     // ipv4
    hints.ai_socktype = SOCK_STREAM; // TCP stream-sockets
    hints.ai_flags = AI_PASSIVE;     // put my ip automatically\

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
    //int flag = 0;

    while (true){
        set_descriptors(&readfs, &writefs, listenfd);
        ready_fd = select(fd_max, &readfs, &writefs, NULL, NULL);
        if(ready_fd == 0){
            continue;
        } else if(ready_fd < 0){
            perror("select errror:");
            exit(1);
        }
        //printf("%d\n",time(NULL));
        handle_descriptors(&readfs,&writefs);
        if(FD_ISSET(listenfd,&readfs)){
            socklen_t addrlen = sizeof(newclient_addr);
            //if(flag == 1)
            ///    continue;
            new_connection_fd = accept(listenfd, (struct sockaddr *)&newclient_addr, &addrlen);
            //printf("new connection from port: %d\n",newclient_addr.sin_port);
            if(new_connection_fd < 0){
                perror("accept error:");
                exit(2);
            }
            set_fd_max(new_connection_fd);
            add(new_connection_fd);
            //flag = 1;
        }
    }

}