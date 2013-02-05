#include <queue>
#include <map>
#include <vector>
#include <pthread.h>

/* Used to group together addr and socket of clients */
struct clientConnection
{
	sockaddr_in* addr;
	uint32_t seqnum;
	int numNoResponses;
	int numNoKeepAlive;
	bool dataSent;
	bool keepAlive;

	clientConnection()
	{
		numNoResponses = 0;
		numNoKeepAlive = 0;
		seqnum = 0;
		dataSent = false;
		keepAlive = false;
	}
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
	std::map<uint32_t, clientConnection> m_cliAddresses;	//keeps track of id to client addr
	std::vector<uint32_t> m_requestIds;		// list of request ids
	std::vector<uint32_t> m_workerIds;		// list of worker ids
	std::vector<uint32_t> m_requestDisconnects;		// list of the ids of requests that have been dropped
	std::vector<uint32_t> m_workerDisconnects;		// list of the ids of workers that have been dropped
	std::vector<uint32_t> m_awaitingMessages;		// list of clients that have connected and not sent a data message
	pthread_t 	m_readReqThread;
	pthread_t 	m_readWorkThread;
	pthread_t   m_writeThread;
	pthread_t   m_epochThread;
	uint32_t m_nextSeqnum;
	uint32_t m_nextReqId;		//next connection id to be assigned to request
	uint32_t m_nextWorkId;		//next connection id to be assigned to worker
	// pthread_mutex_t m_inboxLock;
	// pthread_mutex_t m_outboxLock;
	pthread_mutex_t m_waitingMessageLock;
	lsp_message* m_messageWaiting;	// message waiting for an acknowledgment
	lsp_message* m_mostRecentMessage;
	bool m_messageAcknowledged;		// has last message been acknowledged
	bool m_endThreads;
	// bool m_isMessageWaiting;		// keeps track if there is a server message that is awaiting approval
	int m_epoch;					// the number of seconds between epochs
	int m_dropThreshhold;			// number of no repsonses before the connection is dropped
	int m_KAThreshhold;				// number of missed keep alives before the connection is dropped
public:

