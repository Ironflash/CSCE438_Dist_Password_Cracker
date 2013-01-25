#include <sys/socket.h>      
#include <sys/types.h>        
#include <arpa/inet.h> 
#include <netinet/ip.h> 
#include <netinet/in.h>
#include <unistd.h> 
#include <stdlib.h>
#include <string.h>
#include "lsp_message.c"
#include <stdio.h>
// #include <pthread.h>

/* Declare what bool is */
typedef enum { false, true } bool;

#define SOCKET_ERROR -1


struct lsp_server
{
	int m_port;		//port that will be used for listening 
	int m_socket;		//socket that will listen for incoming connections from clients and workers
	struct sockaddr_in m_servaddr, m_cliaddr;	//address of the server and client
};

// Sets up server and returns NULL if server could not be started
struct lsp_server* lsp_server_create(int port)
{
	printf("one");
	struct lsp_server* newServer; 
	newServer = malloc(sizeof(struct lsp_server)); //alocate memory for server
	if(newServer == NULL)
	{
		printf("space allocation for server failed\n");
		free(newServer);
		return NULL;		//return NULL if memory could not be allocated
	}
	newServer->m_port = port;
	if((newServer->m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		printf("socket creation failed\n");
		free(newServer);
		return NULL; // return Null on error
	}
	memset(&newServer->m_servaddr, 0, sizeof(newServer->m_servaddr));
	newServer->m_servaddr.sin_family = AF_INET;
	newServer->m_servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	newServer->m_servaddr.sin_port = htons(1234);

	//Bind Socket
	if ( bind(newServer->m_socket,(struct sockaddr *) &(newServer->m_servaddr), sizeof(newServer->m_servaddr)) < 0)
	{
		printf("bind failed\n");
		free(newServer);
		return NULL;	//return NULL if socket could not be bound
	}
	return newServer;
}

// Read from connection. Return NULL when connection lost
// Returns number of bytes read. conn_id is an output parameter
int lsp_server_read(const struct lsp_server* a_srv, void* pld, uint32_t* conn_id)
{
	char buffer[1000];
	int error;
	int timeOut = 100;

	printf("Waiting\n");

	int sockLen = sizeof(a_srv->m_cliaddr);

	//get packet from socket
	int num_read;
	if((num_read = recvfrom(a_srv->m_socket, buffer , 1, 0,
                 (struct sockaddr *) &a_srv->m_cliaddr, &sockLen)) < 0)
	{
		printf("Unable to read\n");
		return NULL;
	}
	// strcpy(pld,&msg);		//used for testing
	struct lsp_message msg;
	// Code to unmarshall a lsp_message ... still in progress won't compile until create proto-c .h and .c
	size_t msg_len = num_read;
	msg = lspmessage__unpack(NULL, msg_len, buffer);   
  	if (msg == NULL)
    {
      fprintf(stderr, "error unpacking incoming message\n");
      return NULL;
    }
    // End of unmarshalling
    

    //get payload from lsp_message
    memcpy(pld, &msg->payload,sizeof(msg->payload)); // may need fixed

	//get connection id from the packet
	memcpy(conn_id,&msg->connid,sizeof(uint32_t));

	//set current sequence number for that client
	//TO-DO

	printf("Buffer:  %s\n",buffer); // Testing purposes

	//Free memory that was allocated while marshalling
    lspmessage__free_unpacked(msg, NULL);

	return num_read;
}

// Server Write. Should not send NULL
bool lsp_server_write(const struct lsp_server* a_srv, void* pld, int lth, uint32_t conn_id)
{
	return true;
}

// Close connection.
bool lsp_server_close(const struct lsp_server* a_srv, uint32_t conn_id)
{
	close(a_srv->m_socket);
	free(a_srv); //De-allocate memory
	return true;
}