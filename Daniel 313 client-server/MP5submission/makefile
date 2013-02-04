# makefile

# MP3 implementation compiler calls
mp3_client: reqdataserver reqclient

reqchannel.o: reqchannel.H reqchannel.C
	g++ -c -g reqchannel.C

reqdataserver: reqdataserver.C reqchannel.o
	g++ -g -o reqdataserver reqdataserver.C reqchannel.o
	
semaphore.o: semaphore.H semaphore.C
	g++ -c -g semaphore.C

reqclient: reqclient.C reqchannel.o semaphore.o
	g++ -g -o reqclient reqclient.C reqchannel.o semaphore.o

mp3:
	clear
	rm -f *o reqdataserver reqclient core
	rm -f fifo*
	make mp3_client

# MP5 implementation compiler calls
test_client: dataserver simpleclient

mp5_client: dataserver client

netreqchannel.o: netreqchannel.H netreqchannel.C
	g++ -c -g netreqchannel.C

dataserver: dataserver.C netreqchannel.o
	g++ -g -o dataserver dataserver.C netreqchannel.o

simpleclient: simpleclient.C netreqchannel.o semaphore.o
	g++ -g -o simpleclient simpleclient.C netreqchannel.o semaphore.o
	
client: client.C netreqchannel.o semaphore.o
	g++ -g -o client client.C netreqchannel.o semaphore.o
			
mp5_test:
	clear
	rm -f *o dataserver simpleclient core
	rm -f fifo*
	make test_client

mp5:
	clear
	rm -f *o dataserver client core
	rm -f fifo*
	make mp5_client
