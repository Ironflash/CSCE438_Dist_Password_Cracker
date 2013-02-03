# makefile

# hw2 implementation compiler calls
# hw2_client: server request
hw2_client: request worker

req_client: request

hw2_server: server

semaphore.o: semaphore.H semaphore.C
	g++ -c -g semaphore.C

#request: request.c semaphore.o
request: request.c
	g++ request.c lspMessage.pb.cc -o request -lprotobuf -lpthread
#	g++ -g -o request request.c lspMessage.pb.cc -lprotobuf -lpthread

worker: worker.c
	g++ -g -o worker worker.c	

#server: server.c semaphore.o
server: server.c
	g++ server.c lspMessage.pb.cc -o server -lprotobuf -lpthread

hw2:
	clear
	rm -f *o request request_lsp_api worker worker_lsp_api server server_lsp_api core
	rm -f fifo*
	make hw2_client

hw2r:
	clear
	rm -f *o request request_lsp_api core
	rm -f fifo*
	make req_client

hw2s:
	clear
	rm -f *o server server_lsp_api core
	rm -f fifo*
	make server

clean:
	rm -f *o request request_lsp_api worker worker_lsp_api core
	rm -f fifo*
	