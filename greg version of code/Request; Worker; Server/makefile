# makefile

# hw2 implementation compiler calls
# hw2_client: server request
hw2_client: request worker

hw2_server: server

semaphore.o: semaphore.h semaphore.c
	g++ -c -g semaphore.c

netreqchannel.o: netreqchannel.h netreqchannel.c
	g++ -c -g netreqchannel.c

trequest: request.c netreqchannel.o
	g++ request.c netreqchannel.o lspMessage.pb.cc -o request -lprotobuf -lpthread

tworker: worker.c netreqchannel.o
	g++ worker.c netreqchannel.o lspMessage.pb.cc -o worker -lprotobuf -lpthread

tserver: server.c netreqchannel.o
	g++ server.c netreqchannel.o lspMessage.pb.cc -o server -lprotobuf -lpthread

#request: request.c semaphore.o
request: request.c
	g++ -g request.c lspMessage.pb.cc -o request -lprotobuf -lpthread
#	g++ -g -o request request.c lspMessage.pb.cc -lprotobuf -lpthread

worker: worker.c
	g++ -g worker.c lspMessage.pb.cc -o worker -lprotobuf -lpthread
# 	g++ -g -o worker worker.c

#server: server.c semaphore.o
#	g++ -g server.c semaphore.o lspMessage.pb.cc -o server -lprotobuf -lpthread
server: server.c
	g++ -g server.c lspMessage.pb.cc -o server -lprotobuf -lpthread

hw2:
	clear
	rm -f *o request request_lsp_api worker worker_lsp_api server server_lsp_api core
	rm -f fifo*
	make hw2_client

hw2rt:
	clear
	rm -f *o request request_lsp_api core
	rm -f fifo*
	make trequest

hw2wt:
	clear
	rm -f *o worker worker_lsp_api core
	rm -f fifo*
	make tworker

hw2st:
	clear
	rm -f *o server server_lsp_api core
	rm -f fifo*
	make tserver

hw2r:
	clear
	rm -f *o request request_lsp_api core
	rm -f fifo*
	make request

hw2w:
	clear
	rm -f *o worker worker_lsp_api core
	rm -f fifo*
	make worker

hw2s:
	clear
	rm -f *o server server_lsp_api core
	rm -f fifo*
	make server

clean:
	rm -f *o request request_lsp_api worker worker_lsp_api core
	rm -f fifo*
	