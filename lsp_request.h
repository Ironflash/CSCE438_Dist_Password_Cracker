#include <queue>
#include <pthread.h>

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
	pthread_mutex_t m_waitingMessageLock;
	pthread_mutex_t m_outboxLock;
	lsp_message* m_messageWaiting;	// message waiting for an acknowledgment
	bool m_messageAcknowledged;		// has last message been acknowledged
	bool m_isMessageWaiting;
	uint32_t m_nextSeqnum;
	uint32_t m_lastServerSeqnum;	// the last sequence number recieved from the server
public:

	lsp_request()
	{
		m_connid = 0;
		m_nextSeqnum = 0;
		m_lastServerSeqnum = 0;
		m_messageAcknowledged = true;
		m_isMessageWaiting = false;
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
			printf("socket creation error\n");
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

	void toInbox(lsp_message* message)
	{
		printf("Attempting to add to inbox\n");
		m_inbox.push(message);
		printf("Added to inbox\n");
	}

	void toOutbox(lsp_message* message)
	{
		// pthread_mutex_lock(&m_outboxLock);
		m_outbox.push(message);
		// pthread_mutex_unlock(&m_outboxLock);
	}

	void toWaitbox(lsp_message* message)
	{
		printf("Attempting to add to inbox\n");
		m_waitbox.push(message);
	}

	void setMessageWaiting(lsp_message* message)
	{
		pthread_mutex_lock(&m_waitingMessageLock);
		m_messageWaiting = message;
		m_isMessageWaiting = true;
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

	uint32_t getLastSeqnum() const
	{
		return m_lastServerSeqnum;
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
};