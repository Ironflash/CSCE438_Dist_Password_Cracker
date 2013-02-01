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


/* Reads a request message from network */
void* readReqMessage(void* arg) 
{
	lsp_server* a_srv = (lsp_server*) arg;
	const int MAX_BUFFER = 1000;
	void* buffer = malloc(MAX_BUFFER);

	printf("Waiting\n");

	
	socklen_t sockLen = sizeof(sockaddr_in);
	int num_read;
	//keep reading for messages
	while(true)
	{
		sockaddr_in* tempCli = (sockaddr_in*)malloc(sockLen);
		//get packet from socket
		if((num_read = recvfrom(a_srv->getReadReqSocket(), buffer , MAX_BUFFER, 0,	
	                 (struct sockaddr *) tempCli, &sockLen)) < 0)
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
			a_srv->checkMessageAck(connid,seqnum);
			/*if it is an ACK then attempt to get next message */
			continue;
		}
		
		/* check if this is a connection request */
		printf("Request info %d, %d, payload: %s\n",connid,seqnum, payload.c_str());
		if(connid == 0 && seqnum == 0 && payload == "")
		{
			printf("Connection request detected\n");
			/* Assign connection id to connection*/
			//check for any ids that have been freed by disconnects
			if(a_srv->hasReqDisconnect())
			{
				connid = a_srv->nextReqDis();
			}
			else
			{
				connid = a_srv->nextReqId();
			}
			/* add client to list of clients*/
			printf("Client Address Port from read: %d\n",ntohs(tempCli->sin_port));
			printf("Client Id from read: %d\n",connid);
			a_srv->toCliAddr(connid,tempCli);
			/* Add message to inbox */
			//have to call after client address has been added 
			a_srv->toInbox(new lsp_message(0,seqnum,payload,num_read));
		}
		else if(connid == 0 && seqnum == 0)	// somereason a client sent a message using connid 0 other than connecting
		{
			//ignore message ... should probably send back an error
			continue;
		}
		else
		{
			/* Add message to inbox */
			a_srv->toInbox(new lsp_message(connid,seqnum,payload,num_read));
		}
		printf("sending to outbox\n");
		/* add ACK to outbox */
		printf("Sending request message to outbox with id: %d\n",connid);
		a_srv->toOutbox(new lsp_message(connid,seqnum,""));	
		printf("END OF READ\n");
	}
}

/* Reads a worker message from network */
void* readWorkMessage(void* arg) 
{
	lsp_server* a_srv = (lsp_server*) arg;
	const int MAX_BUFFER = 1000;
	void* buffer = malloc(MAX_BUFFER);

	printf("Waiting\n");

	
	
	socklen_t sockLen = sizeof(sockaddr_in);
	int num_read;
	//keep reading for messages
	while(true)
	{
		sockaddr_in* tempCli = (sockaddr_in*)malloc(sockLen);
		//get packet from socket
		if((num_read = recvfrom(a_srv->getReadWorkSocket(), buffer , MAX_BUFFER, 0,	
	                 (struct sockaddr *) tempCli, &sockLen)) < 0)
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

	    //consruct message
	    //get payload from lsp_message
	    string payload =  msg.payload();
	    printf("payload: %s\n",payload.c_str());
		//get connection id from the packet
		uint32_t connid = msg.connid();
		printf("Conn-id: %d\n",connid);

		//set current sequence number for that client
		uint32_t seqnum = msg.seqnum();
		printf("Seqnum: %d\n",seqnum);
		/* check if message is an ACK */
		if(connid != 0 && seqnum != 0 && payload == "")
		{
			a_srv->checkMessageAck(connid,seqnum);
			printf("rached\n");
			/*if it is an ACK then attempt to get next message */
			continue;
		}
		/* check if this is a connection request */
		if(connid == 0 && seqnum == 0 && payload == "")
		{
			/* Assign connection id to connection*/
			//check for any ids that have been freed by disconnects
			if(a_srv->hasWorkDisconnect())
			{
				connid = a_srv->nextWorkDis();
			}
			else
			{
				connid = a_srv->nextWorkId();
			}
			/* add client to list of clients*/
			a_srv->toCliAddr(connid,tempCli);
			/* Add message to inbox */
			//have to call after client address has been added 
			a_srv->toInbox(new lsp_message(0,seqnum,payload,num_read));
		}
		else if(connid == 0 && seqnum == 0)	// somereason a client sent a message using connid 0 other than connecting
		{
			//ignore message ... should probably send back an error
			continue;
		}
		else
		{
			/* Add message to inbox */
			a_srv->toInbox(new lsp_message(connid,seqnum,payload,num_read));
		}
		/* add ACK to outbox */
		printf("Sending worker message to outbox with id: %d\n",connid);
		a_srv->toOutbox(new lsp_message(connid,seqnum,""));	
		
		printf("END OF READ\n");
	}
}

