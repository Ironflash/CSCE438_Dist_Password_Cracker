# makefile

# hw2 implementation compiler calls
# hw2_client: server request
hw2_client: request

request_lsp_api.o: request_lsp_api.C
	g++ -c -g request_lsp_api.C

request: request.C request_lsp_api.o
	g++ -g -o request request.C request_lsp_api.o
				
hw2:
	clear
	rm -f *o request request_lsp_api core
	rm -f fifo*
	make hw2_client
