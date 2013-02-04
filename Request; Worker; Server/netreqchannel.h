/*
    File: netreqchannel.H

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 2012/07/30

*/

#ifndef _netreqchannel_H_                   // include file only once
#define _netreqchannel_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include <iostream>
#include <fstream>

#include <string>

using namespace std;

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CLASS  N e t w o r k R e q u e s t C h a n n e l */
/*--------------------------------------------------------------------------*/

class NetworkRequestChannel {

private:
  
  int fd; // socket file descriptor;

public:

  /* -- CONSTRUCTOR/DESTRUCTOR */

  NetworkRequestChannel(const string _server_host_name, const unsigned short _port_no);
  /* Creates a CLIENT-SIDE local copy of the channel. The channel is connected
     to the given port number at the given server host. 
     THIS CONSTRUCTOR IS CALLED BY THE CLIENT. */

  NetworkRequestChannel(const unsigned short _port_no, void * (*connection_handler) (void *), int _backlog);
  /* Creates a SERVER-SIDE local copy of the channel that is accepting connections
     at the given port number.
     NOTE that multiple clients can be connected to the same server-side end of the
     request channel. Whenever a new connection comes in, it is accepted by the 
     the server, and the given connection handler is invoked. The parameter to 
     the connection handler is the file descriptor of the slave socket returned
     by the accept call.
     NOTE that it is up to the connection handler to close the socket when the 
     client is done. */ 

  ~NetworkRequestChannel();
  /* Destructor of the local copy of the bus. By default, the Server Side deletes 
     any IPC mechanisms associated with the channel. */

  string send_request(string _request);
  /* Send a string over the channel and wait for a reply. */

  string cread();
  /* Blocking read of data from the channel. Returns a string of characters
     read from the channel. Returns NULL if read failed. */

  int cwrite(string _msg);
  /* Write the data to the channel. The function returns the number of characters written
     to the channel. */

};

/* LOCAL FUNCTIONS -- SIMULATE UDP-LSP */
  void lsp_request_create(const char* dest, int port);
  string lsp_request_read(uint8_t* pld);
  bool lsp_request_write(string pld, int lth);
  bool lsp_request_close();

  NetworkRequestChannel* lsp_server_create(int port, void * (*connection_handler) (void *));
  int lsp_server_read(void* pld, uint32_t* conn_id);
  bool lsp_server_write(void* pld, int lth, uint32_t conn_id);
  bool lsp_server_close(uint32_t conn_id);

#endif
