#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define LENGTH 2048


// Global variables
volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[32];
char password[32];
// action can be log in(action = 1) our create account(action = 2)
int action = 0;
int type;

char *decrypt_message(char *msg)
{
	type = msg[0] - '0';
	int i = 1;
	for (i = 1; msg[i]; i++)
	{
		msg[i - 1] = msg[i];
	}
	msg[i - 2] = '\0';
	return msg;
}


char* create_message(char *msg, int type)
{
	int msg_len = strlen(msg);
	char *message_to_send = (char*)malloc((msg_len + 2));
	if (!message_to_send)
	{
		printf("memory allocation error");
		exit(EXIT_FAILURE);
	}

	sprintf(message_to_send, "%d%s", type, msg);
	return message_to_send;
}

void str_overwrite_stdout() 
{
    printf("\33[2K\r");
	printf("%s", "> ");
	fflush(stdout);
}

void str_trim_lf (char* arr, int length) 
{
	int i;
	for (i = 0; i < length; i++)
	{
		if (arr[i] == '\n') 
		{
			arr[i] = '\0';
			break;
		}
	}
}

void catch_ctrl_c_and_exit(int sig) 
{
    flag = 1;
	char *message = create_message(name, 0);
	send(sockfd, message, sizeof(message), 0);
}

void send_msg_handler() 
{
  	char message[LENGTH] = {};
	char buffer[LENGTH + 34] = {};



	while(1) 
	{
		str_overwrite_stdout();
		fgets(message, LENGTH, stdin);
		str_trim_lf(message, LENGTH);

		if (strcmp(message, "exit") == 0) 
		{
			break;
		} 
		else 
		{
			sprintf(buffer, "%s: %s\n", name, message);
			char *c_mess = create_message(buffer, 5);
			send(sockfd, c_mess, strlen(c_mess), 0);
		} 

		bzero(message, LENGTH);
		bzero(buffer, LENGTH + 32);
  	}
 	catch_ctrl_c_and_exit(2);
}

void recv_msg_handler() 
{
	char message[LENGTH] = {};
  	while (1) 
	{

		int receive = recv(sockfd, message, LENGTH, 0);
		char *aux = decrypt_message(message);
    	if (receive > 0) 
		{
			switch (type)
			{
				case 3:
					printf("Username or password is not correct!\n");
					exit(1);
					break;
				case 4:
					printf("Username already exists!\n");
					exit(1);
					break;
				case 5:
					printf("%s", aux);
					str_overwrite_stdout();
					break;
				
				default:
					break;
			}
    	} 
		else if (receive == 0) 
		{
			break;
    	} 
		else 
		{
			// -1
		}
		memset(aux, 0, 2048);
  	}
}


int main(int argc, char **argv)
{
	if(argc != 2)
	{
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);

	signal(SIGINT, catch_ctrl_c_and_exit);

	struct sockaddr_in server_addr;

	/* Socket settings */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
  	server_addr.sin_family = AF_INET;
  	server_addr.sin_addr.s_addr = inet_addr(ip);
  	server_addr.sin_port = htons(port);


    /* Connect to Server */
  	int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  	if (err == -1) 
	{
		printf("ERROR: connect\n");
		return EXIT_FAILURE;
	}

	/* Sign in or Sign up */
	printf("To sign in - press 1\n");
	printf("To sign up - press 2\n");
	scanf("%d", &action);
	getchar();

	/* If action is not valid */
	if (action < 1 || action > 2)
	{
		printf("This is not a valid action\n");
		return EXIT_FAILURE;
	}

	
	printf("Please enter your name: ");
	fgets(name, 32, stdin);
	str_trim_lf(name, strlen(name));

	if (strlen(name) > 32 || strlen(name) < 2)
	{
		printf("Name must be less than 30 and more than 2 characters.\n");
		return EXIT_FAILURE;
	}

	printf("Please enter your password: ");
	fgets(password, 32, stdin);
	str_trim_lf(password, strlen(password));

	if (strlen(password) > 32 || strlen(password) < 2)
	{
		printf("Password must be less than 30 and more than 2 characters.\n");
	}

	if (action == 1)	
	{
        char *name_password = (char*)malloc(strlen(name) + strlen(password) + 1);
        sprintf(name_password, "%s\t%s\n", name, password);
		char *message = create_message(name_password, 1);
		send(sockfd, message, 65, 0);
	}

	if (action == 2)	
	{
        char *name_password = (char*)malloc(strlen(name) + strlen(password) + 1);
        sprintf(name_password, "%s\t%s\n", name, password);
		char *message = create_message(name_password, 2);
		send(sockfd, message, 65, 0);
	}

	printf("=== WELCOME TO THE CHATROOM ===\n");

	pthread_t send_msg_thread;
  	if (pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0)
	{
		printf("ERROR: pthread\n");
    	return EXIT_FAILURE;
	}

	pthread_t recv_msg_thread;
  	if (pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0)
	{
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	while (1)
	{
		if (flag)
		{
			printf("\nBye\n");
			break;
    	}
	}

	close(sockfd);
	return EXIT_SUCCESS;
}