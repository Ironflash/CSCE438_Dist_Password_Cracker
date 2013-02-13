#include <string>

struct lsp_message
{
	uint32_t m_connid;
	uint32_t m_seqnum;
	std::string m_payload;
	int m_bytesRead;

	lsp_message(uint32_t connid, uint32_t seqnum, std::string payload, int bytes = 0)
	{
		m_connid = connid;
		m_seqnum = seqnum;
		m_payload = payload;
		m_bytesRead = bytes;
	}

	~lsp_message(){}
};
