# makefile

all: dataserver client

reqchannel.o: reqchannel.H reqchannel.C
	g++ -c -g reqchannel.C

dataserver: dataserver.C reqchannel.o 
	g++ -g -o dataserver dataserver.C reqchannel.o 

semaphore.o: semaphore.H semaphore.C
	g++ -c -g semaphore.C
	
client: client.C reqchannel.o semaphore.o
	g++ -g -o client client.C reqchannel.o semaphore.o

clean:
	clear
	rm -f *o dataserver client core
	rm -f fifo*
	make