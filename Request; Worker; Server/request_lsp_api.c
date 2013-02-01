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
#include "lsp_message.c"
#include "lsp_request.h"
#include "lspMessage.pb.h" 


using namespace std;

/* Reads a message from network */
void* readMessage(void* arg) 
{
	lsp_request* a_request = (lsp_request*) arg;
	const int MAX_BUFFER = 1000;
	void* buffer = malloc(MAX_BUFFER);

	printf("Waiting\n");

	sockaddr_in tempServ;
	socklen_t sockLen = sizeof(sockaddr_in);
	int num_read;
	//keep reading for messages
	while(true)
	{
		// sockaddr_in* tempCli = (sockaddr_in*)malloc(sockLen);
		//get packet from socket
		// if((num_read = recvfrom(a_request->getReadSocket(), buffer , MAX_BUFFER, 0,	
	 //                 (struct sockaddr *) &tempServ, &sockLen)) < 0)
		// {
		if((num_read = recvfrom(a_request->getSocket(), buffer , MAX_BUFFER, 0,	
	                 (struct sockaddr *) &tempServ, &sockLen)) < 0)
		{
			perror("Unable to read");
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

	    //consruct message
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
		/* check if message is an ACK */
		if(connid != 0 && seqnum != 0 && payload == "")
		{
			printf("ACK received for message: %d",seqnum);
			a_request->checkMessageAck(connid,seqnum);
		}
		/* check if this is a response to a connection request */
		// printf("Request info %d, %d, payload: %s\n",connid,seqnum, payload.c_str());
		else if(connid != 0 && seqnum == 0 && payload == "" && a_request->getConnid() == 0)
		{
			printf("Connection request response detected\n");
			a_request->setConnid(connid);
			a_request->waitingToOutbox();
		}
		else if(connid == 0 && seqnum == 0)	// somereason a server sent a message using connid 0
		{
			//ignore message ... should probably send back an error
		}
		else // is a normal message
		{
			/* Add message to inbox */
			a_request->toInbox(new lsp_message(connid,seqnum,payload,num_read));
		
			printf("sending to outbox\n");
			/* add ACK to outbox */
			printf("Sending request message to outbox with id: %d\n",connid);
			a_request->toOutbox(new lsp_message(connid,seqnum,""));	
		}	
		printf("END OF READ\n");
	}
}

// void* writeMessageold(void* arg)
// {
// 	lsp_request* a_request = (lsp_request*) arg;
// 	// Code to marshall a lsp_message
// 	printf("Pld: %s\n",pld.c_str());
// 	GOOGLE_PROTOBUF_VERIFY_VERSION;
// 	lspMessage::LspMessage* msg = new lspMessage::LspMessage();
// 	msg->set_connid(connid); 
// 	msg->set_seqnum(seqnum); 
// 	msg->set_payload(pld);

// 	int size = msg->ByteSize(); 
// 	printf("Byte Size: %d\n",size);
// 	void *buffer = malloc(size);
// 	if(!msg->SerializeToArray(buffer, size))
// 	{
// 		printf("serialize failed\n");
// 		return false;
// 	}
// 	printf("Marshalled successfully\n");
// 	// end of marshalling

// 	printf("Attempting to send message\n");
// 	printf("Size of pld: %d\n", sizeof(pld));
// 	printf("size of msg: %d\n", sizeof(*msg));
// 	printf("Socket: %d\n",a_request->getWriteSocket());

// 	int sent;
// 	//need to convert the string to a char* for sendto
// 	if((sent = sendto(a_request->getWriteSocket(), buffer, size, 0, (struct sockaddr *)&a_request->getWriteAddr(), sizeof(a_request->getWriteAddr()))) < 0)
// 	{
// 		perror("Sendto failed");
// 	   return false;
// 	}
// 	else
// 	{
// 		printf("Sent: %d bytes\n",sent);
// 	}
// 	// Free up memory that was allocated while marshalling
// 	delete buffer;
// 	delete msg;
// }
/* sends a message over the network*/
void* writeMessage(void* arg)
{
	lsp_request* a_request = (lsp_request*) arg;

	/* continually try to send messages */
	while(true)
	{
		/* Check for if last message has reveived ACK if not DO NOT get another message */
		if(!a_request->messageAcknowledged())
		{
			continue;
		}
		/* get values from the message */
		lsp_message* message = a_request->fromOutbox();
		if(message == NULL)
		{
			continue;	// TO-DO this needs to probably be adjusted
		}
		string pld = message->m_payload;		//convert the void* to a string pointer and set data to that of the string
		uint32_t connid = message->m_connid;
		uint32_t seqnum = message->m_seqnum;

		// Code to marshall a lsp_message
		GOOGLE_PROTOBUF_VERIFY_VERSION;
		lspMessage::LspMessage* msg = new lspMessage::LspMessage();
		msg->set_connid(connid);
		msg->set_seqnum(seqnum); 
		msg->set_payload(pld);

		int size = msg->ByteSize(); 
		printf("Byte Size: %d\n",size);
		void* buffer = malloc(size);
		if(!msg->SerializeToArray(buffer, size))
		{
			printf("serialize failed\n");
			// return false;
		}
		printf("Marshalled successfully\n");
		// end of marshalling

		printf("Attempting to send message\n");
		printf("Size of pld: %d\n", (int)sizeof(pld)); // (int) added cast 
		printf("size of msg: %d\n", (int)sizeof(*msg)); // (int) added cast

		// printf("Socket: %d\n",a_request->getSocket());

		int sent;
		printf("Client Id: %d\n",connid);
		sockaddr_in servAddr = a_request->getServAddr();

		// string ip = "127.0.0.1"; //temp
		// sockaddr_in tempServ;
		// tempServ.sin_addr.s_addr = inet_addr(ip.c_str());
		// tempServ.sin_port = htons(1234);

		// if((sent = sendto(a_request->getWriteSocket(), buffer, size, 0, (struct sockaddr *) &servAddr, sizeof(servAddr))) < 0)
		if((sent = sendto(a_request->getSocket(), buffer, size, 0, (struct sockaddr *) &servAddr, sizeof(servAddr))) < 0) 
		{
			perror("Sendto failed");
		   // return false;
		}
		else
		{
			printf("Sent: %d bytes\n",sent);
		}
		/* set message waiting */
		a_request->setMessageWaiting(message);

		// Free up memory that was allocated while marshalling
		free (buffer);
		delete msg;
		printf("END OF WRITE\n");
	}
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
	struct hostent* host;
	if((host = (struct hostent*) gethostbyname(dest))<0)
	{
		return NULL;		//server name couldn't be resolved
	}
	string ip = "127.0.0.1"; //temp

	// /* create socket for reading */
	// newRequest->setReadPort(1333); // this needs to be dynamic
	// if((newRequest->setReadSocket(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
	// {
	// 	printf("read socket creation failed\n");
	// 	delete newRequest;
	// 	return NULL; 		// return NULL on error
	// }
	// sockaddr_in tempCli;
	// tempCli.sin_family = AF_INET;
	// tempCli.sin_addr.s_addr = htonl(INADDR_ANY);
	// tempCli.sin_port = htons(1333); // should get from client
 //  	newRequest->setReadAddr(tempCli);
 //  	//Bind Socket
	// if ( bind(newRequest->getReadSocket(),(struct sockaddr *) &(newRequest->getReadAddr()), sizeof(newRequest->getReadAddr())) < 0)
	// {
	// 	perror("bind failed on read\n");
	// 	delete newRequest;
	// 	return NULL;	//return false if socket could not be bound
	// }

 //  	/* create socket for writing */
	// newRequest->setWritePort(1322);
	// if((newRequest->setWriteSocket(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
	// {
	// 	printf("write socket creation failed\n");
	// 	delete newRequest;
	// 	return NULL; 		// return NULL on error
	// }
	// tempCli.sin_port = htons(1322);
 //  	newRequest->setWriteAddr(tempCli);
 //  	//Bind Socket
	// if ( bind(newRequest->getWriteSocket(),(struct sockaddr *) &(newRequest->getWriteAddr()), sizeof(newRequest->getWriteAddr())) < 0)
	// {
	// 	perror("bind failed on write\n");
	// 	delete newRequest;
	// 	return NULL;	//return false if socket could not be bound
	// }

	/* create socket */
	newRequest->setPort(1333); // this needs to be dynamic
	if((newRequest->setSocket(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
	{
		printf("read socket creation failed\n");
		delete newRequest;
		return NULL; 		// return NULL on error
	}
	sockaddr_in tempCli;
	tempCli.sin_family = AF_INET;
	tempCli.sin_addr.s_addr = htonl(INADDR_ANY);
	tempCli.sin_port = htons(newRequest->getPort());
  	newRequest->setAddr(tempCli);
	//Bind Socket
	if (bind(newRequest->getSocket(),(struct sockaddr *) &(newRequest->getAddr()), sizeof(newRequest->getAddr())) < 0)
	{
		perror("bind failed on read\n");
		delete newRequest;
		return NULL;	//return false if socket could not be bound
	}

  	/* create Serv address */
  	struct sockaddr_in tempServ;
  	// tempServ.sin_family = AF_INET;
  	tempServ.sin_addr.s_addr = inet_addr(ip.c_str());
  	tempServ.sin_port = htons(port);
  	newRequest->setServAddr(tempServ);

	/***************************************/
	/*	Start Threads					   */
	/***************************************/

	/* start a thread that will continue to read for messages */
	pthread_t readThread = newRequest->getReadThread();
	if( (pthread_create(&readThread, NULL, readMessage, (void*)(newRequest))) < 0)
	{
		perror("Read Pthread create failed");
		delete newRequest;
		return NULL;
	}
	else if((pthread_detach(newRequest->getReadThread())) < 0)
	{
		perror("Read Pthread detach failed");
		delete newRequest;
		return NULL;
	}

	/* start a thread that will continue to attempt to send messages */
	pthread_t writeThread = newRequest->getWriteThread();
	if( (pthread_create(&writeThread, NULL, writeMessage, (void*)(newRequest))) < 0)
	{
		perror("Write Pthread create failed");
		delete newRequest;
		return NULL;
	}
	else if((pthread_detach(newRequest->getWriteThread())) < 0)
	{
		perror("Write Pthread detach failed");
		delete newRequest;
		return NULL;
	}


	/* send new connection request */
	newRequest->toOutbox(new lsp_message(newRequest->getConnid(), newRequest->nextSeq(),""));
	return newRequest;
}

// Request Read. Returns NULL when connection lost
// Returns number of bytes read
int lsp_request_read(lsp_request* a_request, uint8_t* pld)
{
	lsp_message* message = a_request->fromInbox();
	if(message == NULL)
	{
		return -1;
	}
	*((string*)pld) = message->m_payload;		//convert the void* to a string pointer and set data to that of the string

	return message->m_bytesRead;
}

// Request Write. Should not send NULL
bool lsp_request_write(lsp_request* a_request, string pld, int lth)
{
	// string payload = *((string*)pld);
	/*perform check on input*/

	/* check if connection has been made */
	if(a_request->getConnid() == 0)
	{
		printf("write with id as 0\n");
		//if not then store messages in seperate queue until they can be added with corrent connection id 
		a_request->toWaitbox(new lsp_message(a_request->getConnid(),a_request->nextSeq(),pld));

	}
	else
	{	
		printf("write with id as > 0\n");
		a_request->toOutbox(new lsp_message(a_request->getConnid(),a_request->nextSeq(),pld));
	}
	return true;
}

// Close connection. Remember to free memory.
bool lsp_request_close(lsp_request* a_request)
{
	//go through and free all lsp_message
	// close(a_request->getReadSocket());
	// close(a_request->getWriteSocket());
	close(a_request->getSocket());
	delete a_request;
	return true;
}