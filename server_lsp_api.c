#include "server_lsp_api.h"

// Sets up server and returns NULL if server could not be started
lsp_server* lsp_server_create(int port)
{

}

// Read from connection. Return NULL when connection lost
// Returns number of bytes read. conn_id is an output parameter
int lsp_server_read(lsp_server* a_srv, void* pld, uint32_t* conn_id)
{

}

// Server Write. Should not send NULL
bool lsp_server_write(lsp_server* a_srv, void* pld, int lth, uint32_t conn_id)
{

}

// Close connection.
bool lsp_server_close(lsp_server* a_srv, uint32_t conn_id);
{
	
}