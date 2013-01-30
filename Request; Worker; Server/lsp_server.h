#include <queue>
// #include "lsp_message.c"
#include <pthread.h>
class lsp_server
{
private:
	int m_port;		//port that will be used for listening 
	int m_socket;		//socket that will listen for incoming connections from requests and workers
	struct sockaddr_in m_servaddr, m_cliaddr;	//address of the server and client
	std::queue<lsp_message*> m_inbox;
	std::queue<lsp_message*> m_outbox;
	pthread_t 	m_readThread;
public:

	/* setters */
	void setPort(int port)
	{
		m_port = port;
	}

	int setSocket(int socket)
	{
		m_socket = socket;
		return m_socket;
	}

	void setServAddr(struct sockaddr_in servaddr)
	{
		m_servaddr = servaddr;
	}

	void setCliAddr(struct sockaddr_in cliaddr)
	{
		m_cliaddr = cliaddr;
	}

	void addInbox(lsp_message* message)
	{
		m_inbox.push(message);
	}

	/* getters */

	int getPort() const
	{
		return m_port;
	}

	int getSocket() const
	{
		return m_socket;
	}

	struct sockaddr_in getServAddr() const
	{
		return m_servaddr;
	}

	struct sockaddr_in getCliAddr()	const
	{
		return m_cliaddr;
	}

	pthread_t getReadThread() const
	{
		return m_readThread;
	}

	lsp_message* fromInbox()
	{
		if(m_inbox.empty())
		{
			return NULL;
		}
		lsp_message* result = m_inbox.front();
		m_inbox.pop();
		return result;
	}
};