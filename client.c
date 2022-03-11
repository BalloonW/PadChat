#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>


#define ADDRESS "127.0.0.1"
#define PORT 8884

#define Name_length 32                //folosit pentru lungimea maxima a username-ului
#define Message_length 1024           //folosit pentru lungimea maxima a unui mesaj



//O copie a fisierului primit de la server, care va contine toate usernameurile
char *filename = "clientFile.txt";

//numarul returnat de functia socket() (descriptorul soclului)
int socket_d;


//functia de copiere a fisierului primit de la server
void write_file(int socket_d){
	
	int n;
	FILE *f;
	char buffer[Message_length];

	f = fopen(filename, "w");
	if(f == NULL){
		perror("File error");
		exit(0);
	}

	while(1){
		n = recv(socket_d, buffer, Message_length, 0);
		if(n<=0){
			break;
			return;
		}
		fprintf(f, "%s", buffer);
		bzero(buffer, Message_length);
	}
	return;
}

//functia de verificare a usernameului introdus
int verify_username(char username[Name_length]){
	
	FILE *f;
	char names[Name_length];
	
	f = fopen(filename, "r");
	if(f == NULL){
		perror("File error");
		exit(0);
	}

	//se parcurge fisierul si se compara fiecare username cu cel introdus de la tastatura
	while(fgets(names, Name_length, f)){
		if(strcmp(names, username) == 0){
			return 0;
		}
		bzero(names, Name_length);
	}

	return 1;

}

//mesajul pe care dorim sa-l trimitem serverului
char message_to_send[Message_length];
//mesajul primit de la server
char recieved_message[Message_length];


//Se va ocupa de trimiterea mesajelor
void send_message_thread(){

	while(1){
		printf("\n Type your message here: ");
		fgets(message_to_send, Message_length, stdin);
	
		if( send(socket_d , message_to_send , strlen(message_to_send) , 0) < 0){
			puts("Send failed");
			return;
		}

		bzero(message_to_send, Message_length);
	}

}


//se va ocupa de receptionarea mesajelor
void recieve_message_thread(){
	
	int recieved_length; 
	
	while(1){
	
		recieved_length = recv(socket_d, recieved_message, Message_length, 0);
		if(recieved_length > 0){
			printf("%s\n", recieved_message);
		}

		bzero(recieved_message, Message_length);
	}
}



int main(int argc , char *argv[]){

	struct sockaddr_in server;
	char Username[Name_length], Password[Name_length];

	//Creare socket
	socket_d = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_d == -1){
		printf("Could not create socket");
	}

	server.sin_addr.s_addr = inet_addr(ADDRESS);
	server.sin_family = AF_INET;
	server.sin_port = htons( PORT );

	//Conectarea la server
	if (connect(socket_d , (struct sockaddr *)&server , sizeof(server)) < 0){
		perror("connect failed. Error");
		return 1;
	}

	printf("Connected to the server\n");
	

	//am comentat aici pana va fi implementata pe partea de server trimiterea fisieului cu username-urile
	//write_file(sock);

	printf("\nEnter your username: ");
	fgets(Username, Name_length, stdin);
	

	//verificarea valabilitatii username-ului
	while( verify_username(Username) == 0 ){
		bzero(Username, Name_length);
		printf("\n===== Username already in use =====\n");
		printf("\nPlease enter a new username: ");
		fgets(Username, 32, stdin);
	}

	printf("\nEnter your password: ");
	fgets(Password, Name_length, stdin);


	//trimite username-ul catre server
	if( send(socket_d , Username , strlen(Username) , 0) < 0){
		puts("Send failed");
		return 1;
	}


	//trimite parola catre  server
	if( send(socket_d , Password , strlen(Password) , 0) < 0){
		puts("Send failed");
		return 1;
	}

	//vom folosi fire de executie pentru a putea trimite si receptiona mesaje in paralel

	//fir de executie pentru trimiterea mesajelor
	pthread_t send_message;

	if(pthread_create(&send_message, NULL, (void *) send_message_thread, NULL ) != 0){
		printf("PTHREAD ERROR\n\n");
		return EXIT_FAILURE;
	}


	//fir de executie pentru receptionarea mesajelor
	pthread_t recieve_message;
	if(pthread_create(&recieve_message, NULL, (void *) recieve_message_thread, NULL ) != 0){
		printf("PTHREAD ERROR\n\n");
		return EXIT_FAILURE;
	}
	
	//merge chatul pana dam ctrl+c
	while(1){}

	close(socket_d);
	return 0;
}