#include "request_lsp_api.h"

//Create a handle for a client returns FALSE if server unavailable
lsp_client* lsp_client_create(const char* dest, int port)
{

}

// Client Read. Returns NULL when connection lost
// Returns number of bytes read
int lsp_client_read(lsp_client* a_client, uint8_t* pld)
{

}

// Client Write. Should not send NULL
bool lsp_client_write(lsp_client* a_client, uint8_t* pld, int lth)
{

}

// Close connection. Remember to free memory.
bool lsp_client_close(lsp_client* a_client)
{

}