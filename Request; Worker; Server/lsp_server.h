#include <queue>
#include <map>
#include <pthread.h>

/* Used to group together addr and socket of clients */
struct clientConnection
{
	sockaddr_in* addr;
	int socket;
};

class lsp_server
{
private:
	int m_readReqPort;		//port that will be used for listening 
	int m_readReqSocket;		//socket that will listen for incoming messages from requests
	int m_readWorkPort;		//port that will be used for listening
	int m_readWorkSocket;	//socket that will listen for incoming messages from workers
	int m_writePort;		//port that will be used for writing messages
	int m_writeSocket;	//socket that will write outgoing messages 
	sockaddr_in m_readReqAddr;	//address of the server for reading requests 
	sockaddr_in m_readWorkAddr;	//address of the server for reading workers
	sockaddr_in m_writeAddr;	//address of the server for writing
	std::queue<lsp_message*> m_inbox;
	std::queue<lsp_message*> m_outbox;
	std::queue<uint32_t> m_reqDis;		//store connection ids of disconnected requests
	std::queue<uint32_t> m_workDis;		//store connection ids of disconnected workers
	std::map<uint32_t, clientConnection> m_cliAddresses;
	pthread_t 	m_readReqThread;
	pthread_t 	m_readWorkThread;
	pthread_t   m_writeThread;
	uint32_t m_nextSeqnum;
	uint32_t m_nextReqId;		//next connection id to be assigned to request
	uint32_t m_nextWorkId;		//next connection id to be assigned to worker
	// pthread_mutex_t m_inboxLock;
	// pthread_mutex_t m_outboxLock;
	pthread_mutex_t m_waitingMessageLock;
	lsp_message* m_messageWaiting;	// message waiting for an acknowledgment
	bool m_messageAcknowledged;		// has last message been acknowledged
	bool m_isMessageWaiting;
public:

	lsp_server()
	{
		m_nextSeqnum = 1;
		m_nextReqId = 1;	//Request ids will be odd
		m_nextWorkId = 2;	//Worker ids will be even
		m_messageAcknowledged = true;
		m_isMessageWaiting = false;
		// pthread_mutex_init(&m_inboxLock, NULL);
		// pthread_mutex_init(&m_outboxLock, NULL);
		pthread_mutex_init(&m_waitingMessageLock, NULL);
	}
	~lsp_server()
	{
		//TODO go through all client addresses and delete address pointers maybe free because used malloc
		// pthread_mutex_destroy(&m_inboxLock);
		// pthread_mutex_destroy(&m_outboxLock);
		pthread_mutex_destroy(&m_waitingMessageLock);
	}
	/* setters */
	void setReadReqPort(int port)
	{
		m_readReqPort = port;
	}

	int setReadReqSocket(int socket)
	{
		m_readReqSocket = socket;
		return m_readReqSocket;
	}

	void setReadReqAddr(struct sockaddr_in servaddr)
	{
		m_readReqAddr = servaddr;
	}  

	void setReadWorkPort(int port)
	{
		m_readWorkPort = port;
	}

	int setReadWorkSocket(int socket)
	{
		m_readWorkSocket = socket;
		return m_readWorkSocket;
	}

	void setReadWorkAddr(struct sockaddr_in servaddr)
	{
		m_readWorkAddr = servaddr;
	}  

	void setWritePort(int port)
	{
		m_writePort = port;
	}

	int setWriteSocket(int socket)
	{
		m_writeSocket = socket;
		return m_writeSocket;
	}

	void setWriteAddr(struct sockaddr_in servaddr)
	{
		m_writeAddr = servaddr;
	}  


	void toCliAddr(uint32_t connid, sockaddr_in* addr)
	{
		m_cliAddresses[connid].addr = addr;
	}

	void toInbox(lsp_message* message)
	{
		printf("Attempting to add to inbox\n");
		// pthread_mutex_lock(&m_inboxLock);
		m_inbox.push(message);
		// pthread_mutex_unlock(&m_inboxLock);
		printf("Added to inbox\n");
	}

	void toOutbox(lsp_message* message)
	{
		// pthread_mutex_lock(&m_outboxLock);
		m_outbox.push(message);
		// pthread_mutex_unlock(&m_outboxLock);
	}

	void setMessageWaiting(lsp_message* message)
	{
		pthread_mutex_lock(&m_waitingMessageLock);
		m_messageWaiting = message;
		m_isMessageWaiting = true;
		pthread_mutex_unlock(&m_waitingMessageLock);
	}

	/* getters */

	int getReadReqPort() const
	{
		return m_readReqPort;
	}

	int getReadReqSocket() const
	{
		return m_readReqSocket;
	}

	int getReadWorkPort() const
	{
		return m_readWorkPort;
	}

	int getReadWorkSocket() const
	{
		return m_readWorkSocket;
	}

	int getWritePort() const
	{
		return m_writePort;
	}

	int getWriteSocket() const
	{
		return m_writeSocket;
	}

	struct sockaddr_in getReadReqAddr() const
	{
		return m_readReqAddr;
	}

	struct sockaddr_in getReadWorkAddr() const
	{
		return m_readWorkAddr;
	}

	struct sockaddr_in getWriteAddr() const
	{
		return m_writeAddr;
	}

	sockaddr_in* getCliAddr(uint32_t connid) 
	{
		if(m_cliAddresses.empty())
		{
			return NULL;
		}
		sockaddr_in* result = m_cliAddresses[connid].addr;
		return result;
	}

	pthread_t getReadReqThread() const
	{
		return m_readReqThread;
	}

	pthread_t getReadWorkThread() const
	{
		return m_readWorkThread;
	}

	pthread_t getWriteThread() const
	{
		return m_writeThread;
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

	uint32_t nextReqDis()
	{
		if(m_reqDis.empty())
		{
			return -1;
		}
		uint32_t result = m_reqDis.front();
		m_reqDis.pop();
		return result;
	}

	uint32_t nextWorkDis()
	{
		if(m_workDis.empty())
		{
			return -1;
		}
		uint32_t result = m_workDis.front();
		m_workDis.pop();
		return result;
	}

	uint32_t nextReqId()
	{
		uint32_t result = m_nextReqId;
		m_nextReqId += 2;
		return result;
	}

	uint32_t nextWorkId()
	{
		uint32_t result = m_nextWorkId;
		m_nextWorkId += 2;
		return result;
	}

	/* map remove */
	void removeCliAddr(uint32_t connid)
	{
		std::map<uint32_t, clientConnection>::iterator it = m_cliAddresses.find(connid);
		m_cliAddresses.erase(it);
	}


	/* other functions */
	bool hasReqDisconnect()
	{
		return !m_reqDis.empty();
	}

	bool hasWorkDisconnect()
	{
		return !m_workDis.empty();
	}

	void checkMessageAck(uint32_t connid, uint32_t seqnum)
	{
		pthread_mutex_lock(&m_waitingMessageLock);
		/* if message Waiting is null then it isn't an acknowledgement */
		if(m_messageWaiting == NULL)
		{
			printf("Reached inner\n");
			pthread_mutex_unlock(&m_waitingMessageLock);
			return;
		}
		printf("Message Waiting id: %d\n",m_messageWaiting->m_connid);
		m_messageAcknowledged = (m_messageWaiting->m_connid == connid && m_messageWaiting->m_seqnum == seqnum);
		pthread_mutex_unlock(&m_waitingMessageLock);
		printf("Reached 2\n");
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
};



