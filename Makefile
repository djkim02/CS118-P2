CC=gcc
CFLAGS=-I.
DEPS = # header file 
SENDEROBJ = sender.o
RECEIVEROBJ = receiver.o

all: sender receiver		

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sender: $(SENDEROBJ)
	$(CC) -o $@ $^ $(CFLAGS)

receiver: $(RECEIVEROBJ)
	$(CC) -o $@ $^ $(CFLAGS)
	
	


