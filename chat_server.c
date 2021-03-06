/*
 * Copyright 2014 - 2020
 *
 * Author:      Yorick de Wid
 * Description: Simple chatroom in C
 * License:     GPLv3
 * Version:     1.5
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<linux/if_ether.h>
#include<arpa/inet.h>
#include<sys/ioctl.h>
#include <signal.h>


#define NUM_THREADS 2      //线程个数即服务器节点网卡个数

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048
char *tag="flag";
static _Atomic unsigned int cli_count = 0;
static int uid = 10;
/* Server structure */
struct server_data{
    int port;
    char *interName;
};
/* Client structure */
typedef struct {
    struct sockaddr_in addr; /* Client remote address */
    int connfd;              /* Connection file descriptor */
    int uid;                 /* Client unique identifier */
    char name[32];           /* Client name */
} client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

static char topic[BUFFER_SZ/2];

pthread_mutex_t topic_mutex = PTHREAD_MUTEX_INITIALIZER;

/* The 'strdup' function is not available in the C standard  */
char *_strdup(const char *s) {
    size_t size = strlen(s) + 1;
    char *p = malloc(size);
    if (p) {
        memcpy(p, s, size);
    }
    return p;
}

/* Add client to queue */
void queue_add(client_t *cl){
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients[i]) {
            clients[i] = cl;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

/* Delete client from queue */
void queue_delete(int uid){
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {
            if (clients[i]->uid == uid) {
                clients[i] = NULL;
                break;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

/* Send message to all clients but the sender */
void send_message(char *s, int uid){
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {
            if (clients[i]->uid != uid) {
                if (write(clients[i]->connfd, s, strlen(s)) < 0) {
                    perror("Write to descriptor failed");
                    break;
                }
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

/* Send message to all clients */
void send_message_all(char *s){
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i <MAX_CLIENTS; ++i){
        if (clients[i]) {
            if (write(clients[i]->connfd, s, strlen(s)) < 0) {
                perror("Write to descriptor failed");
                break;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

/* Send message to sender */
void send_message_self(const char *s, int connfd){
    if (write(connfd, s, strlen(s)) < 0) {
        perror("Write to descriptor failed");
        exit(-1);
    }
}

/* Send message to client */
void send_message_client(char *s, int uid){
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i){
        if (clients[i]) {
            if (clients[i]->uid == uid) {
                if (write(clients[i]->connfd, s, strlen(s))<0) {
                    perror("Write to descriptor failed");
                    break;
                }
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}
/* Send message to Selected client  数据条件发送*/ 
void send_message_select_client(char *s,int uid){
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i){
        if (clients[i]) {
            if (clients[i]->uid == uid) {
                if(strstr(s,tag)){
                    if (write(clients[i]->connfd, s, strlen(s))<0) {
                        perror("Write to descriptor failed");
                        break;
                    }   
                }
                
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

/* Send list of active clients */
void send_active_clients(int connfd){
    char s[64];

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i){
        if (clients[i]) {
            sprintf(s, "<< [%d] %s\r\n", clients[i]->uid, clients[i]->name);
            send_message_self(s, connfd);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

/* Strip CRLF */
void strip_newline(char *s){
    while (*s != '\0') {
        if (*s == '\r' || *s == '\n') {
            *s = '\0';
        }
        s++;
    }
}

/* Print ip address */
void print_client_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

/* Handle all communication with the client */
void *handle_client(void *arg){
    char buff_out[BUFFER_SZ];
    char buff_in[BUFFER_SZ / 2];
    int rlen;

    cli_count++;
    client_t *cli = (client_t *)arg;

    printf("<< accept ");
    print_client_addr(cli->addr);
    printf(" referenced by %d\n", cli->uid);

    sprintf(buff_out, "<< %s has joined\r\n", cli->name);
    send_message_all(buff_out);

    pthread_mutex_lock(&topic_mutex);
    if (strlen(topic)) {
        buff_out[0] = '\0';
        sprintf(buff_out, "<< topic: %s\r\n", topic);
        send_message_self(buff_out, cli->connfd);
    }
    pthread_mutex_unlock(&topic_mutex);

    send_message_self("<< see /help for assistance\r\n", cli->connfd);

    /* Receive input from client */
    while ((rlen = read(cli->connfd, buff_in, sizeof(buff_in) - 1)) > 0) {
        buff_in[rlen] = '\0';
        buff_out[0] = '\0';
        strip_newline(buff_in);

        /* Ignore empty buffer */
        if (!strlen(buff_in)) {
            continue;
        }

        /* Special options */
        if (buff_in[0] == '/') {
            char *command, *param;
            command = strtok(buff_in," ");
            if (!strcmp(command, "/quit")) {
                break;
            } else if (!strcmp(command, "/ping")) {
                send_message_self("<< pong\r\n", cli->connfd);
            } else if (!strcmp(command, "/topic")) {
                param = strtok(NULL, " ");
                if (param) {
                    pthread_mutex_lock(&topic_mutex);
                    topic[0] = '\0';
                    while (param != NULL) {
                        strcat(topic, param);
                        strcat(topic, " ");
                        param = strtok(NULL, " ");
                    }
                    pthread_mutex_unlock(&topic_mutex);
                    sprintf(buff_out, "<< topic changed to: %s \r\n", topic);
                    send_message_all(buff_out);
                } else {
                    send_message_self("<< message cannot be null\r\n", cli->connfd);
                }
            } else if (!strcmp(command, "/nick")) {
                param = strtok(NULL, " ");
                if (param) {
                    char *old_name = _strdup(cli->name);
                    if (!old_name) {
                        perror("Cannot allocate memory");
                        continue;
                    }
                    strncpy(cli->name, param, sizeof(cli->name));
                    cli->name[sizeof(cli->name)-1] = '\0';
                    sprintf(buff_out, "<< %s is now known as %s\r\n", old_name, cli->name);
                    free(old_name);
                    send_message_all(buff_out);
                } else {
                    send_message_self("<< name cannot be null\r\n", cli->connfd);
                }
            } else if (!strcmp(command, "/msg")) {
                param = strtok(NULL, " ");
                if (param) {
                    int uid = atoi(param);
                    param = strtok(NULL, " ");
                    if (param) {
                        sprintf(buff_out, "[源客户端][%s]", cli->name);
                        while (param != NULL) {
                            strcat(buff_out, " ");
                            strcat(buff_out, param);
                            param = strtok(NULL, " ");
                        }
                        strcat(buff_out, "\r\n");
                        send_message_client(buff_out, uid);
                    } else {
                        send_message_self("<< message cannot be null\r\n", cli->connfd);
                    }
                } else {
                    send_message_self("<< reference cannot be null\r\n", cli->connfd);
                }
            } else if(!strcmp(command, "/test")) {
                param = strtok(NULL, " ");
                if (param) {
                    int uid = atoi(param);
                    param = strtok(NULL, " ");
                    if (param) {
                        sprintf(buff_out, "[源客户端][%s]", cli->name);
                        while (param != NULL) {
                            strcat(buff_out, " ");
                            strcat(buff_out, param);
                            param = strtok(NULL, " ");
                        }
                        strcat(buff_out, "\r\n");
                        send_message_select_client(buff_out, uid);
                    } else {
                        send_message_self("<< message cannot be null\r\n", cli->connfd);
                    }
                } else {
                    send_message_self("<< reference cannot be null\r\n", cli->connfd);
                }
            } else if(!strcmp(command, "/list")) {
                sprintf(buff_out, "<< clients %d\r\n", cli_count);
                send_message_self(buff_out, cli->connfd);
                send_active_clients(cli->connfd);
            }else if (!strcmp(command, "/help")) {
                strcat(buff_out, "<< /quit     Quit chatroom\r\n");
                strcat(buff_out, "<< /ping     Server test\r\n");
                strcat(buff_out, "<< /topic    <message> Set chat topic\r\n");
                strcat(buff_out, "<< /nick     <name> Change nickname\r\n");
                strcat(buff_out, "<< /msg      <reference> <message> Send private message\r\n");
                strcat(buff_out, "<< /list     Show active clients\r\n");
                strcat(buff_out, "<< /help     Show help\r\n");
                strcat(buff_out, "<< /test     <reference> <message> delay test\r\n");
                send_message_self(buff_out, cli->connfd);
            } else {
                send_message_self("<< unknown command\r\n", cli->connfd);
            }
        } else {
            /* Send message */
            snprintf(buff_out, sizeof(buff_out), "[%s] %s\r\n", cli->name, buff_in);
            send_message(buff_out, cli->uid);
        }
    }

    /* Close connection */
    sprintf(buff_out, "<< %s has left\r\n", cli->name);
    send_message_all(buff_out);
    close(cli->connfd);

    /* Delete client from queue and yield thread */
    queue_delete(cli->uid);
    printf("<< quit ");
    print_client_addr(cli->addr);
    printf(" referenced by %d\n", cli->uid);
    free(cli);
    cli_count--;
    pthread_detach(pthread_self());

    return NULL;
}

int run_server(int port, char *interName){
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    pthread_t tid;


    

    /* Socket settings */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if (listenfd == -1)
    	{
        	printf("socket create error:%s \n", strerror(errno));
        	return EXIT_FAILURE;
    	}
    
    struct ifreq interface;
//  strncpy(interface.ifr_ifrn.ifrn_name, "tunl0",IFNAMSIZ);
    strncpy(interface.ifr_ifrn.ifrn_name,interName,IFNAMSIZ);
    if (setsockopt(listenfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&interface, sizeof(interface))< 0) {
        perror("server:SO_BINDTODEVICE failed");
        /*Deal with error... */
        return -1;
    }


    serv_addr.sin_family = AF_INET; //设置结构类型为TCP/IP
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //服务端是等待别人来连，不需要找谁的ip
    //这里写一个长量INADDR_ANY表示server上所有ip，这个一个server可能有多个ip地址，因为可能有多块网卡
    serv_addr.sin_port = htons(port);//指定一个端口号并将hosts字节型传化成Inet型字节型（大端或或者小端问题）

    /* Ignore pipe signals */
    signal(SIGPIPE, SIG_IGN);

    /* Bind */
    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
       	perror("Socket binding failed");
        return EXIT_FAILURE;
    }
    

    /* Listen */
    if (listen(listenfd, 10) < 0) {
        perror("Socket listening failed");
        return EXIT_FAILURE;
    }

    printf("<[ SERVER STARTED ]>\n");

    /* Accept clients */
    while (1) {
        socklen_t clilen = sizeof(cli_addr);
        connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

        /* Check if max clients is reached */
        if ((cli_count + 1) == MAX_CLIENTS) {
            printf("<< max clients reached\n");
            printf("<< reject ");
            print_client_addr(cli_addr);
            printf("\n");
            close(connfd);
            continue;
        }

        /* Client settings */
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->addr = cli_addr;
        cli->connfd = connfd;
        cli->uid = uid++;
        sprintf(cli->name, "%d", cli->uid);

        /* Add client to the queue and fork thread */
        queue_add(cli);
        pthread_create(&tid, NULL, &handle_client, (void*)cli);

        /* Reduce CPU usage */
        sleep(1);
    }

    return EXIT_SUCCESS;
}

int *creat_server(void *serverarg)
{
    struct server_data *my_data =  (struct server_data *) serverarg;
    int port = my_data->port;
    char *interName = my_data->interName;
    if (port == 0)
    {
        printf("port error! \n");
    }
    else
    {
        run_server(port,interName);
    }
    pthread_exit(NULL);
}
int main(int argc, char *argv[])
{
    //定义线程的 id 变量，多个变量使用数组
    pthread_t threads[NUM_THREADS];
    struct server_data sd[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; ++i) {
//        printf("请输入服务端端口号以及对应的网卡名称\n");
//        int port ;
//        char *interName;
        sd[i].port=atoi(argv[i*2+1]);
        sd[i].interName=argv[i*2+2];
    }
    for (int i = 0; i < NUM_THREADS; ++i) {
        printf("main() : creating thread, %d\n", i);
        int ret = pthread_create(&threads[i], NULL, creat_server, (void*)&(sd[i]));
        if (ret != 0) {
            printf("pthread_create error: error_code = %d\n", ret);
            exit(-1);
        }
    }

    //等各个线程退出后，进程才结束，否则进程强制结束了，线程可能还没反应过来；
    pthread_exit(NULL);
}
