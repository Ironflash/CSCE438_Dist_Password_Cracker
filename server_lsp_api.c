#include <sys/socket.h>      
#include <sys/types.h>        
#include <arpa/inet.h> 
#include <netinet/ip.h> 
#include <netinet/in.h>
#include <unistd.h> 
#include <stdlib.h>
#include <string>
#include "lsp_message.c"
#include "lsp_server.h"
#include "lspMessage.pb.h"
#include <stdio.h>
#include <pthread.h>

using namespace std;


/* Reads a message from network */
void* readMessage(void* arg) //, void* pld, uint32_t* conn_id, struct lsp_message* message)
{
	lsp_server* a_srv = (lsp_server*) arg;
	const int MAX_BUFFER = 1000;
	void* buffer = malloc(MAX_BUFFER);

	printf("Waiting\n");

	
	sockaddr_in tempCli;
	socklen_t sockLen = sizeof(sockaddr_in);
	int num_read;
	//keep reading for messages
	while(true)
	{
		//get packet from socket
		if((num_read = recvfrom(a_srv->getSocket(), buffer , MAX_BUFFER, 0,	
	                 (struct sockaddr *) &tempCli, &sockLen)) < 0)
		{
			perror("Unable to read\n");
			// return NULL;
		}
		else
		{
			printf("Read: %d Bytes\n",num_read);
		}
		// needed to use char* to get message from recv so this converts the char* to a string that protobuf can use
		// Code to unmarshall a lsp_message ... still in progress 
		GOOGLE_PROTOBUF_VERIFY_VERSION;
		lspMessage::LspMessage msg;

		if(!msg.ParseFromArray(buffer,num_read))
	    {
	      fprintf(stderr, "error unpacking incoming message\n");
	      // return NULL;
	    }
	    printf("End of marshalling\n");
	    // End of unmarshalling

	    
	    //get payload from lsp_message
	    string payload =  msg.payload();
	    // *((string*)pld) = payload;		//convert the void* to a string pointer and set data to that of the string
	    printf("payload: %s\n",payload.c_str());
		//get connection id from the packet
		uint32_t connid = msg.connid();
		// conn_id = &connid;
		printf("Conn-id: %d\n",connid);

		//set current sequence number for that client
		uint32_t seqnum = msg.seqnum();
		printf("Seqnum: %d\n",seqnum);
		//consruct message
	    // *message = {connid, seqnum, payload};
		a_srv->addInbox(new lsp_message(connid,seqnum,payload,num_read));
	}
}

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

	/* start a thread that will continue to read for messages */
	pthread_t readThread = newServer->getReadThread();
	if( (pthread_create(&readThread, NULL, readMessage, (void*)(newServer))) < 0)
	{
		perror("Pthread create failed");
		delete newServer;
		return NULL;
	}
	else if((pthread_detach(newServer->getReadThread())) < 0)
	{
		perror("Pthread detach failed");
		delete newServer;
		return NULL;
	}
	// readMessage((void*) newServer);
	return newServer;
}

// Read from connection. Return NULL when connection lost
// Returns number of bytes read. conn_id is an output parameter
int lsp_server_read(lsp_server* a_srv, void* pld, uint32_t* conn_id)
{
	// struct lsp_message* message
	// int num_read = readMessage(a_srv,pld, conn_id, message);
	//  //if message was sucessfully created than add it to the queue of messages 
	// if(message != NULL)
	// {
	// 	a_srv->inbox.push_back(message);
	// }
	// return num_read;
	lsp_message* message = a_srv->fromInbox();
	if(message == NULL)
	{
		return -1;
	}
	*((string*)pld) = message->m_payload;		//convert the void* to a string pointer and set data to that of the string
	uint32_t connid = 
	*conn_id = message->m_connid;
	return message->m_bytesRead;
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