#include <queue>
#include <map>
#include <vector>
#include <pthread.h>
#include "lsp_globals.h"

//#define DEBUG // uncomment to turn on print outs
#ifdef DEBUG
#define DEBUG_MSG(str) do { std::cout << str << std::endl; } while( false )
#else
#define DEBUG_MSG(str) do { } while ( false )
#endif

/* Used to group together addr and socket of clients */
struct clientConnection
{
	sockaddr_in* addr;
	uint32_t seqnum;
	uint32_t nextServSeqnum;
	int numNoResponses;
	int numNoKeepAlive;
	bool dataSent;
	bool keepAlive;

	clientConnection()
	{
		numNoResponses = 0;
		numNoKeepAlive = 0;
		seqnum = 0;
		nextServSeqnum = 1;
		dataSent = false;
		keepAlive = false;
	}
};

class lsp_server
{
private:
	int m_reqPort;		//port that will be used for listening 
	int m_reqSocket;		//socket that will listen for incoming messages from requests
	int m_workPort;		//port that will be used for listening
	int m_workSocket;	//socket that will listen for incoming messages from workers
	// int m_writePort;		//port that will be used for writing messages
	// int m_writeSocket;	//socket that will write outgoing messages 
	sockaddr_in m_reqAddr;	//address of the server for reading requests 
	sockaddr_in m_workAddr;	//address of the server for reading workers
	// sockaddr_in m_writeAddr;	//address of the server for writing
	std::queue<lsp_message*> m_reqInbox;
	std::queue<lsp_message*> m_reqOutbox;
	std::queue<lsp_message*> m_workInbox;
	std::queue<lsp_message*> m_workOutbox;
	std::queue<lsp_message*> m_reqAckbox;
	std::queue<lsp_message*> m_workAckbox;
	std::queue<uint32_t> m_reqDis;		//store connection ids of disconnected requests
	std::queue<uint32_t> m_workDis;		//store connection ids of disconnected workers
	std::map<uint32_t, clientConnection> m_cliConnections;	//keeps track of id to client connections
	std::vector<uint32_t> m_requestIds;		// list of request ids
	std::vector<uint32_t> m_workerIds;		// list of worker ids
	std::vector<uint32_t> m_requestDisconnects;		// list of the ids of requests that have been dropped
	std::vector<uint32_t> m_workerDisconnects;		// list of the ids of workers that have been dropped
	std::vector<uint32_t> m_awaitingReqMessages;		// list of clients that have connected and not sent a data message
	std::vector<uint32_t> m_awaitingWorkMessages;		// list of clients that have connected and not sent a data message
	pthread_t 	m_readReqThread;
	pthread_t 	m_readWorkThread;
	pthread_t   m_writeReqThread;
	pthread_t   m_writeWorkThread;
	pthread_t   m_epochThread;
	uint32_t m_nextSeqnum;
	uint32_t m_nextReqId;		//next connection id to be assigned to request
	uint32_t m_nextWorkId;		//next connection id to be assigned to worker
	// pthread_mutex_t m_inboxLock;
	// pthread_mutex_t m_outboxLock;
	pthread_mutex_t m_waitingReqMessageLock;
	pthread_mutex_t m_waitingWorkMessageLock;
	pthread_mutex_t m_cliConnectionsLock;
	pthread_mutex_t m_reqRWLock;
	pthread_mutex_t m_workRWLock;
	lsp_message* m_reqMessageWaiting;	// message waiting for an acknowledgment
	lsp_message* m_workMessageWaiting;	// message waiting for an acknowledgment
	lsp_message* m_mostRecentReqMessage;
	lsp_message* m_mostRecentWorkMessage;
	bool m_reqMessageAcknowledged;		// has last message been acknowledged
	bool m_workMessageAcknowledged;		// has last message been acknowledged
	bool m_endThreads;
	// bool m_isMessageWaiting;		// keeps track if there is a server message that is awaiting approval
	// int m_epoch;					// the number of seconds between epochs
	// int m_dropThreshhold;			// number of no repsonses before the connection is dropped
	int m_KAThreshhold;				// number of missed keep alives before the connection is dropped
public:

