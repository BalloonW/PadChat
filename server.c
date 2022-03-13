#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <errno.h> 
#include <string.h> 
#include <pthread.h> 
#include <sys/types.h> 
#include <signal.h> 

#define MAX_CLIENTS 10
#define BUFFER 2048
#define NAME_LEN 32
#define PASS_LEN 32 

static _Atomic unsigned int clients_count = 0; 
static int uid = 10; 

// client structure 
typedef struct 
{
    struct sockaddr_in address; 
    int sockfd;
    int uid; // unique user id
    char NAME[NAME_LEN];
    char PASSWORD[PASS_LEN];
}Client;

Client *clients[MAX_CLIENTS];

// using to send the message to clients
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


void str_overwrite_stdout();
void str_trim_lf(char* arr, int length);
void queue_add(client_t *client);
void queue_remove(int uid);
// function used to send messages to all the clients
// par: uid - user id of the client who sens the message
void send_message(char *message, int uid)
void print_ip_addr(struct sockaddr_in addr);

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        perror("Usage %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *ip = "127.0.0.1";
    int port = atoi(argv[1]);

    int option = 1; 
    int listenfd = 0; 
    int coonfd = 0; 
    struct sockaddr_in server_addr; 
    struct sockaddr_in client_addr; 
    pthread_t tid; 

    // socket settings 
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET; 
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    // signals (software generic interrupts)
    signal(SIGPIPE, SIG_IGN);

    if(setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char*) &option), sizeof(option)) < 0) 
    {
        perror("ERROR: setsockopt\n");
        return EXIT_FAILURE;
    }

    // bind
    if(bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("ERROR: bind\n");
        return EXIT_FAILURE;
    }

    // listen (10 backlogs)
    if(listen(listenfd, 10) < 0)
    {
        perror("ERROR: listen\n");
        return EXIT_FAILURE; 
    }

    // reaching this step means server works
    printf("___WELCOME TO THE CHATROOM___");

    while(1) 
    {
        socklen_t client_len = sizeof(client_addr);
        coonfd = accept(listenfd, (struct sockaddr*)&client_addr, client_len));

        // max number of clients check
        if((clients_count + 1) == MAX_CLIENTS)
        {
            perror("MAXIMUM NUMBER OF CLIENTS CONNECTED\nCONNECTION REJECTED");
            print_ip_addr(client_addr);
            close(coonfd);
            continue;
        }

        // client settings
        Client *clnt = (Client*)malloc(sizeof(Client));
        clnt->address = client_addr; 
        clnt->sockfd = coonfd;
        clnt->uid = uid++;

        // add client to the queue
        queue_add(clnt);

        pthread_create(&tid, NULL, &handle_client, (void*)clnt);

        // reduce CPU usage
        sleep(1);
    }



    return EXIT_SUCCESS; 
}

void print_ip_addr(struct sockaddr_in addr)
{
    printf("%d.%d.%d.%d", 
    addr.sin_addr.s_addr & 0xff, 
    (addr.sin_addr.s_addr & 0xff00) >> 8,
    (addr.sin_addr.s_addr & 0xff0000) >> 16,
    (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

void str_overwrite_stdout()
{
    printf("\r%s", "> ");
    fflush(stdout);
}

void str_trim_lf(char* arr, int length)
{
    for(int i = 0; i < length; i++)
    {
        if(arr[i] == "\n")
        {
            arr[i] = "\0";
            break;
        }
    }
}

void queue_add(client_t *client) 
{
    pthread_mutex_lock(&clients_mutex);

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(!clients[i])
        {
            clients[i] = client;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void queue_remove(int uid)
{
    pthread_mutex_lock(&clients_mutex);

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(clients[i])
        {
            if(clients[i]->uid == uid)
            {
                clients[i] = NULL;
                break;
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void send_message(char *message, int uid)
{
    pthread_mutex_lock(&clients_mutex);

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(clients[i])
        {
            if(clients[i]->uid != uid)
            {
                if((write(clients[i]->sockfd, message, strlen(message))) < 0)
                {
                    perror("ERROR: write to descriptor failed\n");
                    break;
                }
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}