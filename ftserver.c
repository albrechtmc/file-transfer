/*
CS372 Introduction to Networking
File Transfer Client Created by Matthew Albrecht
June 2, 2019
This program is a file transfer program.  It reads the command line
and determines if the user would like to request the directory or
would like to transfer a file.  The program then creates a second
connection on a different port and sends the requested information.
This is the server part of the program.
This program was made with assistance from Beej and his guide to
networking.  Additional functions were borrowed from my first CS372
program.  Additionally sources are cited as appropriate.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#define BACKLOG 10 // how many pending connections queue will hold
#define MAXDATASIZE 500 // max number of bytes we can get at once


//taken from Beej
void sigchld_handler(int s)
{
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;
	while (waitpid(-1, NULL, WNOHANG) > 0);
	errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6: (by Beej)
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
/*****************************************************************************
This function receives messages from the client.  The only message the
program is designed to receive is the first message sent.  It passes the
type of request, port number for the data port, and file name, if applicable.
Each of these three pieces of information are modefied via their pointers, 
thus they are passed to the initial function.  If an invalid command is passed
the program will displan an error message and exit.
This function was taken from my CS372 Project 1 and slightly modefied.  That
function in turn was influenced by Beej in the Guide to Network Programming.
*****************************************************************************/
int receiveMessage(int sockfd, char buf[MAXDATASIZE], char* dataPtr, char* dataTypePtr, char* dataNamePtr)
{
	int numbytes;
	//receive the message and check for errors.  Error checking courtesy of Beej.
	if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1)
	{
		perror("recv");
		exit(1);
	}
	//add a null terminator
	buf[numbytes] = '\0';

	//printf("Client Says:> %s\n", buf);

	char *dataPtr2, *fileLocation;//data port, file name
	//split up the message into its respective pieces
	char delim[] = " ";
	dataPtr2 = strtok(buf, delim);
	char *ptr2 = strtok(NULL, delim);//-l or -g
	strcpy(dataPtr, dataPtr2);
	//printf("Port:> %s\n", dataPtr);

	//determine the type of request
	if (strcmp(ptr2, "-l") == 0)
	{
		//printf("This is -l\n");
		return 1;
	}
	else if (strcmp(ptr2, "-g") == 0)
	{
		//printf("This is -g\n");
		fileLocation = strtok(NULL, delim);
		strcpy(dataNamePtr, fileLocation);
		printf("File requested to Transfer:> %s\n", dataNamePtr);
		return 2;
	}
	else//if the request was invalid, exit
	{
		printf("Please enter a valid command\n");
		return 0;
	}	
	return 1;
}

/************************************************
initiateContact function: this function determines and creates
the socket.  The functions and design are from Beej's Guide to
Network Programming Stream Server Example.  The function takes
the pointer to the addrinfo struct and the int sockfd.  It returns
sockfd for use in the main function.  I took this function from
my first project.
************************************************/
int initiateContact(struct addrinfo *servinfo, struct addrinfo *p, int sockfd)
{
	//connect to the first socket available
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		//set sockfd and check for errors
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("client: socket");
			continue;
		}
		//initiate the connection and check for errors
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			continue;
		}
		break;
	}
	//if the client fails to connect exit the program
	if (p == NULL)
	{
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}
	return sockfd;
}
/************************************************
sendMessage function: this function sends messages to
the client.  it takes the user's handle, and the quit string, and 
the int sockfd to send messages.  If the user transmits \quit, the
message will not be sent and the program will close. The function 
will return 0.  If a message is sent the function will return 1, 
and continue in the while loop in the main function.  Message sending 
function was influenced by Beej and his Guide to Network Programming
************************************************/
int sendMessage(int sockfd, char message[500])
{
	char *ptr = message;//pointer to the input string
	int len = strlen(ptr);

	if (send(sockfd, ptr, len, 0) == -1)
		perror("send");
	return 1;
}
/************************************************
sendMessage function: This function retrieves the contents
of the directory.  It takes the socket that the information
will be sent over.  Each time a name is pulled it is sent to
the client.  After all of the directory names have been sent, 
the End of directory phrase is sent.  This signals to the client
that all items have been sent.  I took this design for this 
function from the following website: 
https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
************************************************/
int retrieveDirectory(int secondSocket)
{
	DIR *d;
	struct dirent *dir;
	d = opendir(".");
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if (dir->d_type == DT_REG)
			{
				printf("%s\n", dir->d_name);
				sendMessage(secondSocket, dir->d_name);
				int n, j = 0;
				for (n = 0; n < 100000; n++)
					j++;
			}
		}
		closedir(d);
	}
	sendMessage(secondSocket, "*End of directory*");
	return(0);
}

/************************************************
sendMessage function: This function opens the file to be 
sent and reads from it.  It takes a pointer to the filename, a
pointer to the contents, and the socket the information will
be sent over.  It continuously reads and sends information 
until all of the information has been sent.  Once the fgets 
fucnction does not pull any more information, the loop ends 
and the file is closed.  If there is no file by the given name, 
0 is returned.  If there is a file and it is read, 1 is returned.
************************************************/
int openFile(char* fileName, char *fileContents, int secondSocket)
{
	FILE *filePointer;
	char fileLine[10000];
	//char fileContents[10000];
	if ((filePointer = fopen(fileName, "r")) == NULL) {
		printf("Error! opening file\n");
		sendMessage(secondSocket, "noFILE");
		return 0;
	}
	sendMessage(secondSocket, fileName);
	printf("File %s is open.\n", fileName);
	while (fgets(fileLine, sizeof(fileLine), filePointer) != NULL)
	{
		//printf("Read: %s\n", fileLine);
		//printf("Read: %s\n", fileContents);
		sendMessage(secondSocket, fileLine);
		//strcat(fileContents, fileLine);
		//printf("Whole string: %s\n", fileContents);
	}

	//printf("%s\n", fileContents);
	fclose(filePointer);
	return 1;

}

