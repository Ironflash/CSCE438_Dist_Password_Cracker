#include <string>

struct lsp_message
{
	uint32_t m_connid;
	uint32_t m_sequm;
	std::string m_payload;
	int m_bytesRead;

	lsp_message(uint32_t connid, uint32_t sequm, std::string payload, int bytes)
	{
		m_connid = connid;
		m_sequm = sequm;
		m_payload = payload;
		m_bytesRead = bytes;
	}

	~lsp_message(){}
};