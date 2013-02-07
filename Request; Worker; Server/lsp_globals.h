//Globals

double m_epoch = 2;					// the number of seconds between epochs
int m_dropThreshhold = 5;			// number of no responses before the connection is dropped
double m_dropRate = 1;				// rate at which packets will get dropped for testing

// Set length of epoch (in seconds)
void lsp_set_epoch_lth(double lth)
{
	m_epoch = lth;
}

// Set number of epochs before timing out
void lsp_set_epoch_cnt(int cnt)
{
	m_dropThreshhold = cnt;
}

// Set fraction of packets that get dropped along each connection
void lsp_set_drop_rate(double rate)
{
	m_dropRate = rate;
}