#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent
#include <stdlib.h>
#include <string.h>
#define BUFSIZE 1024

int SEQNUM = 0;
const int DEFAULT_PORTNO = 5000;
const double DEFAULT_PL = 0.10;
const double DEFAULT_PC = 0.10;

void error(char *msg)
{
    perror(msg);
    exit(0);
}

struct Packet
{
  int seqNum;
  int dataLen;
  char data[BUFSIZE - 2*sizeof(int)];
};

// Form file request packet
int sendFileRequest(int sockfd, struct sockaddr_in serveraddr, char *filename, char buf[])
{
    struct Packet request_pkt;
    request_pkt.seqNum = SEQNUM;     // Sequence number for request message starts at 0
    request_pkt.dataLen = strlen(filename) + 1;
    memcpy(request_pkt.data, filename, strlen(filename) + 1);
    memcpy(buf, &request_pkt, BUFSIZE);
    return sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
}

int receiveFile(int sockfd, struct sockaddr_in serveraddr, char* filename)
{
    struct Packet* receive_pck;
    FILE* fp = fopen(filename, "a");

    while(1)
    {
        if (recvfrom(sockfd, (void*) receive_pck, BUFSIZE, 0, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) < 0)
            error("ERROR: Failed to receive the file\n");

        //fprintf(fp, receive_pck->data);
        printf("%s", receive_pck->data);

        if (receive_pck->dataLen < 1016)
            break;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 4)
        error("ERROR: Invalid Arguments. Arguments should be the <sender_hostname> <sender_portnumber> <filename>\n");
    
    int portno = DEFAULT_PORTNO;
    double prob_loss = DEFAULT_PL;
    double prob_corrupt = DEFAULT_PC;
    char* filename;
    char* hostname;

    int sockfd;
    struct sockaddr_in serveraddr;
    struct hostent* server;
    char buf[BUFSIZE];

    switch(argc)
    {
    case 6:
        prob_corrupt = atof(argv[5]);
    case 5:
        prob_loss = atof(argv[4]);
    }
    portno = atoi(argv[2]);
    
    filename = malloc(strlen(argv[3]) + 1);
    strcpy(filename, argv[3]);

    hostname = malloc(strlen(argv[1]) + 1);
    strcpy(hostname, argv[1]);

    printf("argc: %d\n", argc);
    printf("Sender Hostname: %s, Sender Port Number: %d, File name: %s, PL: %f, PC: %f\n", hostname, portno, filename, prob_loss, prob_corrupt);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR: Could not open a socket\n");

    server = gethostbyname(hostname);
    if (server == NULL)
        error("ERROR: No such hostname\n");

    bzero((char*) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char*) server->h_addr, (char*) &serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    if (sendFileRequest(sockfd, serveraddr, filename, buf) < 0)
    {
        error("ERROR: Send request fail\n");
    }

    close(sockfd);
}
