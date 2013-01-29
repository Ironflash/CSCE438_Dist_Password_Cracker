class lsp_server
{
private:
	int m_port;		//port that will be used for listening 
	int m_socket;		//socket that will listen for incoming connections from requests and workers
	struct sockaddr_in m_servaddr, m_cliaddr;	//address of the server and client
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

	/* getters */

	int getPort() const
	{
		return m_port;
	}

	int getSocket() const
	{
		return m_socket;
	}

	struct sockaddr_in getServAddr()
	{
		return m_servaddr;
	}

	struct sockaddr_in getCliAddr()
	{
		return m_cliaddr;
	}
};