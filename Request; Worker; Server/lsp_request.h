#include <queue>
#include <pthread.h>
#include "lsp_globals.h"

//#define DEBUG // uncomment to turn on print outs
#ifdef DEBUG
#define DEBUG_MSG(str) do { std::cout << str << std::endl; } while( false )
#else
#define DEBUG_MSG(str) do { } while ( false )
#endif

class lsp_request
{
private:
	// int m_readPort;		//port for reading
	// int m_readSocket;		//socket for reading
	// int m_writePort;		//port for writing
	// int m_writeSocket;		//socket for writing
	int m_port;		// request's port
	int m_socket;		// request's socket
	// struct sockaddr_in m_readAddr,m_writeAddr, 
	struct sockaddr_in m_servAddr;	//address of the server 
	struct sockaddr_in m_addr; // address of client
	std::queue<lsp_message*> m_inbox;
	std::queue<lsp_message*> m_outbox;
	std::queue<lsp_message*> m_waitbox;		// holds messages until connection id has been assigned
	uint32_t m_connid;		//the connection id 
	pthread_t 	m_readThread;
	pthread_t   m_writeThread;
	pthread_t   m_epochThread;
	pthread_mutex_t m_waitingMessageLock;
	pthread_mutex_t m_outboxLock;
	lsp_message* m_messageWaiting;	// message waiting for an acknowledgment
	lsp_message* m_mostRecentMessage;
	bool m_messageAcknowledged;		// has last message been acknowledged
	// bool m_isMessageWaiting;
	bool m_connectionAcknowledged;
	bool m_dataMessRcvd;
	bool m_endThreads;				// flag for ending threads
	uint32_t m_nextSeqnum;
	uint32_t m_lastServerSeqnum;	// the last sequence number recieved from the server
	//int m_epoch;					// the number of seconds between epochs
	int m_numNoResponses;
	//int m_dropThreshhold;			// number of no repsonses before the connection is dropped
public:

	lsp_request()
	{
		m_connid = 0;
		m_nextSeqnum = 0;
		m_lastServerSeqnum = 0;
		m_numNoResponses = 0;
		//m_epoch = 2;		// epoch defaults to intervals of 2 seconds
		//m_dropThreshhold = 5; // default is 5 no responses
		m_messageAcknowledged = true;
		m_dataMessRcvd = false;
		m_endThreads = false;
		m_mostRecentMessage = NULL;
		// m_isMessageWaiting = false;
		m_connectionAcknowledged = false;
		pthread_mutex_init(&m_waitingMessageLock, NULL);
		pthread_mutex_init(&m_outboxLock, NULL);
	}
	~lsp_request()
	{
		pthread_mutex_destroy(&m_waitingMessageLock);
		pthread_mutex_destroy(&m_outboxLock);
	}
	/* setters */
	// void setReadPort(int port)
	// {
	// 	m_readPort = port;
	// }

	// int setReadSocket(int socket)
	// {
	// 	m_readSocket = socket;
	// 	if(m_readSocket < 0)
	// 	{
	// 		printf("socket creation error\n");
	// 	}
	// 	return m_readSocket;
	// }

	// void setReadAddr(struct sockaddr_in servaddr)
	// {
	// 	m_readAddr = servaddr;
	// }

	// void setWritePort(int port)
	// {
	// 	m_writePort = port;
	// }

	// int setWriteSocket(int socket)
	// {
	// 	m_writeSocket = socket;
	// 	return m_writeSocket;
	// }

	// void setWriteAddr(struct sockaddr_in servaddr)
	// {
	// 	m_writeAddr = servaddr;
	// }

	void setPort(int port)
	{
		m_port = port;
	}

	int setSocket(int socket)
	{
		m_socket = socket;
		if(m_socket < 0)
		{
			//printf("socket creation error\n");
			DEBUG_MSG("socket creation error");
		}
		return m_socket;
	}

	void setAddr(struct sockaddr_in servaddr)
	{
		m_addr = servaddr;
	}

	void setServAddr(struct sockaddr_in servaddr)
	{
		m_servAddr = servaddr;
	}

	void setConnid(uint32_t connid)
	{
		m_connid = connid;
	}

	/*
	void setEpoch(int seconds)
	{
		m_epoch = seconds;
	}

	void setDropThreshhold(int num)
	{
		m_dropThreshhold = num;
	}
	*/

	void setMostRecentMessage(lsp_message* message)
	{
		m_mostRecentMessage = new lsp_message(*message);
	}

	void toInbox(lsp_message* message)
	{
		//printf("Attempting to add to inbox\n");
		DEBUG_MSG("Attempting to add to inbox");
		m_inbox.push(message);
		//printf("Added to inbox\n");
		DEBUG_MSG("Added to inbox");
	}

	void toOutbox(lsp_message* message)
	{
		// pthread_mutex_lock(&m_outboxLock);
		m_outbox.push(message);
		// pthread_mutex_unlock(&m_outboxLock);
	}

	void toWaitbox(lsp_message* message)
	{
		//printf("Attempting to add to inbox\n");
		DEBUG_MSG("Attempting to add to inbox");
		m_waitbox.push(message);
	}

	void setMessageWaiting(lsp_message* message)
	{
		pthread_mutex_lock(&m_waitingMessageLock);
		m_messageWaiting = message;
		// m_isMessageWaiting = true;
		m_messageAcknowledged = false;
		pthread_mutex_unlock(&m_waitingMessageLock);
	}

