#include "request_lsp_api.c"

int main()
{
	/* Create request */
	struct lsp_request* request = lsp_request_create("localhost",1234);
	string example = "new test";
	int msg_cnt = 0;
	string input;
	lsp_request_write(request,example,msg_cnt);
	while(true)
	{
		int numRead = lsp_request_read(request,(void*) &input);
		if(numRead > 0)
		{
			printf("From Server: %s\n",input.c_str());
		}

	}
	/* Close the request when done */
	lsp_request_close(request);
}