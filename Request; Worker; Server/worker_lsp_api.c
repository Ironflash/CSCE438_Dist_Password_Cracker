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

//#define DEBUG // uncomment to turn on print outs
#ifdef DEBUG
#define DEBUG_MSG(str) do { std::cout << str << std::endl; } while( false )
#else
#define DEBUG_MSG(str) do { } while ( false )
#endif

using namespace std;

/* Reads a message from network */
void* readMessage(void* arg) 
{
	lsp_request* a_request = (lsp_request*) arg;
	const int MAX_BUFFER = 1000;
	void* buffer = malloc(MAX_BUFFER);

	//printf("Waiting\n");
	DEBUG_MSG("Waiting");

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
			//printf("Read: %d Bytes\n",num_read);
			DEBUG_MSG("Read: "<<num_read);
		}
		//Drop Percentage of Packets
		// get random numger 1 - 10
		int randNum = rand() % 10 + 1;
		if(randNum <= m_dropRate*10)
		{
			//drop packet
			//printf("Dropping Packet\n");
			DEBUG_MSG("Dropping Packet");
			continue;
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
	    //printf("End of marshalling\n");
	    DEBUG_MSG("End of marshalling");
	    // End of unmarshalling

	    //consruct message
	    //get payload from lsp_message
	    string payload =  msg.payload();
	    // *((string*)pld) = payload;		//convert the void* to a string pointer and set data to that of the string
	    //printf("payload: %s\n",payload.c_str());
	    DEBUG_MSG("payload: "<<payload.c_str());
		//get connection id from the packet
		uint32_t connid = msg.connid();
		// conn_id = &connid;
		//printf("Conn-id: %d\n",connid);
		DEBUG_MSG("Conn-id: "<<connid);

		//set current sequence number for that client
		uint32_t seqnum = msg.seqnum();
		//printf("Seqnum: %d\n",seqnum);
		DEBUG_MSG("Seqnum: "<<seqnum);
		
		/* check if message is an ACK */
		if(connid != 0 && seqnum != 0 && payload == "")
		{
			//printf("ACK received for message: %d",seqnum);
			DEBUG_MSG("ACK received for message: "<<seqnum);
			a_request->checkMessageAck(connid,seqnum);
		}
		/* check if this is a response to a connection request */
		// printf("Request info %d, %d, payload: %s\n",connid,seqnum, payload.c_str());
		else if(connid != 0 && seqnum == 0 && payload == "" && a_request->getConnid() == 0)
		{
			//check if already received response to connection response
			// if(a_request->connReqAcknowledged())
			// {
			// 	continue;
			// }
			//printf("Connection request response detected\n");
			DEBUG_MSG("Connection request response detected");
			a_request->setConnid(connid);
			a_request->waitingToOutbox();
			//set connection response received
			a_request->connectionWasAcknowledged(); 
		}
		else if(connid == 0 && seqnum == 0)	// somereason a server sent a message using connid 0
		{
			//ignore message ... should probably send back an error
		}
		else // is a normal message
		{
			/* check if message is a duplicate or out of order*/
			if(seqnum != a_request->getLastSeqnum()+1 && a_request->getLastSeqnum() > 0)
			{
				// drop the message
				//DEBUG_MSG("sequence incorrect was:" <<seqnum, <<"should of been: "<<a_request->getLastSeqnum());
				cout<<"sequence incorrect was:" <<seqnum <<"should of been: "<<(a_request->getLastSeqnum()+1);
				continue;
			}
			/* Add message to inbox */
			a_request->toInbox(new lsp_message(connid,seqnum,payload,num_read));
			
			// update the last sequnce number from server
			a_request->increaseLastSeqnum();

			/* keep track of most recently received data message */
			a_request->setMostRecentMessage(new lsp_message(connid,seqnum,""));	

			// data message has been received
			a_request->dataMessageWasReceived();

			//printf("sending to outbox\n");
			DEBUG_MSG("sending to outbox");
			/* add ACK to outbox */
			//printf("Sending ack message to outbox with id: %d\n",connid);
			DEBUG_MSG("Sending ack message to outbox with id: "<<connid);
			a_request->toOutbox(new lsp_message(connid,seqnum,""));	

		}	
		//printf("END OF READ\n");
		DEBUG_MSG("END OF READ");
	}
}

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
		//printf("Byte Size: %d\n",size);
		DEBUG_MSG("Byte Size: "<<size);
		void* buffer = malloc(size);
		if(!msg->SerializeToArray(buffer, size))
		{
			//printf("serialize failed\n");
			DEBUG_MSG("serialize failed");
			// return false;
		}
		//printf("Marshalled successfully\n");
		DEBUG_MSG("Marshalled successfully");
		// end of marshalling

		//printf("Attempting to send message\n");
		DEBUG_MSG("Attempting to send message");
		//printf("Size of pld: %d\n", (int)sizeof(pld));
		DEBUG_MSG("Size of pld: "<<(int)sizeof(pld));
		//printf("size of msg: %d\n", (int)sizeof(*msg));
		DEBUG_MSG("size of msg: "<<(int)sizeof(pld));

		// printf("Socket: %d\n",a_request->getSocket());

		int sent;
		//printf("Client Id: %d\n",connid);
		DEBUG_MSG("Client Id: "<<connid);
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
			//printf("Sent: %d bytes\n",sent);
			DEBUG_MSG("Sent: "<<sent);
		}
		//message is not an acknowledgement
		if(pld != "")
		{
			/* set message waiting */
			a_request->setMessageWaiting(message);
		}	

		// Free up memory that was allocated while marshalling
		free (buffer);
		delete msg;
		//printf("END OF WRITE\n");
		DEBUG_MSG("END OF WRITE");
	}
}

