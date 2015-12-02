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
#include <sys/wait.h> /* for the waitpid() system call */
#include <signal.h> /* signal name macros, and the kill() prototype */
#include <sys/stat.h>   // stat()
#include <time.h>

#include <errno.h>

#define BUFSIZE 1024

int PORTNO = 5000;
int CWND = 1024;     // given in unit of bytes
double PL = 0.0;
double PC = 0.0;

// For DATA packets:
//  seqNum = sending packet #
//  dataLen = 
struct Packet
{
  int seqNum;
  int dataLen;
  char data[BUFSIZE - 2*sizeof(int)];
};

void error(char *msg)
{
  perror(msg);
  exit(1);
}

double rand_percent()
{
  return (double)rand() / (double)RAND_MAX;
}

void sendFile(int sockfd, struct sockaddr_in *cli_addr, socklen_t cli_len, char *filename)
{
  FILE* fp = fopen(filename, "r");
  struct Packet pkt;
  struct Packet ack_pkt;
  if (fp == NULL)   // cannot open file
  {
    pkt.seqNum = -1;  // signals error in filename
    pkt.dataLen = 0;
    sendto(sockfd, &pkt, BUFSIZE, 0, (struct sockaddr *) cli_addr, cli_len);
    printf("ERROR opening file\n");
  }
  else
  {
    struct stat filestat;
    stat(filename, &filestat);
    int filesize = filestat.st_size;

    char *file_buf = (char*)malloc(filesize);
    fread(file_buf, filesize*sizeof(char), sizeof(char), fp);

    int base = 1;
    int nextSeqNum = 1;
    int maxDataLen = BUFSIZE - 2*sizeof(int);
    pkt.dataLen = maxDataLen;
    int numPackets = filesize / (BUFSIZE - 2*sizeof(int)) + 1;    // at least 1 packet

    time_t timer;
    time(&timer);
    // Send initial packets to fill the window
    while (nextSeqNum*maxDataLen <= CWND)    // TODO(yjchoi): if CWND is larger than file size?
    {
      pkt.seqNum = nextSeqNum;
      bzero(pkt.data, maxDataLen);
      memcpy(pkt.data, file_buf+(nextSeqNum-1)*maxDataLen, pkt.dataLen);
      if (sendto(sockfd, &pkt, BUFSIZE, 0, (struct sockaddr *) cli_addr, cli_len) < 0) {
        printf("ERROR on sending DATA #%d\n", nextSeqNum);
      } else {
        printf("SENDER: Sent %d bytes for DATA #%d\n", pkt.dataLen, nextSeqNum);
        nextSeqNum++;
      }
    }

    while (base <= numPackets)
    {
      if (time(NULL) > timer + 5)
      {
        printf("SENDER: Timeout on #%d!\n", base);
        time(&timer);
        int i = base;
        for ( ; i < nextSeqNum; i++)
        {
          pkt.seqNum = i;
          pkt.dataLen = i == numPackets ? filesize % maxDataLen : maxDataLen;
          bzero(pkt.data, maxDataLen);
          memcpy(pkt.data, file_buf+(i-1)*maxDataLen, pkt.dataLen);
          if (sendto(sockfd, &pkt, BUFSIZE, 0, (struct sockaddr *) cli_addr, cli_len) < 0) {
            printf("ERROR on retransmitting DATA #%d\n", i);
          } else {
            printf("SENDER: Retransmitted DATA #%d\n", i);
          }
        }
      }

      if (recvfrom(sockfd, &ack_pkt, BUFSIZE, 0, (struct sockaddr *) cli_addr, &cli_len) > 0)
      {
        if (rand_percent() < PC)
        {
          printf("SENDER: Received a corrupted ACK\n");
        }
        else if (rand_percent() >= PL)
        {
          printf("SENDER: Received ACK #%d\n", ack_pkt.seqNum);
          if (base == ack_pkt.seqNum)
          {
            time(&timer);
          }
          base = ack_pkt.seqNum + 1;
          while ((nextSeqNum-base+1)*maxDataLen <= CWND && nextSeqNum <= numPackets)
          {
            // if nextSeqNum == numPackets-1 and can fit in window?
            pkt.seqNum = nextSeqNum;
            pkt.dataLen = nextSeqNum == numPackets ? filesize % maxDataLen : maxDataLen;
            bzero(pkt.data, maxDataLen);
            memcpy(pkt.data, file_buf+(nextSeqNum-1)*maxDataLen, pkt.dataLen);
            if (sendto(sockfd, &pkt, BUFSIZE, 0, (struct sockaddr *) cli_addr, cli_len) < 0) {
              printf("ERROR on sending DATA #%d\n", nextSeqNum);
            } else {
              printf("SENDER: Sent %d bytes for DATA #%d\n", pkt.dataLen, nextSeqNum);
              nextSeqNum++;
            }
          }
        }
      }
    }
    close(file_buf);
  }
  fclose(fp);
}

int main(int argc, char *argv[])
{
  int sockfd, recvlen;
  struct sockaddr_in serv_addr, cli_addr;
  socklen_t cli_len = sizeof(cli_addr);
  struct Packet pkt;
  struct timeval timeout;
  timeout.tv_sec = 1;  // 1 second;
  timeout.tv_usec = 0;
  struct timeval notimeout;
  notimeout.tv_sec = 0;
  notimeout.tv_usec = 0;

  // Read arguments
  switch(argc)
  {
    case 5:
      PC = atof(argv[4]);
    case 4:
      PL = atof(argv[3]);
    case 3:
      CWND = atoi(argv[2]);
    case 2:
      PORTNO = atoi(argv[1]);
      break;
  }

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(PORTNO);
     
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    error("ERROR on binding");
  }
  
  while(1) {
    printf("Waiting for request\n");
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&notimeout, sizeof(struct timeval));
    recvlen = recvfrom(sockfd, &pkt, BUFSIZE, 0, (struct sockaddr *) &cli_addr, &cli_len);
    if (recvlen == -1) {
      error("ERROR on receiving request");
    }
    printf("SENDER: Received request. Filename: %s\n", pkt.data);
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&timeout, sizeof(struct timeval));
    sendFile(sockfd, &cli_addr, cli_len, pkt.data);
  }
}
