#define BUFSIZE 1024

#define START_SEQ_NUM 0
#define FILE_REQUEST -1
#define FILE_ERROR -2
#define FIN -3
#define ACK_FIN -4

//  seqNum = packet # OR reserved codes
//  dataLen = bytes left in file if DATA packet. 0 otherwise
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

int min(int a, int b)
{
    if (a < b)
        return a;
    return b;
}