	lsp_server()
	{
		m_nextSeqnum = 1;
		m_nextReqId = 1;	//Request ids will be odd
		m_nextWorkId = 2;	//Worker ids will be even
		m_messageAcknowledged = true;
		m_endThreads = false;
		// m_isMessageWaiting = false;
		m_mostRecentMessage = NULL;
		m_epoch = 2;		// epoch defaults to intervals of 2 seconds
		m_dropThreshhold = 5;		// default is 5 times
		m_KAThreshhold = 5;			// default is 5 times
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

	void setEpoch(int seconds)
	{
		m_epoch = seconds;
	}

	void setDropThreshhold(int num)
	{
		m_dropThreshhold = num;
	}

	void toCliAddr(uint32_t connid, sockaddr_in* addr)
	{
		m_cliAddresses[connid].addr = addr;
	}

	void toInbox(lsp_message* message)
	{
		// printf("Attempting to add to inbox\n");
		// pthread_mutex_lock(&m_inboxLock);
		m_inbox.push(message);
		// pthread_mutex_unlock(&m_inboxLock);
		// printf("Added to inbox\n");
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
		// m_isMessageWaiting = true;
		m_messageAcknowledged = false;
		pthread_mutex_unlock(&m_waitingMessageLock);
	}

	void setMostRecentMessage(lsp_message* message)
	{
		m_mostRecentMessage = new lsp_message(*message);
	}

	void addRequest(uint32_t connid)
	{
		m_requestIds.push_back(connid);
	}

	void addWorker(uint32_t connid)
	{
		m_workerIds.push_back(connid);
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
		printf("num cli: %d\n",m_cliAddresses.size());
		if(m_cliAddresses.empty())
		{
			printf("cliAddresses empty\n");
			return NULL;
		}
		else
		{
			printf("cliAddresses not empty\n");
		}
		// client no longer exists
		printf("num of clients with address %d: %d",connid,m_cliAddresses.count(connid));
		if(m_cliAddresses.count(connid) == 0)
		{
			return NULL;
		}
		sockaddr_in* result = m_cliAddresses[connid].addr;
		return result;
	}

	uint32_t getCliSeqnum(uint32_t connid)
	{
		if(m_cliAddresses.empty())
		{
			return -1;
		}
		if(m_cliAddresses.count(connid) == 0)
		{
			return -1;
		}
		uint32_t result = m_cliAddresses[connid].seqnum;
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

	pthread_t getEpochThread() const
	{
		return m_epochThread;
	}

	std::vector<uint32_t> getRequests()
	{
		return m_requestIds;
	}

	std::vector<uint32_t> getWorkers()
	{
		return m_workerIds;
	}

	lsp_message* getMessageWaiting()
	{
		return m_messageWaiting;
	}

	lsp_message* getMostRecentMessage() const
	{
		return m_mostRecentMessage;
	}

	int getEpoch() const
	{
		return m_epoch;
	}	
	// return list of request ids that have disconnected since the last time checked
	std::vector<uint32_t> getRequestDis()
	{
		std::vector<uint32_t> result = m_requestDisconnects;
		m_requestDisconnects.clear();
		return result;
	}

	//return list of worker ids that have disconnected since the last time checked
	std::vector<uint32_t> getWorkerDis()
	{
		std::vector<uint32_t> result = m_workerDisconnects;
		m_workerDisconnects.clear();
		return result;
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
		if(m_cliAddresses.count(connid) > 0)
		{
			printf("removeing addr\n");
			std::map<uint32_t, clientConnection>::iterator it = m_cliAddresses.find(connid);
			m_cliAddresses.erase(it);
		}
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
			pthread_mutex_unlock(&m_waitingMessageLock);
			return;
		}
		// printf("Message Waiting id: %d\n",m_messageWaiting->m_connid);
		m_messageAcknowledged = (m_messageWaiting->m_connid == connid && m_messageWaiting->m_seqnum == seqnum);
		pthread_mutex_unlock(&m_waitingMessageLock);
		// printf("Reached 2\n");
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

	// determines if a request with the given id has been disconnected
	bool requestDis(uint32_t connid)
	{
		for(int i = 0; i < m_requestDisconnects.size(); i++)
		{
			if(m_requestDisconnects[i] == connid)
			{
				return true;
			}
		}
		return false;
	}

	// determines if a worker with the given id has been disconnected
	bool workerDis(uint32_t connid)
	{
		for(int i = 0; i < m_workerDisconnects.size(); i++)
		{
			if(m_workerDisconnects[i] == connid)
			{
				return true;
			}
		}
		return false;
	}

	void updateClientSeqnum(uint32_t connid,uint32_t seqnum)
	{
		m_cliAddresses[connid].seqnum = seqnum;
		
	}

	void noResponse(uint32_t connid)
	{
		m_cliAddresses[connid].numNoResponses++;
	}

	bool clientAboveThreshhold(uint32_t connid)
	{
		if(m_cliAddresses.count(connid) == 0)
		{
			return false;
		}
		return m_cliAddresses[connid].numNoResponses > m_dropThreshhold;
	}

	void dropClient(uint32_t connid)
	{
		/* remove the cli from the list of clients */
		removeCliAddr(connid);
		//if the connection id is even and therefore a worker
		if(connid % 2 == 0)
		{
			printf("dropping worker\n");
			//add to list of disconnected workers
			m_workerDisconnects.push_back(connid);
			//put id up for re-assignment
			m_workDis.push(connid);
			//remove from list of current workers
			if(m_workerIds.count(connid) > 0)
			{
				std::vector<uint32_t>::iterator it;
				it = std::find(m_workerIds.begin(), m_workerIds.end(), connid);
				m_workerIds.erase(it);
			}
		}
		else	// the connection id is for a request
		{
			printf("dropping request\n");
			//add to list of disconnected workers
			m_requestDisconnects.push_back(connid);
			//put id up for re-assignment
			m_reqDis.push(connid);
			//remove from list of current workers
			if(m_requestIds.count(connid) > 0)
			{
				std::vector<uint32_t>::iterator it;
				it = std::find(m_requestIds.begin(), m_requestIds.end(), connid);
				m_requestIds.erase(it);
			}
		}
		
		//required so that the server will go on and not wait for a reply
		m_messageAcknowledged = true;
	}

	bool dataSentTo(uint32_t connid)
	{
		if(m_cliAddresses.count(connid) > 0)
		{
			return m_cliAddresses[connid].dataSent;
		}
	}

	void dataWasSentTo(uint32_t connid)
	{
		m_cliAddresses[connid].dataSent = true;
	}

	bool keepAliveReceived(uint32_t connid)
	{
		if(m_cliAddresses.count(connid) == 0)
		{
			return false;
		}
		bool result = m_cliAddresses[connid].keepAlive;
		//reset for next keep alive message
		m_cliAddresses[connid].keepAlive = false;
		return result;
	}

	void receivedKeepAlive(uint32_t connid)
	{
		//reset num no keep alive 
		printf("num keep alive reset\n");
		m_cliAddresses[connid].numNoKeepAlive = 0;
		m_cliAddresses[connid].keepAlive = true;
	}

	void incNoKeepAlive(uint32_t connid)
	{
		if(m_cliAddresses.count(connid) > 0)
		{
			m_cliAddresses[connid].numNoKeepAlive++;
		}
	}

	bool clientAboveKAThreshhold(uint32_t connid)
	{
		if(m_cliAddresses.count(connid) == 0)
		{
			return false;
		}
		return m_cliAddresses[connid].numNoKeepAlive >  m_KAThreshhold;
	}

	void endThreads()
	{
		m_endThreads = true;
	}

	bool shouldEndThreads()
	{
		return m_endThreads;
	}

	/* awaiting message functions */
	void awaitingMessage(uint32_t connid)
	{
		m_awaitingMessages.push_back(connid);
	}

	void removeAwaitingMessage(uint32_t connid)
	{
		if(m_awaitingMessages.count(connid) > 0)
		{
			std::vector<uint32_t>::iterator it;
			it = std::find(m_awaitingMessages.begin(), m_awaitingMessages.end(), connid);
			m_awaitingMessages.erase(it);
		}
	}

	std::vector<uint32_t> getAwaitingMessages() const
	{
		return m_awaitingMessages;
	}


};