/* epoch timer will keep track of when no activity has occured for a set amount of time */
void* epochTimer(void* arg)
{
	lsp_request* a_request = (lsp_request*) arg;
	while(true)
	{
		/* only check every so often*/
		sleep(a_request->getEpoch());
		
		/*Resend a connection request, if the original connection request has not yet been acknowledged */
		// if(!a_request->connReqAcknowledged())
		if(a_request->getConnid() == 0)
		{
			//printf("resending connection request\n");
			DEBUG_MSG("resending connection request");
			a_request->toOutbox(new lsp_message(0,0,""));
		}
		/*Send an acknowledgment message for the most recently received data message, or an acknowledgment with sequence number 0 if no data messages have been received*/
		lsp_message* message = a_request->getMostRecentMessage();
		if(message != NULL)
		{
			//printf("ack most recent data message %s\n",message->m_payload.c_str());
			DEBUG_MSG("ack most recent data message"<<message->m_payload.c_str());
			a_request->toOutbox(message);
		}
		else if(!a_request->dataMessageReceived() && a_request->getConnid() != 0)
 		{
 			//printf("sending keep alive signal\n");
 			DEBUG_MSG("sending keep alive signal");
 			a_request->toOutbox(new lsp_message(a_request->getConnid(),0,""));
		}
		/*If a data message has been sent, but not yet acknowledged, then resend the data message */
		if(!a_request->messageAcknowledged())
		{
			lsp_message* message = a_request->getMessageWaiting();
			if(message != NULL)
			{
				uint32_t connid = message->m_connid;
				//increase the number of no responses from a client
				a_request->noResponse();
				//determin if the number of no responses is above the threshhold
				if(a_request->serverAboveThreshhold())
				{
					a_request->dropServer();
				}
				//printf("resending unacknowledged message %s\n",message->m_payload.c_str());
				DEBUG_MSG("resending unacknowledged message "<<message->m_payload.c_str());
				a_request->toOutbox(a_request->getMessageWaiting());
			}
		}
	}
}

lsp_request* lsp_request_create(const char* dest, int port)
{
	lsp_request* newRequest = new lsp_request(); 
	if(newRequest == NULL)
	{
		//printf("space allocation for request failed\n");
		DEBUG_MSG("space allocation for request failed");
		delete newRequest;
		return NULL;		//return NULL if memory could not be allocated
	}
	
	struct hostent* host;
	if((host = (struct hostent*) gethostbyname(dest))<0)
	{
		return NULL;		//server name couldn't be resolved
	}

	// create socket
	//newRequest->setPort(1334); // this needs to be dynamic

	// dynamic port creation:
	int regenerate_port = 6000;
	sockaddr_in tempCli;
	memset(&tempCli, 0, sizeof(tempCli)); // Zero out address
	tempCli.sin_family = AF_INET;
	//memcpy(&tempCli.sin_addr, host->h_addr, host->h_length);
	tempCli.sin_addr.s_addr = htonl(INADDR_ANY);
	//tempCli.sin_port = pse->s_port;
		
	for (int i=0; i<30; i++) { //try 30 times
		DEBUG_MSG("try # "<<i);
		newRequest->setPort(regenerate_port); // this needs to be dynamic
		if((newRequest->setSocket(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0) {
			DEBUG_MSG("read socket creation failed");
			delete newRequest;
			return NULL; 		// return NULL on error
		}
		
		tempCli.sin_port = htons(newRequest->getPort());
	  	newRequest->setAddr(tempCli);
	  	//Bind Socket
		if ( bind(newRequest->getSocket(),(struct sockaddr *) &(newRequest->getAddr()), sizeof(newRequest->getAddr())) < 0) {
			DEBUG_MSG("bind failed on read");
			//delete newRequest;
			regenerate_port++;
			if (i == 29) {
				delete newRequest;
				return NULL;	//return false if socket could not be bound
			}
		} else {
			break;
		}
	}

	string ip = "127.0.0.1"; //temp

  	// create Serv address
  	struct sockaddr_in tempServ;
  	// tempServ.sin_family = AF_INET;
  	// tempServ.sin_addr.s_addr = inet_addr(host->h_addr); // uncomment for use on multiple machines
  	tempServ.sin_addr.s_addr = inet_addr(ip.c_str()); // comment for use on multiple machines
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

	/* start a thread that will maintain an epoch timer */
	pthread_t epochThread = newRequest->getEpochThread();
	if( (pthread_create(&epochThread, NULL, epochTimer, (void*)(newRequest))) < 0)
	{
		perror("Epoch Pthread create failed");
		delete newRequest;
		return NULL;
	}
	else if((pthread_detach(newRequest->getEpochThread())) < 0)
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
int lsp_request_read(lsp_request* a_request, void* pld)
{
	lsp_message* message = a_request->fromInbox();
	if(message == NULL)
	{
		return -1;
	}
	//printf("non null message from inbox\n");
	DEBUG_MSG("non null message from inbox");
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
		//printf("write with id as 0\n");
		DEBUG_MSG("write with id as 0");
		//if not then store messages in seperate queue until they can be added with corrent connection id 
		a_request->toWaitbox(new lsp_message(a_request->getConnid(),a_request->nextSeq(),pld));

	}
	else
	{	
		//printf("write with id as > 0\n");
		DEBUG_MSG("write with id as > 0");
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