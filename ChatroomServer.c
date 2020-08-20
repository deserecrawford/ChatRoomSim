/*
	Name: Desere Crawford
	Decription: This is the server side of a chat room simulated program. This part uses pthreads and selects to send. recieve, and manipulate data.
		This project utilizes server sockets, fds, ip addresses, and others to run the program.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/tcp.h>

struct users{//struct to hold username, blocked list,  and the fd to make things easier to access
	int fd;//fd value
	char username[50];//users chosen username
	int blacklist[30];//array of blocked fds
};

void *connection_handler(void *);//function to do the actual chat room functions
struct users clientson[30];//making the array of struct to hold number of clients
int ind=0;//index for when making new clients

int main(int argc, char *argv[])
{
	int svrfd;//server id
	int portno;//number of port using
	struct sockaddr_in svr_addr, cli_addr;//for the server and client address
	socklen_t clilen;//length of child
	int clifd;//child id
	int *newfd;//temp user fd

	if (argc != 2)//if the user inputs the port number with the program
	{
		fprintf(stderr, "Not enough arguments provided");//puts an error statment
		exit(EXIT_FAILURE);//exits
	}

	svrfd = socket(AF_INET, SOCK_STREAM, 0);//creating the server socket
	if(svrfd < 0)//if the socket is not created
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	portno = atoi(argv[1]);//changing the value from string to int

	bzero((char *) &svr_addr, sizeof(svr_addr));//clears everything from the server address
	//sets up the server socket
        svr_addr.sin_family = AF_INET;
        svr_addr.sin_addr.s_addr = INADDR_ANY;
        svr_addr.sin_port = htons(portno);

	if(bind(svrfd, (struct sockaddr *) &svr_addr, sizeof(svr_addr))<0)//if the binding does not work
	{
		perror("bind");//print an error
		exit(EXIT_FAILURE);//exit
	}

	listen(svrfd, 5);

	clilen = sizeof(cli_addr);//set the length of the child socket address

	printf("Listening on port %d\n", portno);//lets the server side know the port

	printf("waiting for connections...\n");//lets sever side know it is waiting for connections
	fflush(stdout);

	while(clifd = accept(svrfd, (struct sockaddr *)&cli_addr, &clilen))//a client socket is created
	{
		pthread_t client_thread;//declares the pthread to have multiple clients
		newfd  = malloc(sizeof(int));
		*newfd = clifd;

		if(pthread_create(&client_thread, NULL,  connection_handler, (void *) newfd)<0)//creates the thread as well with the function
		{
			perror("pthread_create");//if doesnt work print error
			exit(EXIT_FAILURE);//exits program
		}

		printf("New connection, socket fd is %d, ip is: %s, port:  %d\n", clifd, inet_ntoa(cli_addr.sin_addr), cli_addr.sin_port);//lets the server know who has connected
		fflush(stdout);

	}

	if(clifd <0)//if client not accepted 
	{
		perror("accept");//print an error
		exit(EXIT_FAILURE);//exits program
	}

	return 0;
}

void *connection_handler(void *sockfd)
{
	int cli_sockfd = *(int *) sockfd;//hold the client fd
        int nread;
        char usermessage[256];//to be used for the client inputs
	char commands[50][50];//to hold seperated comments
	char termsen[50];//a temp like string
	int temp=0;//temp for looping and finding values
	char *tok;//used to seperate strings strtok
	int j=0;//for incrementing through loops
	int place=0;//place where the current fd is in the array of structs

	bzero(usermessage, 256);	// clear out previous message

	if((nread = recv(cli_sockfd, usermessage, 256, 0)) > 0)//recieves the first thing sent from the client, in this case the username
	{
		j=0;
                tok=strtok(usermessage,"\n");//breaks the user off from the endline
                while(tok)
                {
                        strcpy(commands[j],tok);
                        j++;
                        tok=strtok(NULL,"\n");
                }

		for(int i=0; i<30;i++)//checks for duplicated usernames
		{
			if(strcmp(commands[0],clientson[i].username)==0)//if duplicated one found
			{
				strcpy(termsen,"User ");
				strcat(termsen, usermessage);
				strcat(termsen, " is already in use, leaving program\n");
				fflush(stdout);
			}
		}

		printf("User %s has logged in\n", usermessage);//lets the server know the username of who has logged in
		fflush(stdout);

		strcpy(usermessage, commands[0]);//sets the seperated value to the variable

		for(int i=0; i<sizeof(usermessage); i++)
                {
                        usermessage[i]=tolower(usermessage[i]);//loops to set everything to lowercase for easier manipulation
                }


		clientson[ind].fd=cli_sockfd;//stores the fd to the struct array
		strcpy(clientson[ind].username, usermessage);//sets the username to the struct array
		ind++;//increments the place holder for the entire array
	}

	bzero(usermessage, 256);//clears the user input
	bzero(commands, 50);
        while((nread = recv(cli_sockfd, usermessage, 256, 0)) > 0)//loops whil there is things to read in from the client
        {

		for(int i=0; i<30; i++)
		{
			if(cli_sockfd == clientson[i].fd)//finds where the current fd is in the array
			{
				place = i;//stores it into place
				break;
			}
		}


		j=0;//resets incrementer
		tok=strtok(usermessage,":\n");//breaks in user input by delimeter and endlines
		while(tok)
		{
			strcpy(commands[j],tok);//stores to commands for later use
			j++;
			tok = strtok(NULL, ":\n");
		}

		for(int i=0; i<sizeof(commands[0]); i++)
		{
			commands[0][i]=toupper(commands[0][i]);//sets the first command to all uppercase to find the correct commands
		}

		if(strcmp(commands[0],"HELP")==0)//if user inputs help
		{
			strcpy(usermessage,"Valid commands are:\nBLOCK:<euid>\nLIST\nQUIT\nSEND:(<euid>|*):<string>\nUNBLOCK:<euid>\nHELP\n");//list of commands and how they are suppose to look
			if(write(cli_sockfd, usermessage, strlen(usermessage))<0)//if unable to write to client
			{
				perror("write");//print error
				break;//break
			}
			fflush(stdout);
		}
		else if(strcmp(commands[0], "BLOCK")==0)//if the user inputs help
		{

			for(int i=0; i<sizeof(commands[1]); i++)//sets the username portion to lowercase
	                {
        	                commands[1][i]=tolower(commands[1][i]);
        	        }

			for(int i=0; i<30; i++)//loops to find the user defined username
			{
				if(strcmp(commands[1], clientson[i].username)==0)//if found set the fd to temp
				{
					temp = clientson[i].fd;;
					break;
				}
				else if((i==30-1)&&(strcmp(commands[1], clientson[i].username)!=0))//if username does not exist
				{
					strcpy(termsen, "User ");
					strcat(termsen, commands[1]);
					strcat(termsen, " does not exist\n");
					write(cli_sockfd, termsen, strlen(termsen));//let the user know the name doesnot exist
					goto ends;
				}
			}

			for(int j=0; j<30; j++)//in the blocked list for the current fd
			{
				if(clientson[place].blacklist[j]==0)//if it is empty(meaning zero) 
				{
					clientson[place].blacklist[j]=temp;//set that place to the temp variable
					goto ends;
				}
			}
			strcpy(termsen,"Done. You will not recieve messages from ");//lets the user know that the block was successful
			strcat(termsen,commands[1]);
			strcat(termsen,"\n");
			write(cli_sockfd, termsen, strlen(termsen));//writes to client

		}
		else if(strcmp(commands[0], "LIST")==0)//if the user inputs list
		{
			for(int i=0; i<30; i++)//loops through all of the array 
			{
				if(clientson[i].fd!=0)//if the fd is not empty
				{
					strcpy(termsen, clientson[i].username);
					strcat(termsen, "\n");
					write(cli_sockfd, termsen, strlen(termsen));//print the names to the user
				}
			}

		}
		else if(strcmp(commands[0], "QUIT")==0)//if the user wants to quit
		{
			write(cli_sockfd, commands[0],strlen(commands[0]));//send the word back to be executed in the client
		}
		else if(strcmp(commands[0], "SEND")==0)//if the user chooses send
		{
			strcpy(termsen, "From:");
                        strcat(termsen,clientson[place].username);
                        strcat(termsen,"\n");
                        strcat(termsen,commands[2]);
			strcat(termsen,"\n");//formats the message to be sent to whomever specified

			for(int i=0; i<sizeof(commands[1]); i++)//sets the next portion to lowercase for easier manipulation
        	        {
	                        commands[1][i]=tolower(commands[1][i]);
                	}

			if(strcmp(commands[1],"*")==0)//if it is a * then it will be sent to everyone
			{
				for(int i=0; i<30; i++)//loops through the struct array
				{
					if(clientson[i].fd!=0)//if the spot is not empty
					{
						write(clientson[i].fd, termsen, strlen(termsen));//send the message to them via fd value
					}
				}
			}
			else//else meaning a specific user
			{
				for(int i=0; i<30; i++)//loops to find the username
				{
					if(strcmp(clientson[i].username, commands[1])==0)//if the user is found
					{
						for(int j=0; j<30; j++)//checking to see if the sender is on the recievers blocked list
						{
							if(clientson[i].blacklist[j] ==cli_sockfd)//if yes
							{
								strcpy(termsen, "Sorry. User ");
								strcat(termsen, commands[1]);
								strcat(termsen, " has blocked you.\n");
								write(cli_sockfd, termsen, strlen(termsen));//let the sender know they are blocked
								fflush(stdout);
								goto ends;
							}
							else if(j==30-1)//if the person is not on the list 
							{
								write(clientson[i].fd, termsen,strlen(termsen));//send them the message
							}
						}
					}
					else if((i==30-1)&&(strcmp(commands[1], clientson[i].username)!=0))//else if the persons user does not exist
                                	{
                                        	strcpy(termsen, "User ");
                                        	strcat(termsen, commands[1]);
                                        	strcat(termsen, " does not exist\n");
                                        	write(cli_sockfd, termsen, strlen(termsen));//let the sender know 
                                        	goto ends;
                                	}
				}
			}
		}
		else if(strcmp(commands[0], "UNBLOCK")==0)//to unblock a user
		{
			for(int i=0; i<sizeof(commands[1]); i++)//sets the user name to lowercase
                        {
                                commands[1][i]=tolower(commands[1][i]);
                        }


			for(int i=0; i<30; i++)//loops to find fd
                        {
                                if(strcmp(commands[1], clientson[i].username)==0)//checks to see if there is an actual username by the one specified
                                {
                                        temp = clientson[i].fd;//if so set temp to the users fd
                                }
				else if((i==30-1)&&(strcmp(commands[1], clientson[i].username)!=0))//if the user does not exist
                                {
                                        strcpy(termsen, "User ");
                                        strcat(termsen, commands[1]);
                                        strcat(termsen, " does not exist\n");
                                        write(cli_sockfd, termsen, strlen(termsen));//let the user know
                                        goto ends;
                                }
                        }

			for(int i=0; i<30; i++)//loops though the blocked list
			{
				if(clientson[place].blacklist[i] == temp)//if the value is in the list
				{
					clientson[place].blacklist[i]=0;//switch it back to empty
					break;
				}
			}

			strcpy(termsen, "Done. You will receive messages from ");
			strcat(termsen, commands[1]);
			strcat(termsen, "\n");
			write(cli_sockfd, termsen, strlen(termsen));//lets the user know the user is unblocked

		}
		else//if they input an incorrect command
		{
			strcpy(usermessage,"Command not recognized\n\nValid commands are:\nBLOCK:<euid>\nLIST\nQUIT\nSEND:(<euid>|*):<string>\nUNBLOCK:<euid>\nHELP\n");
                        if(write(cli_sockfd, usermessage, strlen(usermessage))<0)//writes to the user to let them know they did not send a valid command and what the valid ones are
                        {
                                perror("write");
                                break;
                        }
                        fflush(stdout);
		}
		ends:
		// send message back to client
                fflush(stdout);

		bzero(usermessage, 256);	// clear out previous message
        }

	// client connection can be reset by peer, so just disconnect cleanly
        if(nread <= 0)
        {
		for(int i=0; i< 30;i++)//if a client disconnects, find their spot in the struct array
		{
			if(clientson[i].fd == cli_sockfd)//if found
			{
				strcpy(termsen,clientson[i].username);//set to be used to let server know
				clientson[i].fd=0;//set the fd to zero
				strcpy(clientson[i].username,"none");//resets the username
			}
		}
		//printf("Host disconnected, ip %s, port %s\n", inet_ntoa(in6_addr.sin_addr), in6_addr.sin_port);
		printf("User %s has logged out\n", termsen);//lets the server know who logged out.
                fflush(stdout);
        }

        free(sockfd);//free the sockfd

        return 0;
}


