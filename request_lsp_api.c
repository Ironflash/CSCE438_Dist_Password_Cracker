#include <sys/socket.h>      
#include <sys/types.h>        
#include <arpa/inet.h> 
#include <netinet/ip.h> 
#include <netinet/in.h>
#include <unistd.h> 
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include "lsp_packet.c"

/* Declare what bool is */
typedef enum { false, true } bool;

struct lsp_request
{
	int m_port;		//port that will be used for listening 
	int m_socket;		//socket that will listen for incoming connections from requests and workers
	struct sockaddr_in m_servaddr, m_cliaddr;	//address of the server and client
};
//Create a handle for a request returns false if server unavailable
struct lsp_request* lsp_request_create(const char* dest, int port)
{
	struct lsp_request* newRequest; 
	newRequest = malloc(sizeof(struct lsp_request)); //alocate memory for server
	if(newRequest == NULL)
	{
		printf("space allocation for request failed\n");
		free(newRequest);
		return false;		//return false if memory could not be allocated
	}
	newRequest->m_port = port;
	if((newRequest->m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		printf("socket creation failed\n");
		free(newRequest);
		return false; 		// return false on error
	}
	struct hostent* host;
	if((host = (struct hostent*) gethostbyname(dest))<0)
	{
		return false;		//server name couldn't be resolved
	}

	memset(&newRequest->m_servaddr, 0, sizeof(newRequest->m_servaddr));
	newRequest->m_servaddr.sin_family = AF_INET;
	// newRequest->m_servaddr.sin_addr.s_addr = htonl(host->h_addr[0]);
	char* ip = "127.0.0.1"; //temp
	newRequest->m_servaddr.sin_addr.s_addr = inet_addr(ip);
	newRequest->m_servaddr.sin_port = htons(1234);

	newRequest->m_cliaddr.sin_family = AF_INET;
  	newRequest->m_cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  	newRequest->m_cliaddr.sin_port = htons(0);


	//Bind Socket
	if ( bind(newRequest->m_socket,(struct sockaddr *) &(newRequest->m_cliaddr), sizeof(newRequest->m_cliaddr)) < 0)
	{
		printf("bind failed\n");
		free(newRequest);
		return false;	//return false if socket could not be bound
	}
	return newRequest;
}

// Request Read. Returns NULL when connection lost
// Returns number of bytes read
int lsp_request_read(struct lsp_request* a_request, uint8_t* pld)
{

}

// Request Write. Should not send NULL
bool lsp_request_write(struct lsp_request* a_request, uint8_t* pld, int lth)
{
	printf("Attempting to send %d\n",sizeof(pld));
	if(sendto(a_request->m_socket, pld, sizeof(pld), 0, (struct sockaddr *)&a_request->m_servaddr, sizeof(a_request->m_servaddr)) < 0)
	{
	   printf("sendto failed\n");
	   return false;
	}
	return true;
}

// Close connection. Remember to free memory.
bool lsp_request_close(struct lsp_request* a_request)
{
	free(a_request);
}