#include "request_lsp_api.c"

int main()
{
	/* Create request */
	struct lsp_request* request = lsp_request_create("www.example.com",1234);
	string example = "new test";
	int msg_cnt = 0;
	lsp_request_write(request,example,msg_cnt);
	while(true){}
	/* Close the request when done */
	lsp_request_close(request);
}