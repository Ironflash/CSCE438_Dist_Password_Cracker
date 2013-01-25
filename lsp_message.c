struct lsp_message
{
	uint32_t connid;
	uint32_t sequm;
	uint8_t payload[];
};