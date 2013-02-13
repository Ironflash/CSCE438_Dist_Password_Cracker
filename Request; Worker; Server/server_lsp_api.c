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

//#define DEBUG // uncomment to turn on print outs
#ifdef DEBUG
#define DEBUG_MSG(str) do { std::cout << str << std::endl; } while( false )
#else
#define DEBUG_MSG(str) do { } while ( false )
#endif

bool sentReq = false;
bool sentReqAck = false;
bool sentWorkAck = false;


/* Reads a request message from network */
void* readReqMessage(void* arg) 
{
	lsp_server* a_srv = (lsp_server*) arg;
	const int MAX_BUFFER = 1000;
	void* buffer = malloc(MAX_BUFFER);

	//printf("Waiting\n");
	DEBUG_MSG("Waiting");

	
	socklen_t sockLen = sizeof(sockaddr_in);
	int num_read;
	//keep reading for messages
	while(true)
	{
		/* end thread if flagged*/
		if(a_srv->shouldEndThreads())
		{
			break;
		}
		sockaddr_in* tempCli = (sockaddr_in*)malloc(sockLen);
		//get packet from socket
		if((num_read = recvfrom(a_srv->getReqSocket(), buffer , MAX_BUFFER, 0,	
	                 (struct sockaddr *) tempCli, &sockLen)) < 0)
		{
			/* end thread if flagged*/
			if(a_srv->shouldEndThreads())
			{
				break;
			}
			perror("Unable to read\n");
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
			printf("Dropping Packet\n");
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
	    // printf("End of marshalling\n");
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
		// check for keep alive, must be checked before checking for out of order
		if(connid != 0 && seqnum == 0) // client is still alive message
		{
			//printf("received keep alive\n");
			DEBUG_MSG("received keep alive from: "<<connid);
			a_srv->receivedKeepAlive(connid);
		}
		
		/* check if it is a close notification */
		if(connid != 0 && seqnum == -1)
		{
			printf("Received close notification\n");
			a_srv->toReqAckbox(new lsp_message(connid,seqnum,""));
			continue;
		}
		/* check if message is an ACK */
		else if(connid != 0 && seqnum != 0 && payload == "")
		{
			a_srv->checkReqMessageAck(connid,seqnum);
			/*if it is an ACK then attempt to get next message */
			continue;
		}	
		/* check if this is a connection request */
		// printf("Request info %d, %d, payload: %s\n",connid,seqnum, payload.c_str());
		else if(connid == 0 && seqnum == 0 && payload == "")
		{
			printf("Connection request detected from request\n");
			/* Assign connection id to connection*/
			//check for any ids that have been freed by disconnects
			// if(a_srv->hasReqDisconnect())
			// {
			// 	connid = a_srv->nextReqDis();
			// }
			// else
			// {
				connid = a_srv->nextReqId();
			// }
			/* add client to list of clients*/
			// printf("Client Address Port from read: %d\n",ntohs(tempCli->sin_port));
			// printf("Client Id from read: %d\n",connid);
			a_srv->toCliAddr(connid,tempCli);
			a_srv->updateClientSeqnum(connid,seqnum);
			/* add worker to list of workers */
			a_srv->addRequest(connid);
			/* Add message to inbox */
			//have to call after client address has been added 
			// a_srv->toInbox(new lsp_message(0,seqnum,payload,num_read));

			/* add id to a list of ids that have not yet received a message on */
			a_srv->awaitingReqMessage(connid);
		}
		else if(connid == 0 && seqnum == 0)	// somereason a client sent a message using connid 0 other than connecting
		{
			//ignore message ... should probably send back an error
			continue;
		}
		else
		{
			/* check if message is a duplicate or out of order*/
			if(seqnum != a_srv->getCliSeqnum(connid)+1 && a_srv->getCliSeqnum(connid) > 0)
			{
				// drop the message
				continue;
			}
			/* Add message to inbox */
			a_srv->toReqInbox(new lsp_message(connid,seqnum,payload,num_read));

			/* Remove message from list of ids that have not received a message */
			a_srv->removeAwaitingReqMessage(connid);

			/* update the seqnum of the client */
			a_srv->updateClientSeqnum(connid,seqnum);

			/* keep track of most recently received data message */
			a_srv->setMostRecentReqMessage(new lsp_message(connid,seqnum,""));
		}
		// printf("sending to outbox\n");
		/* add ACK to outbox */
		// printf("Sending request message to outbox with id: %d\n",connid);
		a_srv->toReqAckbox(new lsp_message(connid,seqnum,""));	
		
		//printf("END OF READ\n");
		DEBUG_MSG("END OF READ");
	}
}

/* Reads a worker message from network */
void* readWorkMessage(void* arg) 
{
	lsp_server* a_srv = (lsp_server*) arg;
	const int MAX_BUFFER = 1000;
	void* buffer = malloc(MAX_BUFFER);

	//printf("Waiting\n");
	DEBUG_MSG("Waiting");

	
	
	socklen_t sockLen = sizeof(sockaddr_in);
	int num_read;
	//keep reading for messages
	while(true)
	{
		/* end thread if flagged*/
		if(a_srv->shouldEndThreads())
		{
			printf("Break Worker Read Thread\n");
			break;
		}
		sockaddr_in* tempCli = (sockaddr_in*)malloc(sockLen);
		//get packet from socket
		if((num_read = recvfrom(a_srv->getWorkSocket(), buffer , MAX_BUFFER, 0,	
	                 (struct sockaddr *) tempCli, &sockLen)) < 0)
		{
			/* end thread if flagged*/
			if(a_srv->shouldEndThreads())
			{
				printf("Break Worker Read Thread\n");
				break;
			}
			perror("Unable to read\n");
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
			printf("Dropping Packet\n");
			continue;
		}
		// Code to unmarshall a lsp_message ... still in progress 
		GOOGLE_PROTOBUF_VERIFY_VERSION;
		lspMessage::LspMessage msg;

		if(!msg.ParseFromArray(buffer,num_read))
	    {
	      fprintf(stderr, "error unpacking incoming message\n");
	    }
	    // printf("End of marshalling\n");
	    // End of unmarshalling

	    //consruct message
	    //get payload from lsp_message
	    string payload =  msg.payload();
	    //printf("payload: %s\n",payload.c_str());
	    DEBUG_MSG("payload: "<<payload.c_str());
		//get connection id from the packet
		uint32_t connid = msg.connid();
		//printf("Conn-id: %d\n",connid);
		DEBUG_MSG("Conn-id: "<<connid);

		//set current sequence number for that client
		uint32_t seqnum = msg.seqnum();
		//printf("Seqnum: %d\n",seqnum);
		DEBUG_MSG("Seqnum: "<<seqnum);
		// check for keep alive, must be checked before checking for out of order
		if(connid != 0 && seqnum == 0) // client is still alive message
		{
			//printf("received keep alive\n");
			DEBUG_MSG("received keep alive");
			a_srv->receivedKeepAlive(connid);
		}
		if(connid != 0 && seqnum != 0 && payload == "")
		{
			a_srv->checkWorkMessageAck(connid,seqnum);
			// printf("rached\n");
			/*if it is an ACK then attempt to get next message */
			continue;
		}
		/* check if this is a connection request */
		else if(connid == 0 && seqnum == 0 && payload == "")
		{
			printf("Connection request detected from worker\n");
			/* Assign connection id to connection*/
			//check for any ids that have been freed by disconnects
			// if(a_srv->hasWorkDisconnect())
			// {
			// 	connid = a_srv->nextWorkDis();
			// }
			// else
			// {
				connid = a_srv->nextWorkId();
			// }
			/* add client to list of clients*/
			a_srv->toCliAddr(connid,tempCli);
			a_srv->updateClientSeqnum(connid,seqnum);
			/* add worker to list of workers */
			a_srv->addWorker(connid);
			/* Add message to inbox */
			//have to call after client address has been added 
			// a_srv->toInbox(new lsp_message(0,seqnum,payload,num_read));

			/* add id to a list of ids that have not yet received a message on */
			a_srv->awaitingWorkMessage(connid);
		}
		else if(connid == 0 && seqnum == 0)	// somereason a client sent a message using connid 0 other than connecting
		{
			//ignore message ... should probably send back an error
			continue;
		}
		else if(payload != "")
		{
			printf("Worker message with payload %s received\n",payload.c_str());
			/* check if message is a duplicate or out of order*/
			if(seqnum != a_srv->getCliSeqnum(connid)+1 && a_srv->getCliSeqnum(connid) > 0)
			{
				printf("Duplicate or out of order work message\n");
				// drop the message
				continue;
			}
			/* Add message to inbox */
			a_srv->toWorkInbox(new lsp_message(connid,seqnum,payload,num_read));

			/* Remove message from list of ids that have not received a message */
			a_srv->removeAwaitingWorkMessage(connid);

			/* update the seqnum of the client */
			a_srv->updateClientSeqnum(connid,seqnum);

			/* keep track of most recently received data message */
			a_srv->setMostRecentWorkMessage(new lsp_message(connid,seqnum,""));	

		}
		/* add ACK to outbox */
		// printf("Sending worker message to outbox with id: %d\n",connid);
		a_srv->toWorkAckbox(new lsp_message(connid,seqnum,""));	
		
		//printf("END OF READ\n");
		DEBUG_MSG("END OF READ");
	}
}

/* sends a message over the network*/
void* writeReqMessage(void* arg)
{
	lsp_server* a_srv = (lsp_server*) arg;

	/* continually try to send messages */
	while(true)
	{
		lsp_message* message;
		/* Check for if last message has reveived ACK, if not DO NOT get another message */
		if(!a_srv->reqMessageAcknowledged())
		{
			/* get an ack to send */
			message = a_srv->fromReqAckbox();
			if(message == NULL)
			{
				continue;
			}	
			sentReqAck = true;
			printf("Writing req ack\n");
		}
		else 
		{
			if(!sentReqAck)
			{
				/* get an ack to send */
				message = a_srv->fromReqAckbox();
				if(message == NULL)
				{
					message = a_srv->fromReqOutbox();
					if(message == NULL)
					{
						continue;
					}	
				}	
				sentReqAck = true;
				printf("Writing req ack\n");
			}
			else 
			{
				message = a_srv->fromReqOutbox();
				if(message == NULL)
				{
					message = a_srv->fromReqAckbox();
					if(message == NULL)
					{
						continue;
					}
				}
				sentReqAck = false;
				printf("Writing req message\n");
			}
		}
		// printf("reached\n");
		// DEBUG_MSG("reached");
		// message = a_srv->fromReqOutbox();
		// if(message == NULL)
		// 	{
		// 		continue;
		// 	}	

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
		// printf("Marshalled successfully\n");
		// end of marshalling


		//printf("Attempting to send message\n");
		DEBUG_MSG("Attempting to send message");
		// printf("Size of pld: %d\n", sizeof(pld));
		// printf("size of msg: %d\n", sizeof(*msg));

		// printf("Socket: %d\n",a_request->getSocket());

		int sent;
		//printf("Client Id: %d\n",connid);
		DEBUG_MSG("Client Id: "<<connid);
		// pthread_mutex_lock(a_srv->getReqRWLock());
		sockaddr_in* cliAddr = a_srv->getCliAddr(connid);
		if(cliAddr == NULL)
		{
			//printf("NULL client address \n");
			DEBUG_MSG("NULL client address");
			free (buffer);
			delete msg;
			continue;
		}
		// printf("Client Address Port: %d\n",ntohs(cliAddr->sin_port));

		DEBUG_MSG("Seqnum: "<<seqnum);
		DEBUG_MSG("Payload: "<<pld);
		/* end thread if flagged*/
		if(a_srv->shouldEndThreads())
		{
			break;
		}
		if((sent = sendto(a_srv->getReqSocket(), buffer, size, 0, (struct sockaddr *)cliAddr, sizeof(*cliAddr))) < 0) //need to get socket of client
		{
			perror("Req Sendto failed");
			if(cliAddr == NULL)
			{
				printf("Was attempting to use a NULL address");
			}
			free (buffer);
			delete msg;
			continue;
		   // return false;
		}
		else
		{
			//printf("Sent: %d bytes\n",sent);
			DEBUG_MSG("Sent: "<<sent);
		}
		// pthread_mutex_unlock(a_srv->getReqRWLock());
		//check if ack for close notification was sent
		// if(seqnum == -1)
		// {
		// 	printf("ACK for cose notification sent, dropping connection\n");
		// 	//after ack has been sent the client can be dropped
		// 	a_srv->dropClient(connid);
		// }
		// message is not an acknowledgment
		if(pld != "")
		{
			/* set message waiting */
			a_srv->setReqMessageWaiting(message);
			a_srv->dataWasSentTo(connid);
		}
		
		// Free up memory that was allocated while marshalling
		free (buffer);
		delete msg;
		//printf("END OF WRITE\n");
		DEBUG_MSG("END OF WRITE");
	}
}

/* sends a message over the network*/
void* writeWorkMessage(void* arg)
{
	lsp_server* a_srv = (lsp_server*) arg;

	/* continually try to send messages */
	while(true)
	{
		lsp_message* message;
		/* Check for if last message has reveived ACK, if not DO NOT get another message */
		if(!a_srv->workMessageAcknowledged())
		{
			/* get an ack to send */
			message = a_srv->fromWorkAckbox();
			if(message == NULL)
			{
				continue;
			}	
			sentWorkAck = true;
			printf("Writing work ack\n");
		}
		else 
		{
			if(!sentWorkAck)
			{
				/* get an ack to send */
				message = a_srv->fromWorkAckbox();
				if(message == NULL)
				{
					message = a_srv->fromWorkOutbox();
					if(message == NULL)
					{
						continue;
					}	
				}	
				sentWorkAck = true;
				printf("Writing work ack\n");
			}
			else 
			{
				message = a_srv->fromWorkOutbox();
				if(message == NULL)
				{
					message = a_srv->fromWorkAckbox();
					if(message == NULL)
					{
						continue;
					}
				}
				sentWorkAck = false;
				printf("Writing work message\n");
			}
		}
		// printf("reached\n");
		// DEBUG_MSG("reached");
		// message = a_srv->fromWorkOutbox();
		// 	if(message == NULL)
		// 	{
		// 		continue;
		// 	}	
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
		// printf("Marshalled successfully\n");
		// end of marshalling


		//printf("Attempting to send message\n");
		DEBUG_MSG("Attempting to send message");
		// printf("Size of pld: %d\n", sizeof(pld));
		// printf("size of msg: %d\n", sizeof(*msg));

		// printf("Socket: %d\n",a_request->getSocket());

		int sent;
		//printf("Client Id: %d\n",connid);
		DEBUG_MSG("Client Id: "<<connid);
		// pthread_mutex_lock(a_srv->getWorkRWLock());
		sockaddr_in* cliAddr = a_srv->getCliAddr(connid);
		if(cliAddr == NULL)
		{
			//printf("NULL client address \n");
			DEBUG_MSG("NULL client address");
			free (buffer);
			delete msg;
			continue;
		}
		// printf("Client Address Port: %d\n",ntohs(cliAddr->sin_port));

		DEBUG_MSG("Seqnum: "<<seqnum);
		DEBUG_MSG("Payload: "<<pld);
		/* end thread if flagged*/
		if(a_srv->shouldEndThreads())
		{
			break;
		}
		if((sent = sendto(a_srv->getWorkSocket(), buffer, size, 0, (struct sockaddr *)cliAddr, sizeof(*cliAddr))) < 0) //need to get socket of client
		{
			perror("Worker Sendto failed");
			if(cliAddr == NULL)
			{
				printf("Was attempting to use a NULL address");
			}
			free (buffer);
			delete msg;
			continue;
		   // return false;
		}
		else
		{
			//printf("Sent: %d bytes\n",sent);
			DEBUG_MSG("Sent: "<<sent);
		}
		// pthread_mutex_unlock(a_srv->getWorkRWLock());
		// message is not an acknowledgment
		if(pld != "")
		{
			/* set message waiting */
			a_srv->setWorkMessageWaiting(message);
			a_srv->dataWasSentTo(connid);
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
	lsp_server* a_srv = (lsp_server*) arg;
	while(true)
	{
		/* end thread if flagged*/
		if(a_srv->shouldEndThreads())
		{
			break;
		}
		/* only check every so often*/
		sleep(a_srv->getEpoch());
		//printf("epoch has started\n");
		DEBUG_MSG("epoch has started");
		/*Acknowledge the connection request, if no data messages have been received.*/
		vector<uint32_t> ids = a_srv->getAwaitingReqMessages();
		if(!ids.empty())
		{
			//printf("sending ack because no data received\n");
			
			// send a response to every connection that has not received a response
			for(int i = 0; i < ids.size(); i++)
			{
				if(!a_srv->requestDis(ids[i]))
				{
					// uint32_t seqnum = a_srv->getCliSeqnum(ids[i]);
					// if(seqnum >= 0)
					// {
						// a_srv->toReqOutbox(new lsp_message(ids[i],seqnum,""));
					DEBUG_MSG("sending request connection response because no data received "<<ids[i]);
						a_srv->toReqAckbox(new lsp_message(ids[i],0,""));
					// }
				}
			}
		}
		/*Acknowledge the connection request, if no data messages have been received.*/
		ids = a_srv->getAwaitingWorkMessages();
		if(!ids.empty())
		{
			//printf("sending ack because no data received\n");
			
			// send a response to every connection that has not received a response
			for(int i = 0; i < ids.size(); i++)
			{
				if(!a_srv->workerDis(ids[i]))
				{
					// uint32_t seqnum = a_srv->getCliSeqnum(ids[i]);
					// if(seqnum >= 0)
					// {
						// a_srv->toWorkOutbox(new lsp_message(ids[i],seqnum,""));
					DEBUG_MSG("sending worker connection response because no data received "<<ids[i]);
						a_srv->toWorkAckbox(new lsp_message(ids[i],0,""));
					// }
				}
			}
		}
		//printf("one\n");
		// DEBUG_MSG("one");
		/*Acknowledge the most recently received data message, if any.*/
		lsp_message* message = a_srv->getMostRecentReqMessage();
		if(message != NULL)
		{
			printf("ack most recent Req data message %s\n",message->m_payload.c_str());
			// DEBUG_MSG("ack most recent data message");
			message->m_payload = "";
			a_srv->toReqAckbox(message);
		}
		message = a_srv->getMostRecentWorkMessage();
		if(message != NULL)
		{
			printf("ack most recent Worker data message %s\n",message->m_payload.c_str());
			// DEBUG_MSG("ack most recent data message");
			a_srv->toWorkAckbox(message);
		}
		//printf("two\n");
		// DEBUG_MSG("two");
		/*If a data message has been sent, but not yet acknowledged, then resend
			the data message. */
		if(!a_srv->reqMessageAcknowledged())
		{
			lsp_message* message = a_srv->getReqMessageWaiting();
			if(message != NULL)
			{
				uint32_t connid = message->m_connid;
				//increase the number of no responses from a client
				a_srv->noResponse(connid);
				//determin if the number of no responses is above the threshhold
				if(a_srv->clientAboveThreshhold(connid))
				{
					// pthread_mutex_lock(a_srv->getReqRWLock());
					a_srv->dropClient(connid);
					// pthread_mutex_unlock(a_srv->getReqRWLock());
				}
				printf("resending unacknowledged request message: %s\n",message->m_payload.c_str());
				// DEBUG_MSG("resending unacknowledged message");
				a_srv->toReqAckbox(message);
			}
		}
		if(!a_srv->workMessageAcknowledged())
		{
			lsp_message* message = a_srv->getWorkMessageWaiting();
			if(message != NULL)
			{
				uint32_t connid = message->m_connid;
				//increase the number of no responses from a client
				a_srv->noResponse(connid);
				//determin if the number of no responses is above the threshhold
				if(a_srv->clientAboveThreshhold(connid))
				{
					// pthread_mutex_lock(a_srv->getWorkRWLock());
					a_srv->dropClient(connid);
					// pthread_mutex_unlock(a_srv->getWorkRWLock());
				}
				// printf("resending unacknowledged work message: %s\n",message->m_payload.c_str());
				// DEBUG_MSG("resending unacknowledged message");
				a_srv->toWorkAckbox(message);
			}
		}
		//printf("three\n");
		// DEBUG_MSG("three");
		/* If no data has been sent to a client than check for a keep alive signal */
		ids = a_srv->getRequests();
		for(int i = 0; i < ids.size(); i++)
		{
			//no data sent to client
			if(!a_srv->dataSentTo(ids[i]) && !a_srv->keepAliveReceived(ids[i]))
			{
				a_srv->incNoKeepAlive(ids[i]);
				if(a_srv->clientAboveKAThreshhold(ids[i]))
				{
					// pthread_mutex_lock(a_srv->getReqRWLock());
					a_srv->dropClient(ids[i]);
					// pthread_mutex_unlock(a_srv->getReqRWLock());
				}
			}
		}
		ids = a_srv->getWorkers();
		for(int i = 0; i < ids.size(); i++)
		{
			//no data sent to client
			if(!a_srv->dataSentTo(ids[i]) && !a_srv->keepAliveReceived(ids[i]))
			{
				a_srv->incNoKeepAlive(ids[i]);
				if(a_srv->clientAboveKAThreshhold(ids[i]))
				{
					// pthread_mutex_lock(a_srv->getWorkRWLock());
					a_srv->dropClient(ids[i]);
					// pthread_mutex_unlock(a_srv->getWorkRWLock());
				}
			}
		}
	}


}

// Sets up server and returns NULL if server could not be started
lsp_server* lsp_server_create(int port)
{
	lsp_server* newServer = new lsp_server(); 
	if(newServer == NULL)
	{
		//printf("space allocation for server failed\n");
		DEBUG_MSG("space allocation for server failed");
		delete newServer;
		return NULL;		//return NULL if memory could not be allocated
	}
	/* Set up  socket for requests */
	newServer->setReqPort(port);
	if((newServer->setReqSocket(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
	{
		//printf("socket creation failed\n");
		DEBUG_MSG("socket creation failed");
		delete newServer;
		return NULL; // return Null on error
	}
	sockaddr_in tempServ;
	tempServ.sin_family = AF_INET;
	tempServ.sin_addr.s_addr = htonl(INADDR_ANY);
	// tempServ.sin_port = htons(1234);	// should get from server
	tempServ.sin_port = htons(port);
	newServer->setReqAddr(tempServ);

	//Bind Socket
	if ( bind(newServer->getReqSocket(),(struct sockaddr *) &(newServer->getReqAddr()), sizeof(newServer->getReqAddr())) < 0)
	{
		perror("bind failed");
		delete newServer;
		return NULL;	//return NULL if socket could not be bound
	}

	/* Set up socket for workers */
	newServer->setWorkPort(port);
	if((newServer->setWorkSocket(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
	{
		//printf("socket creation failed\n");
		DEBUG_MSG("socket creation failed");
		delete newServer;
		return NULL; // return Null on error
	}
	// tempServ.sin_port = htons(1235);	// should get from server
	tempServ.sin_port = htons(port+1);	// should get from server
	newServer->setWorkAddr(tempServ);

	//Bind Socket
	if ( bind(newServer->getWorkSocket(),(struct sockaddr *) &(newServer->getWorkAddr()), sizeof(newServer->getWorkAddr())) < 0)
	{
		perror("bind failed");
		delete newServer;
		return NULL;	//return NULL if socket could not be bound
	}

	/* Set up write socket */
	// newServer->setWritePort(port);
	// if((newServer->setWriteSocket(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
	// {
	// 	//printf("socket creation failed\n");
	// 	DEBUG_MSG("socket creation failed");
	// 	delete newServer;
	// 	return NULL; // return Null on error
	// }
	// tempServ.sin_port = htons(1236);	// should get from server
	// newServer->setWriteAddr(tempServ);

	// //Bind Socket
	// if ( bind(newServer->getWriteSocket(),(struct sockaddr *) &(newServer->getWriteAddr()), sizeof(newServer->getWriteAddr())) < 0)
	// {
	// 	perror("write bind failed");
	// 	delete newServer;
	// 	return NULL;	//return NULL if socket could not be bound
	// }

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
	pthread_t writeReqThread = newServer->getWriteReqThread();
	if( (pthread_create(&writeReqThread, NULL, writeReqMessage, (void*)(newServer))) < 0)
	{
		perror("Write Pthread create failed");
		delete newServer;
		return NULL;
	}
	else if((pthread_detach(newServer->getWriteReqThread())) < 0)
	{
		perror("Write Pthread detach failed");
		delete newServer;
		return NULL;
	}

	pthread_t writeWorkThread = newServer->getWriteWorkThread();
	if( (pthread_create(&writeWorkThread, NULL, writeWorkMessage, (void*)(newServer))) < 0)
	{
		perror("Write Pthread create failed");
		delete newServer;
		return NULL;
	}
	else if((pthread_detach(newServer->getWriteWorkThread())) < 0)
	{
		perror("Write Pthread detach failed");
		delete newServer;
		return NULL;
	}

	/* start a thread that will maintain an epoch timer */
	pthread_t epochThread = newServer->getEpochThread();
	if( (pthread_create(&epochThread, NULL, epochTimer, (void*)(newServer))) < 0)
	{
		perror("Epoch Pthread create failed");
		delete newServer;
		return NULL;
	}
	else if((pthread_detach(newServer->getEpochThread())) < 0)
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
	lsp_message* message;
	//check if last message returned was from a request
	if(sentReq)
	{
		message = a_srv->fromWorkInbox();
		if(message == NULL)
		{
			message = a_srv->fromReqInbox();
			if(message == NULL)
			{
				return -1;
			}
		}
		sentReq = false;
	}	
	else
	{
		message = a_srv->fromReqInbox();
		if(message == NULL)
		{
			message = a_srv->fromWorkInbox();
			if(message == NULL)
			{
				return -1;
			}
		}
		sentReq = true;
	}
	
	*((string*)pld) = message->m_payload;		//convert the void* to a string pointer and set data to that of the string
	*conn_id = message->m_connid;

	return message->m_bytesRead;
}

// Server Write. Should not send NULL
// bool lsp_server_write(lsp_server* a_srv, void* pld, int lth, uint32_t conn_id)
bool lsp_server_write(lsp_server* a_srv, string payload, int lth, uint32_t conn_id)
{
	// string payload = *((string*)pld);
	/*perform check on input*/
	if(conn_id % 2 == 1)
	{
		if(a_srv->requestDis(conn_id))
		{
			return false;
		}
		a_srv->toReqOutbox(new lsp_message(conn_id,a_srv->nextSeq(conn_id),payload));
	}
	else
	{
		if(a_srv->workerDis(conn_id))
		{
			return false;
		}
		a_srv->toWorkOutbox(new lsp_message(conn_id,a_srv->nextSeq(conn_id),payload));
	}
	
	return true;
}

vector<uint32_t> lsp_server_requests(lsp_server* a_srv)
{
	return a_srv->getRequests();
}

vector<uint32_t> lsp_server_workers(lsp_server* a_srv)
{
	return a_srv->getWorkers();
}

/* returns a list of request ids that have disconnected*/
vector<uint32_t> lsp_server_requestDisconnects(lsp_server* a_srv)
{
	return a_srv->getRequestDis();
}

/* returns a list of worker ids that have disconnected*/
vector<uint32_t> lsp_server_workersDisconnects(lsp_server* a_srv)
{
	return a_srv->getWorkerDis();
}

// Close connection.
bool lsp_server_close(lsp_server* a_srv, uint32_t conn_id)
{
	a_srv->endThreads();
	close(a_srv->getReqSocket());
	close(a_srv->getWorkSocket());
	// close(a_srv->getWriteSocket());
	delete a_srv;
	return true;
}