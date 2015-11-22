/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */
#include <sys/stat.h>		// stat()
#include <time.h>

#define BUFSIZE 1024

const int DEFAULT_PORTNO = 5000;
const int DEFAULT_CWND = 4;
const double DEFAULT_PL = 0.10;
const double DEFAULT_PC = 0.10;

// Function prototypes
char* processRequestMessage(int);
void sendHttpResponse(int, char*);
void writeErrorMessage(int);
const char* getContentType (char*);

void sigchld_handler(int s)
{
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

void error(char *msg)
{
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[])
{
  int sockfd, recvlen;
  struct sockaddr_in serv_addr, cli_addr;
  socklen_t cli_len = sizeof(cli_addr);
  struct sigaction sa;          // for signal SIGCHLD
  char buf[BUFSIZE];

	// Read arguments
	int portno = DEFAULT_PORTNO;
  int cwnd = DEFAULT_CWND;
  double prob_loss = DEFAULT_PL;
  double prob_corrupt = DEFAULT_PC;
  switch(argc)
  {
  	case 5:
  		prob_corrupt = atof(argv[4]);
  	case 4:
  		prob_loss = atof(argv[3]);
  	case 3:
  		cwnd = atoi(argv[2]);
  	case 2:
  		portno = atoi(argv[1]);
  		break;
  }

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
     
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		error("ERROR on binding");
	}
  
	while(1) {
		printf("Waiting for data\n");
		recvlen = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &cli_addr, &cli_len);
		if (recvlen == -1) {
			error("ERROR on receiving request");
		}
		sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *) &cli_addr, cli_len);
	}

 //  if (listen(sockfd,5) == -1) {
 //    error("listen");
 //  }
  
 //  clilen = sizeof(cli_addr);

	// /****** Kill Zombie Processes ******/
	// sa.sa_handler = sigchld_handler; // reap all dead processes
	// sigemptyset(&sa.sa_mask);
	// sa.sa_flags = SA_RESTART;
	// if (sigaction(SIGCHLD, &sa, NULL) == -1) {
	// 	error("sigaction");
	// }
	// /*********************************/
     
	// while (1) {
	// 	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

	// 	if (newsockfd < 0) 
	//   	error("ERROR on accept");

	// 	pid = fork(); //create a new process
	// 	if (pid < 0)
	// 		error("ERROR on fork");

	// 	if (pid == 0)  { // fork() returns a value of 0 to the child process
	// 		close(sockfd);
	// 		char *filename = processRequestMessage(newsockfd);
	// 		sendHttpResponse(newsockfd, filename);
	// 		exit(0);
	// 	}
	// 	else //returns the process ID of the child process to the parent
	// 		close(newsockfd); // parent doesn't need this 
	// } /* end of while */
	// return 0; /* we never get here */
}

/******** processRequestMessage() ********
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
// char* processRequestMessage(int sock)
// {
// 	// TODO: check if request message size > buffer_size and dynamically resize buffer
// 	int buffer_size = 512;
// 	char buffer[buffer_size];
// 	bzero(buffer,buffer_size);
// 	if (read(sock, buffer, buffer_size-1) < 0) {
// 		error("ERROR reading from socket");
// 	}
// 	printf("%s",buffer);

// 	if (strstr(buffer, "GET ")) {	// if GET request
// 		int i = 4;
// 		while (buffer[i] != ' ' && i++);
// 		char* filename = (char*)malloc(i-4);
// 		strncpy(filename, buffer+5, i-5);	// get filename, excluding the first '/'
// 		filename[i-3] = '\0';
// 		return filename;
// 	}
// }

// void writeErrorMessage(int sock) {
// 	int buffer_size = 50;
// 	char msg[buffer_size];
// 	strcat(msg, STATUS_NOT_FOUND);
// 	write(sock, msg, strlen(msg));
// 	// TODO: 404 Error HTML
// }

// void sendHttpResponse(int sock, char* filename) {
// 	FILE* fp = fopen(filename, "r");
// 	struct stat filestat;
// 	int buffer_size = 1024;
// 	char msg[buffer_size];
// 	bzero(msg, buffer_size);

// 	if (fp == NULL) {
// 		writeErrorMessage(sock);
// 		return;
// 	}

// 	// Getting current date
// 	char date[128] = "Date: ";
// 	char date_time[40];
// 	time_t current_time;
// 	time(&current_time);
// 	struct tm* gmt_time = (struct tm*)gmtime(&current_time);
// 	strftime(date_time, 40, "%a, %d %b %Y %T %Z", gmt_time);	// null-terminated string of GMT time
// 	strcat(date, date_time);
// 	strcat(date, "\r\n");

// 	// Getting the last modified date
// 	stat(filename, &filestat);
// 	char modified[128] = "Last-Modified: ";
// 	char modified_time[40];
// 	time_t last_modified_time = filestat.st_mtime;
// 	time(&last_modified_time);
// 	struct tm* gmt_last_modified_time = (struct tm*)gmtime(&last_modified_time);
// 	strftime(modified_time, 40, "%a, %d %b %Y %T %Z", gmt_last_modified_time);
// 	strcat(modified, modified_time);
// 	strcat(modified, "\r\n");

// 	// Getting the size of the file
// 	char file_size[128] = "Content-Length: ";
// 	char len[40];
// 	int size = filestat.st_size;
// 	sprintf(len, "%d", (unsigned int)size);
// 	strcat(file_size, len);
// 	strcat(file_size, "\r\n");

// 	// Getting content type
// 	const char* content_type = getContentType(filename);
// 	if (content_type == NULL) {
// 		writeErrorMessage(sock);
// 		printf("Error: Content-Type not found!");
// 		return;
// 	}

// 	// Constructing the message
// 	strcat(msg, STATUS_OK);
// 	strcat(msg, CONNECTION_CLOSE);
// 	strcat(msg, date);
// 	strcat(msg, SERVER_NAME);
// 	strcat(msg, modified);
// 	strcat(msg, file_size);
// 	strcat(msg, content_type);
// 	strcat(msg, "\r\n");
	
// 	write(sock, msg, strlen(msg));	// write HTTP response header
// 	printf("%s", msg);
		
// 	char *file_buf = (char*)malloc(size);
// 	fread(file_buf, size*sizeof(char), sizeof(char), fp);
// 	write(sock, file_buf, size);
	
// 	fclose(fp);
// 	free(file_buf);
// 	free(filename);
// }

// const char* getContentType (char* filename) {
// 	if (strstr(filename, ".html")) {
// 		return HTML;
// 	} else if (strstr(filename, ".jpeg")) {
// 		return JPEG;
// 	} else if (strstr(filename, ".jpg")) {
// 		return JPG;
// 	} else if (strstr(filename, ".gif")) {
// 		return GIF;
// 	} else {
// 		return TXT;
// 	}
// }
