# makefile

# hw2 implementation compiler calls
# hw2_client: server request
hw2_client: request worker

req_client: request

#request_lsp_api.o: request_lsp_api.c
#	g++ -c -g request_lsp_api.c

#request: request.c request_lsp_api.c
#	g++ -g -o request request.c request_lsp_api.c

semaphore.o: semaphore.H semaphore.C
	g++ -c -g semaphore.C

request: request.c semaphore.o
	g++ request.c lspMessage.pb.cc -o request -lprotobuf -lpthread
#	g++ -g -o request request.c lspMessage.pb.cc -lprotobuf -lpthread

worker: worker.c
	g++ -g -o worker worker.c	

hw2:
	clear
	rm -f *o request request_lsp_api worker worker_lsp_api core
	rm -f fifo*
	make hw2_client

hw2r:
	clear
	rm -f *o request request_lsp_api worker worker_lsp_api core
	rm -f fifo*
	make req_client

clean:
	rm -f *o request request_lsp_api worker worker_lsp_api core
	rm -f fifo*
	