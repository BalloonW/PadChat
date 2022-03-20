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

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048

static _Atomic unsigned int cli_count = 0;
static int uid = 10;
int action = 0;
FILE *data_base = NULL;
int type = -1;

/* Client structure */
typedef struct
{
	struct sockaddr_in address;
	int sockfd;
	int uid;
} 
client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void str_overwrite_stdout() 
{
    printf("\r%s", "> ");
    fflush(stdout);
}

void str_trim_lf (char* arr, int length) 
{
	int i;
	for (i = 0; i < length; i++) // trim \n
	{
		if (arr[i] == '\n') 
		{
			arr[i] = '\0';
			break;
		}
	}
}

void print_client_addr(struct sockaddr_in addr)
{
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

/* Add clients to queue */
void queue_add(client_t *cl){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i)
	{
		if(!clients[i])
		{
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Remove clients to queue */
void queue_remove(int uid)
{
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i)
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

/* Send message to all clients except sender */
void send_message(char *s, int uid)
{
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i<MAX_CLIENTS; ++i)
	{
		if(clients[i])
		{
			if(clients[i]->uid != uid)
			{
				if(write(clients[i]->sockfd, s, strlen(s)) < 0)
				{
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* decrypt message according to defined protocol */
char *decrypt_message(char *msg)
{
	type = msg[0] - '0';
	int i = 1;
	for (i = 1; BUFFER_SZ; i++)
	{
		msg[i - 1] = msg[i];
	}
	msg[i - 1] = '\0';
	return msg;
}

/* Handle all communication with the client */
void *handle_client(void *arg){
	char buff_out[BUFFER_SZ];
	int leave_flag = 0;

	cli_count++;
	client_t *cli = (client_t *)arg;

	bzero(buff_out, BUFFER_SZ);

	while(1)
	{
		if (leave_flag) 
		{
			break;
		}

		if (recv(cli->sockfd, buff_out, BUFFER_SZ, 0) <= 0)
		{
			printf("Didn't enter the message.\n");
			leave_flag = 1;
		} 
		else
		{
			char *msg = decrypt_message(buff_out);
			// printf("%s", msg);
			char left_chat_msg[200];

			char name[32]; 
			char password[32];
			char name_aux[32];
			char password_aux[32];
			char user_already_exits[100] = "4user_already_exists";
			char aux[BUFFER_SZ + 10];
			char not_valid_user[100] = "3user_is_not_valid";
			char new_user_to_the_chat_annoucment[100];

			int user_exists = 0;
			FILE *data_base;

			switch(type) 
			{
				case 0:
					sprintf(left_chat_msg, "5%s left the chat\n\n", msg);
					send_message(left_chat_msg, cli->uid);
					break;
				case 1:
					// log in 
					// verifica daca este in db	
					// trimite eroare

					/* open data base */
					data_base = fopen("data_base.txt", "a+");
					if (data_base == NULL)
					{
						printf("ERORR: Couldn't open database!");
						fclose(data_base);
						exit(111);
					}

					sscanf(msg, "%s\t%s\n", name_aux, password_aux);
					
					// check in database
					while (fscanf(data_base, "%s\t%s\n", name, password) == 2) 
					{
						if(strcmp(name_aux, name) == 0)
						{
							if(strcmp(password_aux, password) == 0)
							{
								user_exists = 1;
								sprintf(new_user_to_the_chat_annoucment, "5%s joined to the party baby!\n\n", name_aux);
								send_message(new_user_to_the_chat_annoucment, cli->uid);
								break;
							}
						}
						
					}

					fclose(data_base);

					if (!user_exists)
					{
						
						printf("user is not valid\n");
						// send message to client
						pthread_mutex_lock(&clients_mutex);
						send(cli->sockfd, not_valid_user, 100, 0);
						pthread_mutex_unlock(&clients_mutex);
					}

					break;
				case 2:
					// creare cont (sign up)
					// VERIFICA DACA ESTE IN DB
					// TRIMITE EROARE DACA II, SAU SUCCES DACA NU II
					
					data_base = fopen("data_base.txt", "a+");
					if (data_base == NULL)
					{
						printf("ERORR: Couldn't open database!");
						fclose(data_base);
						exit(111);
					}

					sscanf(msg, "%s\t%s\n", name_aux, password_aux);
					
					while ( fscanf(data_base, "%s\t%s\n", name, password) == 2) 
					{
						if(strcmp(name_aux, name) == 0)
						{
							user_exists = 1;
							// send message to client
							pthread_mutex_lock(&clients_mutex);
							send(cli->sockfd, user_already_exits, 100, 0);
							pthread_mutex_unlock(&clients_mutex);
							break;
						}
					}

					if(!user_exists)
					{
						fprintf(data_base, "%s\t%s\n", name_aux, password_aux);
					}
					fclose(data_base);

					break;
				case 5:
					sprintf(aux, "5%s\n", buff_out);
					send_message(aux, cli->uid);
					break;
				default:
					printf("error in decrypt method!\n");
					break;
			}
		}
		bzero(buff_out, BUFFER_SZ);
	}

  /* Delete client from queue and yield thread */
	close(cli->sockfd);
	queue_remove(cli->uid);
	free(cli);
	cli_count--;
  	pthread_detach(pthread_self());

	return NULL;
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);
	int option = 1;
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	pthread_t tid;

	/* Socket settings */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(port);

  	/* Ignore pipe signals */
	signal(SIGPIPE, SIG_IGN);

	if (setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR), (char*)&option,sizeof(option)) < 0)
	{
		perror("ERROR: setsockopt failed");
    	return EXIT_FAILURE;
	}

	/* Bind */
	if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
	{
		perror("ERROR: Socket binding failed");
		return EXIT_FAILURE;
	}

  	/* Listen */
  	if (listen(listenfd, 10) < 0) 
	{
		perror("ERROR: Socket listening failed");
		return EXIT_FAILURE;
	}

	printf("=== WELCOME TO THE CHATROOM ===\n");

	while (1)
	{
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		/* Check if max clients is reached */
		if ((cli_count + 1) == MAX_CLIENTS)
		{
			printf("Max clients reached. Rejected: ");
			print_client_addr(cli_addr);
			printf(":%d\n", cli_addr.sin_port);
			close(connfd);
			continue;
		}

		/* Client settings */
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;

		/* Add client to the queue and fork thread */
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

		/* Reduce CPU usage */
		sleep(1);
	}

	return EXIT_SUCCESS;
}
