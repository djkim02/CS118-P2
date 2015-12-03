#define BUFSIZE 1024

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

int min(int a, int b)
{
    if (a < b)
        return a;
    return b;
}
