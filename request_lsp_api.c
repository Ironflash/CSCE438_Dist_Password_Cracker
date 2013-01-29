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
	// Code to marshall a lsp_message

	GOOGLE_PROTOBUF_VERIFY_VERSION;
	lspMessage::LspMessage* msg = new lspMessage::LspMessage();
	string* msg_string = new string();
	msg->set_connid(0); // 0 is for initial connection will need to change
	msg->set_seqnum(lth); // needs to be 0 for initial connection
	msg->set_payload(pld);
	if(!msg->SerializeToString(msg_string))
	{
		printf("serialize failed\n");
		return false;
	}
	printf("Marshalled successfully\n");
	// end of marshalling

	printf("Attempting to send message\n");
	printf("size of msg string: %d\n", sizeof(msg_string));
	printf("Socket: %d\n",a_request->getSocket());

	int sent;
	//need to convert the string to a char* for sendto
	const char* string_conversion = msg_string->c_str();
	if((sent = sendto(a_request->getSocket(), string_conversion, sizeof(string_conversion), 0, (struct sockaddr *)&a_request->getServAddr(), sizeof(a_request->getServAddr()))) < 0)
	{
		perror("Sendto failed");
	   return false;
	}
	else
	{
		printf("Sent: %d bytes\n",sent);
	}
	// Free up memory that was allocated while marshalling
	delete msg_string;

	return true;
}

// Close connection. Remember to free memory.
bool lsp_request_close(lsp_request* a_request)
{
	delete a_request;
}