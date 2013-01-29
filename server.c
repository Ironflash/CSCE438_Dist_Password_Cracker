#include "server_lsp_api.c"

int main()
{
	/* Create server */
	struct lsp_server* serv = lsp_server_create(123);

	string input;
	uint32_t fake_id = 0;
	int num_read = lsp_server_read(serv,(void*) &input, &fake_id);
	printf("From Client: %s\n",input.c_str()); 
	/* Close the server when done */
	// lsp_server_close(serv,1); //1 is just for testing needs to change
}