	lsp_server()
	{
		m_nextSeqnum = 1;
		m_nextReqId = 1;	//Request ids will be odd
		m_nextWorkId = 2;	//Worker ids will be even
		m_reqMessageAcknowledged = true;
		m_workMessageAcknowledged = true;
		m_endThreads = false;
		// m_isMessageWaiting = false;
		m_mostRecentReqMessage = NULL;
		m_mostRecentWorkMessage = NULL;
		// m_epoch = 2;		// epoch defaults to intervals of 2 seconds
		// m_dropThreshhold = 5;		// default is 5 times
		m_KAThreshhold = 5;			// default is 5 times
		// pthread_mutex_init(&m_inboxLock, NULL);
		// pthread_mutex_init(&m_outboxLock, NULL);
		pthread_mutex_init(&m_waitingReqMessageLock, NULL);
		pthread_mutex_init(&m_waitingWorkMessageLock, NULL);
		pthread_mutex_init(&m_cliConnectionsLock, NULL);
		pthread_mutex_init(&m_reqRWLock, NULL);
		pthread_mutex_init(&m_workRWLock, NULL);
	}
	~lsp_server()
	{
		//TODO go through all client addresses and delete address pointers maybe free because used malloc
		// pthread_mutex_destroy(&m_inboxLock);
		// pthread_mutex_destroy(&m_outboxLock);
		pthread_mutex_destroy(&m_waitingReqMessageLock);
		pthread_mutex_destroy(&m_waitingWorkMessageLock);
		pthread_mutex_destroy(&m_cliConnectionsLock);
		pthread_mutex_destroy(&m_reqRWLock);
		pthread_mutex_destroy(&m_workRWLock);
	}
	/* setters */
	void setReqPort(int port)
	{
		m_reqPort = port;
	}

	int setReqSocket(int socket)
	{
		m_reqSocket = socket;
		return m_reqSocket;
	}

	void setReqAddr(struct sockaddr_in servaddr)
	{
		m_reqAddr = servaddr;
	}  

	void setWorkPort(int port)
	{
		m_workPort = port;
	}

	int setWorkSocket(int socket)
	{
		m_workSocket = socket;
		return m_workSocket;
	}

	void setWorkAddr(struct sockaddr_in servaddr)
	{
		m_workAddr = servaddr;
	}  

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

	// void setEpoch(int seconds)
	// {
	// 	m_epoch = seconds;
	// }

	// void setDropThreshhold(int num)
	// {
	// 	m_dropThreshhold = num;
	// }

	void toCliAddr(uint32_t connid, sockaddr_in* addr)
	{
		pthread_mutex_lock(&m_cliConnectionsLock);
		m_cliConnections[connid].addr = addr;
		pthread_mutex_unlock(&m_cliConnectionsLock);
	}

	void toReqInbox(lsp_message* message)
	{
		m_reqInbox.push(message);
	}

	void toReqOutbox(lsp_message* message)
	{
		// pthread_mutex_lock(&m_outboxLock);
		DEBUG_MSG("to outbox connid: "<<message->m_connid);
		m_reqOutbox.push(message);
		DEBUG_MSG("Req Outbox size after: "<<(int)m_reqOutbox.size());
		// pthread_mutex_unlock(&m_outboxLock);
	}

	void toWorkInbox(lsp_message* message)
	{
		m_workInbox.push(message);
	}

	void toWorkOutbox(lsp_message* message)
	{
		// pthread_mutex_lock(&m_outboxLock);
		m_workOutbox.push(message);
		// pthread_mutex_unlock(&m_outboxLock);
	}

	void toReqAckbox(lsp_message* message)
	{
		m_reqAckbox.push(message);
	}

	void toWorkAckbox(lsp_message* message)
	{
		// pthread_mutex_lock(&m_outboxLock);
		DEBUG_MSG("Req Outbox size before: "<<m_reqOutbox.size());
		m_workAckbox.push(message);
		// pthread_mutex_unlock(&m_outboxLock);
	}

	void setReqMessageWaiting(lsp_message* message)
	{
		pthread_mutex_lock(&m_waitingReqMessageLock);
		m_reqMessageWaiting = message;
		// m_isMessageWaiting = true;
		m_reqMessageAcknowledged = false;
		pthread_mutex_unlock(&m_waitingReqMessageLock);
	}

	void setWorkMessageWaiting(lsp_message* message)
	{
		pthread_mutex_lock(&m_waitingWorkMessageLock);
		m_workMessageWaiting = message;
		// m_isMessageWaiting = true;
		m_workMessageAcknowledged = false;
		pthread_mutex_unlock(&m_waitingWorkMessageLock);
	}

	void setMostRecentReqMessage(lsp_message* message)
	{
		m_mostRecentReqMessage = new lsp_message(*message);
	}

