#include <sys/socket.h>      
#include <sys/types.h>        
#include <arpa/inet.h> 
#include <netinet/ip.h> 
#include <netinet/in.h>
#include <unistd.h> 
#include <stdlib.h>
#include <string>
#include "lsp_server.h"
#include "lsp_message.c"
#include "lspMessage.pb.h"
#include <stdio.h>
// #include <pthread.h>

#define SOCKET_ERROR -1
using namespace std;

// Sets up server and returns NULL if server could not be started
lsp_server* lsp_server_create(int port)
{
	printf("one");
	lsp_server* newServer = new lsp_server(); 
	if(newServer == NULL)
	{
		printf("space allocation for server failed\n");
		delete newServer;
		return NULL;		//return NULL if memory could not be allocated
	}
	newServer->setPort(port);
	if((newServer->setSocket(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
	{
		printf("socket creation failed\n");
		delete newServer;
		return NULL; // return Null on error
	}
	sockaddr_in tempServ;
	tempServ.sin_family = AF_INET;
	tempServ.sin_addr.s_addr = htonl(INADDR_ANY);
	tempServ.sin_port = htons(1234);
	newServer->setServAddr(tempServ);

	//Bind Socket
	if ( bind(newServer->getSocket(),(struct sockaddr *) &(newServer->getServAddr()), sizeof(newServer->getServAddr())) < 0)
	{
		perror("bind failed");
		delete newServer;
		return NULL;	//return NULL if socket could not be bound
	}
	return newServer;
}

// Read from connection. Return NULL when connection lost
// Returns number of bytes read. conn_id is an output parameter
int lsp_server_read(const lsp_server* a_srv, void* pld, uint32_t* conn_id)
{
	char recvbuffer[1000];
	string buffer;
	printf("Size of buffer at creation: %d\n",sizeof(buffer));
	int error;
	int timeOut = 100;

	printf("Waiting\n");

	
	struct sockaddr_in tempCli;
	socklen_t sockLen = sizeof(sockaddr_in);
	//get packet from socket
	int num_read;
	if((num_read = recvfrom(a_srv->getSocket(), recvbuffer , 10, 0,		//10 is only for testing
                 (struct sockaddr *) &tempCli, &sockLen)) < 0)
	{
		perror("Unable to read\n");
		return NULL;
	}
	else
	{
		printf("Read: %d Bytes\n",num_read);
	}
	// needed to use char* to get message from recv so this converts the char* to a string that protobuf can use
	buffer.assign(recvbuffer,recvbuffer+num_read);
	// Code to unmarshall a lsp_message ... still in progress 
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	lspMessage::LspMessage msg;
	size_t msg_len = num_read;

	printf("Size of buffer: %d\n",sizeof(buffer));
	if(msg.ParseFromString(buffer))
    {
      fprintf(stderr, "error unpacking incoming message\n");
      return NULL;
    }
    printf("End of marshalling\n");
    // End of unmarshalling
    

    //get payload from lsp_message
    string payload =  msg.payload();
    *((string*)pld) = payload;		//convert the void* to a string pointer and set data to that of the string
    printf("payload: %s\n",payload.c_str());
	//get connection id from the packet
	uint32_t connid = msg.connid();
	conn_id = &connid;

	//set current sequence number for that client
	int seqnum = msg.seqnum();
	//TO-DO

	return num_read;
}

// Server Write. Should not send NULL
bool lsp_server_write(const lsp_server* a_srv, void* pld, int lth, uint32_t conn_id)
{
	return true;
}

// Close connection.
bool lsp_server_close(const lsp_server* a_srv, uint32_t conn_id)
{
	close(a_srv->getSocket());
	delete a_srv;
	return true;
}