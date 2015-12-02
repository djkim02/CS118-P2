#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent
#include <stdlib.h>
#include <string.h>
#define BUFSIZE 1024

int SEQNUM = 1;
int PORTNO = 5000;
double PL = 0.0;
double PC = 0.0;

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

double rand_percent()
{
  return (double)rand() / (double)RAND_MAX;
}

int min(int a, int b)
{
    if (a < b)
        return a;
    return b;
}

// Form file request packet
int sendFileRequest(int sockfd, struct sockaddr_in serveraddr, char *filename) {
    struct Packet request_pkt;
    request_pkt.seqNum = 0;     // Sequence number for request message starts at 0
    request_pkt.dataLen = strlen(filename) + 1;
    memcpy(request_pkt.data, filename, strlen(filename) + 1);
    return sendto(sockfd, &request_pkt, BUFSIZE, 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
}

int receiveFile(int sockfd, struct sockaddr_in serveraddr)
{
    struct Packet receive_pck;
    struct Packet ack_pck;
    FILE* fp = fopen("filerecv", "a");
    struct sockaddr_in recv_addr;
    socklen_t recv_addr_len = sizeof(recv_addr);

    while(1)
    {
        if (recvfrom(sockfd, (void*) &receive_pck, BUFSIZE, 0, (struct sockaddr *) &recv_addr, &recv_addr_len) < 0)
            error("ERROR: Failed to receive the file\n");

        if (receive_pck.seqNum < 0)
            return -1;

        if (rand_percent() < PC)
        {
          printf("RECEIVER: Received corrupted DATA\n");
        }
        else if (rand_percent() >= PL)
        {
            if (receive_pck.seqNum == SEQNUM)
            {
                printf("RECEIVER: Received %d bytes for DATA #%d.\n", min(sizeof(receive_pck.data), receive_pck.dataLen), receive_pck.seqNum);
                fwrite(receive_pck.data, 1, min(sizeof(receive_pck.data), receive_pck.dataLen), fp);
                ack_pck.seqNum = SEQNUM;
                if (sendto(sockfd, (void*) &ack_pck, BUFSIZE, 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) >= 0)
                {
                    printf("RECEIVER: Sent ACK #%d to the sender.\n", ack_pck.seqNum);
                }
                SEQNUM++;
            }
            else
            {
                if (receive_pck.seqNum < SEQNUM)
                    printf("RECEIVER: Received duplicate DATA #%d.\n", receive_pck.seqNum);
                else
                    printf("RECEIVER: Received out-of-order DATA #%d.\n", receive_pck.seqNum);

                ack_pck.seqNum = SEQNUM - 1;
                if (sendto(sockfd, (void*) &ack_pck, BUFSIZE, 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) >= 0)
                {
                    printf("RECEIVER: Sent ACK #%d to the sender.\n", ack_pck.seqNum);
                }
            }

            if (receive_pck.dataLen < BUFSIZE - 2*sizeof(int))
                break;
        }
    }

    // When the last ACK is lost...

    printf("Successfully received the file!\n");
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 4)
        error("ERROR: Invalid Arguments. Arguments should be the <sender_hostname> <sender_portnumber> <filename>\n");
    
    char* filename;
    char* hostname;

    int sockfd;
    struct sockaddr_in serveraddr;
    struct hostent* server;

    switch(argc)
    {
    case 6:
        PC = atof(argv[5]);
    case 5:
        PL = atof(argv[4]);
    }
    PORTNO = atoi(argv[2]);
    
    filename = malloc(strlen(argv[3]) + 1);
    strcpy(filename, argv[3]);

    hostname = malloc(strlen(argv[1]) + 1);
    strcpy(hostname, argv[1]);

    printf("argc: %d\n", argc);
    printf("Sender Hostname: %s, Sender Port Number: %d, File name: %s, PL: %f, PC: %f\n", hostname, PORTNO, filename, PL, PC);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR: Could not open a socket\n");

    server = gethostbyname(hostname);
    if (server == NULL)
        error("ERROR: No such hostname\n");

    bzero((char*) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char*) server->h_addr, (char*) &serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(PORTNO);

    if (sendFileRequest(sockfd, serveraddr, filename) < 0) {
        error("ERROR: Send request failed\n");
    }
    if (receiveFile(sockfd, serveraddr) < 0) {
        error("ERROR: File does not exist\n");
    }

    close(sockfd);
}
