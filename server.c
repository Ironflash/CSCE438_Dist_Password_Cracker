#include "server_lsp_api.c"

int main()
{
	/* Create server */
	struct lsp_server* serv = lsp_server_create(123);

	string input;
	uint32_t fake_id = 0;
	// int num_read = lsp_server_read(serv,(void*) &input, &fake_id);
	int num_read;
	while(true)
	{
		num_read = lsp_server_read(serv,(void*) &input, &fake_id);
		if(num_read > 0)
		{
			printf("From Client: %s\n",input.c_str());
			string returnString = input + " back at you!";
			lsp_server_write(serv,returnString,0,fake_id);
		}
	} 
	/* Close the server when done */
	lsp_server_close(serv,1); //1 is just for testing needs to change
}