	void setMostRecentWorkMessage(lsp_message* message)
	{
		m_mostRecentWorkMessage = new lsp_message(*message);
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

	int getReqPort() const
	{
		return m_reqPort;
	}

	int getReqSocket() const
	{
		return m_reqSocket;
	}

	int getWorkPort() const
	{
		return m_workPort;
	}

	int getWorkSocket() const
	{
		return m_workSocket;
	}

	pthread_mutex_t* getReqRWLock()
	{
		return &m_reqRWLock;
	}

	pthread_mutex_t* getWorkRWLock()
	{
		return &m_workRWLock;
	}  

	// int getWritePort() const
	// {
	// 	return m_writePort;
	// }

	// int getWriteSocket() const
	// {
	// 	return m_writeSocket;
	// }

	struct sockaddr_in getReqAddr() const
	{
		return m_reqAddr;
	}

	struct sockaddr_in getWorkAddr() const
	{
		return m_workAddr;
	}

	// struct sockaddr_in getWriteAddr() const
	// {
	// 	return m_writeAddr;
	// }

	sockaddr_in* getCliAddr(uint32_t connid) 
	{
		pthread_mutex_lock(&m_cliConnectionsLock);
		DEBUG_MSG("num cli: "<<(int)m_cliConnections.size());
		if(m_cliConnections.empty())
		{
			DEBUG_MSG("cliAddresses empty");
			pthread_mutex_unlock(&m_cliConnectionsLock);
			return NULL;
		}
		else
		{
			DEBUG_MSG("cliAddresses not empty");
		}
		// client no longer exists
		DEBUG_MSG("num of clients with address "<<connid": "<<(int)m_cliConnections.count(connid));
		if(m_cliConnections.count(connid) == 0)
		{
			pthread_mutex_unlock(&m_cliConnectionsLock);
			return NULL;
		}
		sockaddr_in* result = m_cliConnections[connid].addr;
		pthread_mutex_unlock(&m_cliConnectionsLock);
		return result;
	}

	uint32_t getCliSeqnum(uint32_t connid)
	{
		pthread_mutex_lock(&m_cliConnectionsLock);
		if(m_cliConnections.empty())
		{
			pthread_mutex_unlock(&m_cliConnectionsLock);
			return -1;
		}
		if(m_cliConnections.count(connid) == 0)
		{
			pthread_mutex_unlock(&m_cliConnectionsLock);
			return -1;
		}
		uint32_t result = m_cliConnections[connid].seqnum;
		pthread_mutex_unlock(&m_cliConnectionsLock);
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

	pthread_t getWriteReqThread() const
	{
		return m_writeReqThread;
	}

	pthread_t getWriteWorkThread() const
	{
		return m_writeWorkThread;
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

	lsp_message* getReqMessageWaiting()
	{
		return m_reqMessageWaiting;
	}

	lsp_message* getWorkMessageWaiting()
	{
		return m_workMessageWaiting;
	}

	lsp_message* getMostRecentReqMessage() const
	{
		return m_mostRecentReqMessage;
	}

	lsp_message* getMostRecentWorkMessage() const
	{
		return m_mostRecentWorkMessage;
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

	lsp_message* fromReqInbox()
	{
		// pthread_mutex_lock(&m_inboxLock);
		if(m_reqInbox.empty())
		{
			return NULL;
		}
		lsp_message* result = m_reqInbox.front();
		m_reqInbox.pop();
		// pthread_mutex_unlock(&m_inboxLock);
		return result;
	}

	lsp_message* fromWorkInbox()
	{
		// pthread_mutex_lock(&m_inboxLock);
		if(m_workInbox.empty())
		{
			return NULL;
		}
		lsp_message* result = m_workInbox.front();
		m_workInbox.pop();
		// pthread_mutex_unlock(&m_inboxLock);
		return result;
	}

	lsp_message* fromReqOutbox()
	{
		// pthread_mutex_lock(&m_outboxLock);
		if(m_reqOutbox.empty())
		{
			return NULL;
		}
		lsp_message* result = m_reqOutbox.front();
		m_reqOutbox.pop();
		// pthread_mutex_unlock(&m_outboxLock);
		return result;
	}

	lsp_message* fromWorkOutbox()
	{
		// pthread_mutex_lock(&m_outboxLock);
		if(m_workOutbox.empty())
		{
			return NULL;
		}
		lsp_message* result = m_workOutbox.front();
		m_workOutbox.pop();
		// pthread_mutex_unlock(&m_outboxLock);
		return result;
	}

	lsp_message* fromReqAckbox()
	{
		// pthread_mutex_lock(&m_outboxLock);
		if(m_reqAckbox.empty())
		{
			return NULL;
		}
		lsp_message* result = m_reqAckbox.front();
		m_reqAckbox.pop();
		// pthread_mutex_unlock(&m_outboxLock);
		return result;
	}

	lsp_message* fromWorkAckbox()
	{
		// pthread_mutex_lock(&m_outboxLock);
		if(m_workAckbox.empty())
		{
			return NULL;
		}
		lsp_message* result = m_workAckbox.front();
		m_workAckbox.pop();
		// pthread_mutex_unlock(&m_outboxLock);
		return result;
	}

	uint32_t nextSeq(uint32_t connid)
	{
		// return m_nextSeqnum++;
		pthread_mutex_lock(&m_cliConnectionsLock);
		if(m_cliConnections.count(connid) == 0)
		{
			pthread_mutex_unlock(&m_cliConnectionsLock);
			return -1;
		}
		uint32_t result = m_cliConnections[connid].nextServSeqnum++;
		pthread_mutex_unlock(&m_cliConnectionsLock);
		return result;
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
		pthread_mutex_lock(&m_cliConnectionsLock);
		if(m_cliConnections.count(connid) > 0)
		{
			DEBUG_MSG("removing addr");
			std::map<uint32_t, clientConnection>::iterator it = m_cliConnections.find(connid);
			m_cliConnections.erase(it);
		}
		pthread_mutex_unlock(&m_cliConnectionsLock);
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

	void checkReqMessageAck(uint32_t connid, uint32_t seqnum)
	{
		pthread_mutex_lock(&m_waitingReqMessageLock);
		/* if message Waiting is null then it isn't an acknowledgement */
		if(m_reqMessageWaiting == NULL)
		{
			pthread_mutex_unlock(&m_waitingReqMessageLock);
			return;
		}
		DEBUG_MSG("Message Waiting id: "<<m_reqMessageWaiting->m_connid);
		DEBUG_MSG("Connid: "<<connid);
		m_reqMessageAcknowledged = (m_reqMessageWaiting->m_connid == connid && m_reqMessageWaiting->m_seqnum == seqnum);
		pthread_mutex_unlock(&m_waitingReqMessageLock);
		return;
	}

	void checkWorkMessageAck(uint32_t connid, uint32_t seqnum)
	{
		pthread_mutex_lock(&m_waitingWorkMessageLock);
		/* if message Waiting is null then it isn't an acknowledgement */
		if(m_workMessageWaiting == NULL)
		{
			pthread_mutex_unlock(&m_waitingWorkMessageLock);
			return;
		}
		DEBUG_MSG("Message Waiting id: "<<m_workMessageWaiting->m_connid);
		DEBUG_MSG("Connid: "<<connid);
		m_workMessageAcknowledged = (m_workMessageWaiting->m_connid == connid && m_workMessageWaiting->m_seqnum == seqnum);
		pthread_mutex_unlock(&m_waitingWorkMessageLock);
		return;
	}

	bool reqMessageAcknowledged()
	{
		return m_reqMessageAcknowledged;
	}

	bool workMessageAcknowledged()
	{
		return m_workMessageAcknowledged;
	}
	bool awaitingReqAck()
	{
		return (m_reqMessageWaiting != NULL);
	}

	bool awaitingWorkAck()
	{
		return (m_workMessageWaiting != NULL);
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
		pthread_mutex_lock(&m_cliConnectionsLock);
		m_cliConnections[connid].seqnum = seqnum;
		pthread_mutex_unlock(&m_cliConnectionsLock);
		
	}

	void noResponse(uint32_t connid)
	{
		pthread_mutex_lock(&m_cliConnectionsLock);
		m_cliConnections[connid].numNoResponses++;
		pthread_mutex_unlock(&m_cliConnectionsLock);
	}

	bool clientAboveThreshhold(uint32_t connid)
	{
		pthread_mutex_lock(&m_cliConnectionsLock);
		if(m_cliConnections.count(connid) == 0)
		{
			pthread_mutex_unlock(&m_cliConnectionsLock);
			return false;
		}
		bool result = m_cliConnections[connid].numNoResponses > m_dropThreshhold;
		pthread_mutex_unlock(&m_cliConnectionsLock);
		return result;
	}

	void dropClient(uint32_t connid)
	{
		/* remove the cli from the list of clients */
		removeCliAddr(connid);
		//if the connection id is even and therefore a worker
		if(connid % 2 == 0)
		{
			DEBUG_MSG("dropping worker");
			//add to list of disconnected workers
			m_workerDisconnects.push_back(connid);
			//put id up for re-assignment
			m_workDis.push(connid);
			//remove from list of current workers
			if(std::count(m_workerIds.begin(), m_workerIds.end(),connid) > 0)
			{
				std::vector<uint32_t>::iterator it;
				it = std::find(m_workerIds.begin(), m_workerIds.end(), connid);
				m_workerIds.erase(it);
			}
			//required so that the server will go on and not wait for a reply
			m_reqMessageAcknowledged = true;
		}
		else	// the connection id is for a request
		{
			DEBUG_MSG("dropping request");
			//add to list of disconnected workers
			m_requestDisconnects.push_back(connid);
			//put id up for re-assignment
			m_reqDis.push(connid);
			//remove from list of current workers
			if(std::count(m_requestIds.begin(), m_requestIds.end(),connid) > 0)
			{
				std::vector<uint32_t>::iterator it;
				it = std::find(m_requestIds.begin(), m_requestIds.end(), connid);
				m_requestIds.erase(it);
			}
			//required so that the server will go on and not wait for a reply
			m_workMessageAcknowledged = true;
		}
		
	}

	bool dataSentTo(uint32_t connid)
	{
		pthread_mutex_lock(&m_cliConnectionsLock);
		if(m_cliConnections.count(connid) > 0)
		{
			bool result = m_cliConnections[connid].dataSent;
			pthread_mutex_unlock(&m_cliConnectionsLock);
			return result;
		}
		pthread_mutex_unlock(&m_cliConnectionsLock);
		return false;
	}

	void dataWasSentTo(uint32_t connid)
	{
		pthread_mutex_lock(&m_cliConnectionsLock);
		m_cliConnections[connid].dataSent = true;
		pthread_mutex_unlock(&m_cliConnectionsLock);
	}

	bool keepAliveReceived(uint32_t connid)
	{
		pthread_mutex_lock(&m_cliConnectionsLock);
		if(m_cliConnections.count(connid) == 0)
		{
			pthread_mutex_unlock(&m_cliConnectionsLock);
			return false;
		}
		bool result = m_cliConnections[connid].keepAlive;
		//reset for next keep alive message
		m_cliConnections[connid].keepAlive = false;
		pthread_mutex_unlock(&m_cliConnectionsLock);
		return result;
	}

	void receivedKeepAlive(uint32_t connid)
	{
		//reset num no keep alive 
		pthread_mutex_lock(&m_cliConnectionsLock);
		DEBUG_MSG("num keep alive reset");
		m_cliConnections[connid].numNoKeepAlive = 0;
		m_cliConnections[connid].keepAlive = true;
		pthread_mutex_unlock(&m_cliConnectionsLock);
	}

	void incNoKeepAlive(uint32_t connid)
	{
		pthread_mutex_lock(&m_cliConnectionsLock);
		if(m_cliConnections.count(connid) > 0)
		{
			m_cliConnections[connid].numNoKeepAlive++;
		}
		pthread_mutex_unlock(&m_cliConnectionsLock);
	}

	bool clientAboveKAThreshhold(uint32_t connid)
	{
		pthread_mutex_lock(&m_cliConnectionsLock);
		if(m_cliConnections.count(connid) == 0)
		{
			pthread_mutex_unlock(&m_cliConnectionsLock);
			return false;
		}
		bool result = m_cliConnections[connid].numNoKeepAlive >  m_KAThreshhold;
		pthread_mutex_unlock(&m_cliConnectionsLock);
		return result;
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
	void awaitingReqMessage(uint32_t connid)
	{
		m_awaitingReqMessages.push_back(connid);
	}

	void awaitingWorkMessage(uint32_t connid)
	{
		m_awaitingWorkMessages.push_back(connid);
	}

	void removeAwaitingReqMessage(uint32_t connid)
	{
		if(std::count(m_awaitingReqMessages.begin(), m_awaitingReqMessages.end(),connid) > 0)
		{
			std::vector<uint32_t>::iterator it;
			it = std::find(m_awaitingReqMessages.begin(), m_awaitingReqMessages.end(), connid);
			m_awaitingReqMessages.erase(it);
		}
	}

	void removeAwaitingWorkMessage(uint32_t connid)
	{
		if(std::count(m_awaitingWorkMessages.begin(), m_awaitingWorkMessages.end(),connid) > 0)
		{
			std::vector<uint32_t>::iterator it;
			it = std::find(m_awaitingWorkMessages.begin(), m_awaitingWorkMessages.end(), connid);
			m_awaitingWorkMessages.erase(it);
		}
	}

	std::vector<uint32_t> getAwaitingReqMessages() const
	{
		return m_awaitingReqMessages;
	}

	std::vector<uint32_t> getAwaitingWorkMessages() const
	{
		return m_awaitingWorkMessages;
	}


};