int main(int argc, char *argv[])
{
	int firstRun = 1;
	char ipAddress[100];
	strcat(ipAddress, argv[1]);
	strcat(ipAddress, ".engr.oregonstate.edu");
	char userPort[10];
	strcat(userPort, argv[2]);
	int running = 0;
	while (running == 0)
	{
		
		//the following 52 lines are taken from Beej's Guide to Socket Programming
		//argv[1] and argv[2] were substituted in the getaddrinfo function
		int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
		struct addrinfo hints, *servinfo, *p;
		struct sockaddr_storage their_addr; // connector's address information
		socklen_t sin_size;
		struct sigaction sa;
		int yes = 1;
		char s[INET6_ADDRSTRLEN];
		char buf[MAXDATASIZE];
		int rv;
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE; // use my IP
		//char ipAddress[100];
		//strcat(ipAddress, argv[1]);
		//strcat(ipAddress, ".engr.oregonstate.edu");
		if (firstRun == 1)
		{
		if ((rv = getaddrinfo(ipAddress, userPort/*argv[2]*/, &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return 1;
		}
		// loop through all the results and bind to the first we can
		for (p = servinfo; p != NULL; p = p->ai_next) {
			if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
				perror("server: socket");
				continue;
			}
			if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
				perror("setsockopt");
				exit(1);
			}
			if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
				close(sockfd);
				perror("server: bind");
				continue;
			}
			break;
		}

			freeaddrinfo(servinfo); // all done with this structure
			if (p == NULL) {
				fprintf(stderr, "server: failed to bind\n");
				exit(1);
			}
			if (listen(sockfd, BACKLOG) == -1) {
				perror("listen");
				exit(1);
			}
			sa.sa_handler = sigchld_handler; // reap all dead processes
			sigemptyset(&sa.sa_mask);
			sa.sa_flags = SA_RESTART;
			if (sigaction(SIGCHLD, &sa, NULL) == -1) {
				perror("sigaction");
				exit(1);
			}
		}
		printf("server: waiting on %s\n", argv[1]);


		//port to send the data
		char dataPort[5];
		char *dataPtr = dataPort;
		//type of data (-l or -g)
		char dataType[3];
		char *dataTypePtr = dataType;
		//file name to send
		char dataName[100];
		char* dataNamePtr = dataName;

		int secondSocket;

		while (1) {
			//the following 35 lines were taken from my first program as the c client
			//set up the network connection.  The function designs were in turn taken
			//from Beej's Guide to Network Programming
			sin_size = sizeof their_addr;
			new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
			//printf("new_fd is: %d\n", new_fd);
			if (new_fd == -1) {
				perror("accept");
				exit(0);
			}
			inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);

			printf("server: got connection from %s\n", s);

			//receive the initial message from the client
			int dataNum = receiveMessage(new_fd, buf, dataPtr, dataTypePtr, dataNamePtr);
			if (dataNum == 1)
				strcpy(dataType, "-l");
			else if (dataNum == 2)
				strcpy(dataType, "-g");
			else if (dataNum == 0)
			{
				close(new_fd);
				break;
			}
			printf("Data Port to use is: %s\n", dataPtr);
			//printf("Data Type to send is: %s\n", dataTypePtr);
			//if (dataNum != 1)
				//printf("File name is: %s\n", dataNamePtr);
			int j = 0;
			int m;
			for (m = 1; m < 200000000; m++)
				j++;

			//initiate connection on second port
			if ((rv = getaddrinfo(argv[1], dataPtr, &hints, &servinfo)) != 0)
			{
				fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
				return 1;
			}
			secondSocket = initiateContact(servinfo, p, secondSocket);

			//this determines if the directory will be sent or a file will be sent
			if (strcmp(dataType, "-l") == 0)
			{
				printf("List directory requested on port %s.\n", dataPort);
				retrieveDirectory(secondSocket);
			}
			else//if the file will be sent
			{
				char fileName[100];
				strcpy(fileName, dataNamePtr);
				char* filePtr = fileName;
				char fileContents[10000];
				char *fileContentsPtr = fileContents;
				for (m = 1; m < 200000000; m++)
					j++;
				//send "Filename to the client, which signals that
				//the filename will be coming next
				sendMessage(secondSocket, "Filename");
				for (m = 1; m < 200000000; m++)
					j++;
				//open the file and return a 0 or 1 if it exists
				int fileCheck = openFile(filePtr, fileContentsPtr, secondSocket);
				//delay loop
				for (m = 1; m < 1000000000; m++)
					j++;


				if (fileCheck == 1)
				{
					sendMessage(secondSocket, "End of the file");
					printf("File transfer complete.\n");
				}
			}
			//close both sockets
			close(new_fd);
			close(secondSocket);
			break;
		}
		firstRun = 0;
		printf("Type Y to continue: ");
		char userInput;
		fflush(stdin);
		userInput = getchar();
		if (userInput == 89 || userInput == 121)
			running = 0;
		else
			running = 1;
		userInput = 'y';
	}
	return 0;
}