/* sends a message over the network*/
void* writeMessage(void* arg)
{
	lsp_server* a_srv = (lsp_server*) arg;

	/* continually try to send messages */
	while(true)
	{
		/* Check for if last message has reveived ACK if not DO NOT get another message */
		if(!a_srv->messageAcknowledged())
		{
			continue;
		}
		/* get values from the message */
		lsp_message* message = a_srv->fromOutbox();
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
		printf("Size of pld: %d\n", sizeof(pld));
		printf("size of msg: %d\n", sizeof(*msg));

		// printf("Socket: %d\n",a_request->getSocket());

		int sent;
		printf("Client Id: %d\n",connid);
		sockaddr_in* cliAddr = a_srv->getCliAddr(connid);
		if(cliAddr == NULL)
		{
			printf("NULL client address \n");
			break;
		}
		printf("Client Address Port: %d\n",ntohs(cliAddr->sin_port));

		if((sent = sendto(a_srv->getWriteSocket(), buffer, size, 0, (struct sockaddr *)cliAddr, sizeof(*cliAddr))) < 0) //need to get socket of client
		{
			perror("Sendto failed");
		   // return false;
		}
		else
		{
			printf("Sent: %d bytes\n",sent);
		}
		/* set message waiting */
		a_srv->setMessageWaiting(message);
		// Free up memory that was allocated while marshalling
		free (buffer);
		delete msg;
		printf("END OF WRITE\n");
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
	/* Set up read socket for requests */
	newServer->setReadReqPort(port);
	if((newServer->setReadReqSocket(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
	{
		printf("socket creation failed\n");
		delete newServer;
		return NULL; // return Null on error
	}
	sockaddr_in tempServ;
	tempServ.sin_family = AF_INET;
	tempServ.sin_addr.s_addr = htonl(INADDR_ANY);
	tempServ.sin_port = htons(1234);	// should get from server
	newServer->setReadReqAddr(tempServ);

	//Bind Socket
	if ( bind(newServer->getReadReqSocket(),(struct sockaddr *) &(newServer->getReadReqAddr()), sizeof(newServer->getReadReqAddr())) < 0)
	{
		perror("bind failed");
		delete newServer;
		return NULL;	//return NULL if socket could not be bound
	}

	/* Set up read socket for workers */
	newServer->setReadWorkPort(port);
	if((newServer->setReadWorkSocket(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
	{
		printf("socket creation failed\n");
		delete newServer;
		return NULL; // return Null on error
	}
	tempServ.sin_port = htons(1235);	// should get from server
	newServer->setReadWorkAddr(tempServ);

	//Bind Socket
	if ( bind(newServer->getReadWorkSocket(),(struct sockaddr *) &(newServer->getReadWorkAddr()), sizeof(newServer->getReadWorkAddr())) < 0)
	{
		perror("bind failed");
		delete newServer;
		return NULL;	//return NULL if socket could not be bound
	}

	/* Set up write socket */
	newServer->setWritePort(port);
	if((newServer->setWriteSocket(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
	{
		printf("socket creation failed\n");
		delete newServer;
		return NULL; // return Null on error
	}
	tempServ.sin_port = htons(1236);	// should get from server
	newServer->setWriteAddr(tempServ);

	//Bind Socket
	if ( bind(newServer->getWriteSocket(),(struct sockaddr *) &(newServer->getWriteAddr()), sizeof(newServer->getWriteAddr())) < 0)
	{
		perror("write bind failed");
		delete newServer;
		return NULL;	//return NULL if socket could not be bound
	}

	/***************************************/
	/*	Start Threads					   */
	/***************************************/

	/* start a thread that will continue to read for request messages */
	pthread_t readReqThread = newServer->getReadReqThread();
	if( (pthread_create(&readReqThread, NULL, readReqMessage, (void*)(newServer))) < 0)
	{
		perror("Read Pthread create failed");
		delete newServer;
		return NULL;
	}
	else if((pthread_detach(newServer->getReadReqThread())) < 0)
	{
		perror("Read Pthread detach failed");
		delete newServer;
		return NULL;
	}

	/* start a thread that will continue to read for worker messages */
	pthread_t readWorkThread = newServer->getReadWorkThread();
	if( (pthread_create(&readWorkThread, NULL, readWorkMessage, (void*)(newServer))) < 0)
	{
		perror("Read Pthread create failed");
		delete newServer;
		return NULL;
	}
	else if((pthread_detach(newServer->getReadWorkThread())) < 0)
	{
		perror("Read Pthread detach failed");
		delete newServer;
		return NULL;
	}

	/* start a thread that will continue to attempt to send messages */
	pthread_t writeThread = newServer->getWriteThread();
	if( (pthread_create(&writeThread, NULL, writeMessage, (void*)(newServer))) < 0)
	{
		perror("Write Pthread create failed");
		delete newServer;
		return NULL;
	}
	else if((pthread_detach(newServer->getWriteThread())) < 0)
	{
		perror("Write Pthread detach failed");
		delete newServer;
		return NULL;
	}
	return newServer;
}

// Read from connection. Return NULL when connection lost
// Returns number of bytes read. conn_id is an output parameter
int lsp_server_read(lsp_server* a_srv, void* pld, uint32_t* conn_id)
{
	lsp_message* message = a_srv->fromInbox();
	if(message == NULL)
	{
		return -1;
	}
	*((string*)pld) = message->m_payload;		//convert the void* to a string pointer and set data to that of the string
	*conn_id = message->m_connid;

	return message->m_bytesRead;
}

// Server Write. Should not send NULL
bool lsp_server_write(lsp_server* a_srv, void* pld, int lth, uint32_t conn_id)
{
	string payload = *((string*)pld);
	/*perform check on input*/

	a_srv->toOutbox(new lsp_message(conn_id,a_srv->nextSeq(),payload));
	return true;
}

// Close connection.
bool lsp_server_close(const lsp_server* a_srv, uint32_t conn_id)
{
	close(a_srv->getReadReqSocket());
	close(a_srv->getReadWorkSocket());
	close(a_srv->getWriteSocket());
	delete a_srv;
	return true;
}