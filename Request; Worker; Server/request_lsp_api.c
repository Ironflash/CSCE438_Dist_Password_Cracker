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
#include <string>
#include "lsp_request.h"
#include "lsp_message.c"
#include "lspMessage.pb.h" 


using namespace std;



bool sendMessage(lsp_request* a_request, uint32_t connid , uint32_t seqnum, string pld)
{
	// Code to marshall a lsp_message
	printf("Pld: %s\n",pld.c_str());
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	lspMessage::LspMessage* msg = new lspMessage::LspMessage();
	msg->set_connid(connid); // 0 is for initial connection will need to change
	msg->set_seqnum(seqnum); // needs to be 0 for initial connection
	msg->set_payload(pld);

	int size = msg->ByteSize(); 
	printf("Byte Size: %d\n",size);
	void *buffer = malloc(size);
	if(!msg->SerializeToArray(buffer, size))
	{
		printf("serialize failed\n");
		return false;
	}
	printf("Marshalled successfully\n");
	// end of marshalling

	printf("Attempting to send message\n");
	printf("Size of pld: %d\n", sizeof(pld));
	printf("size of msg: %d\n", sizeof(*msg));
	printf("Socket: %d\n",a_request->getSocket());

	int sent;
	//need to convert the string to a char* for sendto
	if((sent = sendto(a_request->getSocket(), buffer, size, 0, (struct sockaddr *)&a_request->getServAddr(), sizeof(a_request->getServAddr()))) < 0)
	{
		perror("Sendto failed");
	   return false;
	}
	else
	{
		printf("Sent: %d bytes\n",sent);
	}
	// Free up memory that was allocated while marshalling
	delete buffer;
	delete msg;
}

lsp_request* lsp_request_create(const char* dest, int port)
{
	lsp_request* newRequest = new lsp_request(); 
	if(newRequest == NULL)
	{
		printf("space allocation for request failed\n");
		delete newRequest;
		return NULL;		//return NULL if memory could not be allocated
	}
	newRequest->setPort(port);
	if((newRequest->setSocket(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
	{
		printf("socket creation failed\n");
		//free(newRequest);
		delete newRequest;
		return NULL; 		// return NULL on error
	}
	struct hostent* host;
	if((host = (struct hostent*) gethostbyname(dest))<0)
	{
		return NULL;		//server name couldn't be resolved
	}
	string ip = "127.0.0.1"; //temp
	sockaddr_in tempServ, tempCli;
	tempServ.sin_addr.s_addr = inet_addr(ip.c_str());
	tempServ.sin_port = htons(1234);

	tempCli.sin_family = AF_INET;
  	tempCli.sin_addr.s_addr = htonl(INADDR_ANY);
  	tempCli.sin_port = htons(0);

  	newRequest->setServAddr(tempServ);
  	newRequest->setCliAddr(tempCli);


	//Bind Socket
	if ( bind(newRequest->getSocket(),(struct sockaddr *) &(newRequest->getCliAddr()), sizeof(newRequest->getCliAddr())) < 0)
	{
		printf("bind failed\n");
		delete newRequest;
		return NULL;	//return false if socket could not be bound
	}

	// send new connection request
	if(!sendMessage(newRequest, 0, 0, "")) 			// Message (0,0,nil) equates to connect request
	{
		printf("Initial Connection failed");
		delete newRequest;
		return NULL;
	}
	return newRequest;
}

// Request Read. Returns NULL when connection lost
// Returns number of bytes read
int lsp_request_read(lsp_request* a_request, uint8_t* pld)
{
}

// Request Write. Should not send NULL
bool lsp_request_write(lsp_request* a_request, string pld, int lth)
{
	
	return sendMessage(a_request,0, lth, pld); // 0 needs to change
}

// Close connection. Remember to free memory.
bool lsp_request_close(lsp_request* a_request)
{
	//go through and free all lsp_message
	delete a_request;
}