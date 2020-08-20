#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>

int main(int argc, char *argv[])
{
	int sockfd;//fd for client
	struct hostent *server;//to be for server
	int portno;//port value
	struct sockaddr_in svr_addr;//server address
	char svr_message[256];//messages from server or to server
	int max;//max fd
	int nready;
	int nread;
	fd_set fds;

	if(argc != 4)//if the user does not specify all values needed
	{
		printf("Not enough information\n");//print reason for exiting
		exit(EXIT_FAILURE);//exits program
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);//creats socket
	if(sockfd <0)
	{
		perror("socket");//if socket fails print error
		exit(EXIT_FAILURE);//exits
	}

	server = gethostbyname(argv[1]);//get the ip address
	if(server == NULL)//if none put in 
	{
		fprintf(stderr, "error: host %s not exist\n", argv[1]);//prints error to user
		exit(EXIT_FAILURE);//exits
	}

	portno = atoi(argv[2]);//changes from string to int

	bzero((char *) &svr_addr, sizeof(svr_addr));//clears svr_addr
	svr_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&svr_addr.sin_addr.s_addr, server->h_length);
	svr_addr.sin_port = htons(portno);

	if(connect(sockfd,(struct sockaddr *) &svr_addr, sizeof(svr_addr)) <0)//connects to server
	{
		perror("connect");//if ther is an error
		exit(EXIT_FAILURE);//exits 
	}

	fflush(stdout);

	puts("Welcome to the 3600 Chat Room!\n");//lets client know they are in the chatroom now

	fflush(stdout);
	max = sockfd+1;
	if(send(sockfd,argv[3], strlen(argv[3]), 0) < 0)//sends username to server
	{
		perror("send");//if unable then print error
		exit(EXIT_FAILURE);//exits 
	}

	fflush(stdout);

	while(1)//loops until user quits
	{
		fflush(stdout);

		FD_ZERO(&fds);
		FD_SET(sockfd, &fds);//for recieving purposes
		FD_SET(0, &fds);//for sending purposes

		nready = select(max, &fds, (fd_set *) 0, (fd_set *) 0, (struct timeval *) 0);

		if(FD_ISSET(sockfd, &fds))//if there is something ready to be read
		{

			bzero(svr_message, 2000);//clears the string
			if(recv(sockfd, svr_message, 2000, 0) <0)//reads in message
			{
				perror("recv");//if fails, print error
				exit(EXIT_FAILURE);//exits
			}

			if(strcmp(svr_message, "QUIT")==0)
			{
				exit(1);
			}

			printf("%s\n", svr_message);
		}
		if(FD_ISSET(0, &fds))//if they have input 
		{
			memset(&svr_message[0], 0, sizeof(svr_message));//clear message string

			nread = read(0, svr_message, sizeof(svr_message));//read in string

			if(send(sockfd,svr_message, sizeof(svr_message), 0) < 0)//send to server
			{
				perror("send");//if fails print error
				exit(EXIT_FAILURE);//exits program
			}
		}

	}

	close(sockfd);//closes fd
	return 0;
}
