# makefile

hw2_package: server request worker

request: request.c
	g++ -g request.c lspMessage.pb.cc -o request -lprotobuf -lpthread

worker: worker.c
	g++ -g worker.c lspMessage.pb.cc -o worker -lprotobuf -lpthread -lssl

server: server.c
	g++ -g server.c lspMessage.pb.cc -o server -lprotobuf -lpthread

hw2:
	clear
	rm -f *o request request_lsp_api worker worker_lsp_api server server_lsp_api core
	rm -f fifo*
	make hw2_package
	clear

hw2r:
	clear
	rm -f *o request request_lsp_api core
	rm -f fifo*
	make request
	clear

hw2w:
	clear
	rm -f *o worker worker_lsp_api core
	rm -f fifo*
	make worker
	clear

hw2s:
	clear
	rm -f *o server server_lsp_api core
	rm -f fifo*
	make server
	clear

test_sha1:
	clear
	rm -f *o test_sha1 core
	g++ test_sha1.c -o test_sha1 -lssl

clean:
	rm -f *o request request_lsp_api worker worker_lsp_api server server_lsp_api core
	rm -f fifo*
	