	void increaseLastSeqnum()
	{
		m_lastServerSeqnum++;
	}

	/* getters */

	// int getReadPort() const
	// {
	// 	return m_readPort;
	// }

	// int getReadSocket() const
	// {
	// 	return m_readSocket;
	// }

	// struct sockaddr_in getReadAddr()
	// {
	// 	return m_readAddr;
	// }

	// int getWritePort() const
	// {
	// 	return m_writePort;
	// }

	// int getWriteSocket() const
	// {
	// 	return m_writeSocket;
	// }

	// struct sockaddr_in getWriteAddr()
	// {
	// 	return m_writeAddr;
	// }

	int getPort() const
	{
		return m_port;
	}

	int getSocket() const
	{
		return m_socket;
	}

	struct sockaddr_in getAddr()
	{
		return m_addr;
	}


	struct sockaddr_in getServAddr()
	{
		return m_servAddr;
	}

	uint32_t getConnid() const
	{
		return m_connid;
	}

	pthread_t getReadThread() const
	{
		return m_readThread;
	}

	pthread_t getWriteThread() const
	{
		return m_writeThread;
	}

	pthread_t getEpochThread() const
	{
		return m_epochThread;
	}

	uint32_t getLastSeqnum() const
	{
		return m_lastServerSeqnum;
	}

	int getEpoch() const
	{
		return m_epoch;
	}	

	lsp_message* getMostRecentMessage() const
	{
		return m_mostRecentMessage;
	}

	lsp_message* getMessageWaiting()
	{
		return m_messageWaiting;
	}

	lsp_message* fromInbox()
	{
		// pthread_mutex_lock(&m_inboxLock);
		if(m_inbox.empty())
		{
			return NULL;
		}
		lsp_message* result = m_inbox.front();
		m_inbox.pop();
		// pthread_mutex_unlock(&m_inboxLock);
		return result;
	}

	lsp_message* fromOutbox()
	{
		// pthread_mutex_lock(&m_outboxLock);
		if(m_outbox.empty())
		{
			return NULL;
		}
		lsp_message* result = m_outbox.front();
		m_outbox.pop();
		// pthread_mutex_unlock(&m_outboxLock);
		return result;
	}

	uint32_t nextSeq()
	{
		return m_nextSeqnum++;
	}	

	/* Other functions */

	void checkMessageAck(uint32_t connid, uint32_t seqnum)
	{
		pthread_mutex_lock(&m_waitingMessageLock);
		/* if message Waiting is null then it isn't an acknowledgement */
		if(m_messageWaiting == NULL)
		{
			//printf("Reached inner\n");
			DEBUG_MSG("Reached inner");
			pthread_mutex_unlock(&m_waitingMessageLock);
			return;
		}
		//printf("Message Waiting id: %d\n",m_messageWaiting->m_connid);
		DEBUG_MSG("Message Waiting id: "<<m_messageWaiting->m_connid);
		m_messageAcknowledged = (m_messageWaiting->m_connid == connid && m_messageWaiting->m_seqnum == seqnum);
		pthread_mutex_unlock(&m_waitingMessageLock);
		//printf("Reached 2\n");
		DEBUG_MSG("Reached 2");
		return;
	}
	bool messageAcknowledged()
	{
		return m_messageAcknowledged;
	}
	bool awaitingAck()
	{
		return (m_messageWaiting != NULL);
	}

	void waitingToOutbox()
	{
		// pthread_mutex_lock(&m_outboxLock);
		while(!m_waitbox.empty())
		{
			lsp_message* temp = m_waitbox.front();
			temp->m_connid = m_connid;
			m_outbox.push(temp);
			m_waitbox.pop();
		}
		// pthread_mutex_unlock(&m_outboxLock);
	}

	void connectionWasAcknowledged()
	{
		m_messageAcknowledged = true;
	}

	bool connReqAcknowledged() const
	{
		return m_messageAcknowledged;
	}

	void dataMessageWasReceived()
	{
		m_dataMessRcvd = true;
	}

	bool dataMessageReceived() const
	{
		return m_dataMessRcvd;
	}

	void noResponse()
	{
		m_numNoResponses++;
	}

	bool serverAboveThreshhold()
	{
		return m_numNoResponses > m_dropThreshhold;
	}

	void dropServer()
	{

		// 	//add to list of disconnected workers
		// 	m_requestDisconnects.push_back(connid);
		// 	//put id up for re-assignment
		// 	m_reqDis.push(connid);
		// 	//remove from list of current workers
		// 	std::vector<uint32_t>::iterator it;
		// 	it = std::find(m_requestIds.begin(), m_requestIds.end(), connid);
		// 	m_requestIds.erase(it);
		// /* remove the cli from the list of clients */
		// removeCliAddr(connid);
		// //required so that the server will go on and not wait for a reply
		// m_messageAcknowledged = true;
		//do something
	}

	void endThreads()
	{
		m_endThreads = true;
		// void* res;
		// pthread_join(m_readThread,&res);
		// printf("one\n");
		// pthread_join(m_writeThread,&res);
		// printf("two\n");
		// pthread_join(m_epochThread,&res);
		// printf("three\n");
	}

	bool shouldEndThreads()
	{
		return m_endThreads;
	}
};