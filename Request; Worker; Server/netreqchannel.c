/* 
    File: NetworkRequestChannel.C

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 2012/07/11

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <string>
#include <sstream>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "netreqchannel.h"

using namespace std;

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

typedef struct threadID_socket {
    int socket;
    pthread_t thread_id;
} THREADID_SOCKET;

/*--------------------------------------------------------------------------*/
/* VARIABLES */
/*--------------------------------------------------------------------------*/

struct sockaddr_in sin_server; // Internet endpoint address
bool exit_server = false;

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS -- SIMULATE UDP-LSP */
/*--------------------------------------------------------------------------*/

NetworkRequestChannel* request_channel;

void lsp_request_create(const char* dest, int port){
  unsigned short port_number = (unsigned short) port;
  string host_name = (string)dest;
  request_channel = new NetworkRequestChannel(host_name, port_number);
  //return request_channel;
}
string lsp_request_read(uint8_t* pld) {
  string reply = request_channel->cread();
  //*((string*)pld) = reply;
  return reply;
}
bool lsp_request_write(string pld, int lth) {
  cout<<"Sending Request = "<<pld<<endl;
  string reply = request_channel->send_request(pld);
  cout<<"Received reply = "<<reply<<endl;
  return true;
}
bool lsp_request_close() {
  (*request_channel).~NetworkRequestChannel();
  return true;
}
NetworkRequestChannel* lsp_server_create(int port, void * (*connection_handler) (void *)){
  unsigned short port_number = port;
  int backlog = 10;
  NetworkRequestChannel *server_channel = new NetworkRequestChannel(port_number, connection_handler, backlog);
  return server_channel;
}
int lsp_server_read(void* pld, uint32_t* conn_id){
  return 0;
}
bool lsp_server_write(void* pld, int lth, uint32_t conn_id){
  return true;
}
bool lsp_server_close(uint32_t conn_id){
  return true;
}

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS -- SUPPORT FUNCTIONS */
/*--------------------------------------------------------------------------*/

string int2string(int number) {
   stringstream ss;//create a stringstream
   ss << number;//add number to the stream
   return ss.str();//return a string with the contents of the stream
}

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS -- TCP Server Socket Creation*/
/*--------------------------------------------------------------------------*/

int server_TCP_socket(const char * _desired_service, int _backlog) {
  memset(&sin_server, 0, sizeof(sin_server)); // Zero out address
  sin_server.sin_family = AF_INET;
  sin_server.sin_addr.s_addr = INADDR_ANY;

  // Map service name to port number
  struct servent * pse;
  pse = getservbyname(_desired_service, "tcp");

  cout<<"server_TCP_socket"<<endl;
  _desired_service = "7000";
  cout<<"_desired_service = "<<_desired_service<<endl;

  
  if (pse) {
    sin_server.sin_port = pse->s_port;
  } else if ((sin_server.sin_port = htons((unsigned short)atoi(_desired_service))) == 0) {   
    cerr<<"Cannot get <"<<_desired_service<<"> service entry"<<endl;
    exit(EXIT_FAILURE);
  }

  // Allocate socket
  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    cerr<<"Cannot create socket: "<<strerror(errno)<<endl;
    exit(EXIT_FAILURE);
  }
  // Bind the socket 
  if (bind(s, (struct sockaddr *)&sin_server, sizeof(sin_server)) < 0) {
    cerr<<"Cannot bind to service, "<<_desired_service<<endl;
    exit(EXIT_FAILURE);
  }

  // Listen on socket
  if (listen(s, _backlog) < 0) {
    cerr<<"Cannot listen on service, "<<_desired_service<<endl;
    exit(EXIT_FAILURE);
  }

  return s;
}

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS -- TCP Client Socket Connect */
/*--------------------------------------------------------------------------*/

int client_TCP_connect(const char * _host, const char * _desired_service) {
  struct sockaddr_in sin_client; // Internet endpoint address
  memset(&sin_client, 0, sizeof(sin_client)); // Zero out address
  sin_client.sin_family = AF_INET;

  // Map service name to port number
  struct servent * pse;
  pse = getservbyname(_desired_service, "tcp");

  cout<<"client_TCP_socket"<<endl;
  _desired_service = "7000";
  cout<<"_desired_service = "<<_desired_service<<endl;
  
  if (pse) {
    sin_client.sin_port = pse->s_port;
  } else if ((sin_client.sin_port = htons((unsigned short)atoi(_desired_service))) == 0) {   
    cerr<<"Cannot get <"<<_desired_service<<"> service entry"<<endl;
    exit(EXIT_FAILURE);
  }

  // Map host name to IP address, allowing for dotted decimal
  struct hostent * phe;
  phe = gethostbyname(_host);
  if (phe) {
    memcpy(&sin_client.sin_addr, phe->h_addr, phe->h_length);
  } else if ( (sin_client.sin_addr.s_addr = inet_addr(_host)) == INADDR_NONE ) {
    cerr<<"Cannot get <"<<_host<<"> host entry"<<endl;
    exit(EXIT_FAILURE);
  }

  // Allocate socket
  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    cerr<<"Cannot create socket: "<<strerror(errno)<<endl;
    exit(EXIT_FAILURE);
  }

  // Connect the socket
  while (true) {
    int c = connect(s, (struct sockaddr *)&sin_client, sizeof(sin_client));
    if (c < 0) {
      if (errno == ECONNREFUSED) {
        continue;
      } else {
        cerr<<"Cannot connect to host: "<<_host<<"; service: "<<_desired_service<<"; error: "<<strerror(errno)<<endl;
        exit(EXIT_FAILURE);
      }
    } else { // Connection success
      break;
    }
  }

  return s;
}

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR FOR CLASS   R e q u e s t C h a n n e l  */
/*--------------------------------------------------------------------------*/

NetworkRequestChannel::NetworkRequestChannel(const string _server_host_name, const unsigned short _port_no){
  /* Creates a CLIENT-SIDE local copy of the channel. The channel is connected
     to the given port number at the given server host. 
     THIS CONSTRUCTOR IS CALLED BY THE CLIENT. */

  const char * host_name;
  const char * port_number;
  int int_port_number;

  host_name = _server_host_name.c_str();
  int_port_number = (int) _port_no;
  port_number = (int2string(int_port_number)).c_str();
  
  fd = client_TCP_connect(host_name, port_number);
  //cout<<"CLIENT: fd = "<<fd<<endl;
}

void * thread_kill_handler(void * args){
  THREADID_SOCKET * node = (THREADID_SOCKET *) args;
  pthread_join(node->thread_id, NULL);
  cout<<"closing socket = " <<node->socket<<endl;
  close (node->socket);
  delete node;
  pthread_exit(NULL);
}

NetworkRequestChannel::NetworkRequestChannel(const unsigned short _port_no, void * (*connection_handler) (void *), int _backlog){
  /* Creates a SERVER-SIDE local copy of the channel that is accepting connections
     at the given port number.
     NOTE that multiple clients can be connected to the same server-side end of the
     request channel. Whenever a new connection comes in, it is accepted by the 
     the server, and the given connection handler is invoked. The parameter to 
     the connection handler is the file descriptor of the slave socket returned
     by the accept call.
     NOTE that it is up to the connection handler to close the socket when the 
     client is done. */ 

  const char * port_number;
  int int_port_number;

  int_port_number = (int) _port_no;

  port_number = (int2string(int_port_number)).c_str();

  fd = server_TCP_socket(port_number, _backlog);
  cout<<"SERVER socket (file descriptor) = "<<fd<<endl;

  int sin_server_size = sizeof(sin_server);

  for (;;) {
    pthread_t channel_thread;
    int * slave_socket = new int;
    *slave_socket = accept(fd,(struct sockaddr*)&sin_server, (socklen_t *)&sin_server_size);
    if ( (slave_socket < 0) && (errno == EINTR) ) {
        continue;
    } else if (slave_socket < 0) {
      cerr<<"Accept failed: "<<strerror(errno)<<endl;
      exit(EXIT_FAILURE);
    }
    cout<<"Accept success! slave_socket = "<<*slave_socket<<endl;
    if (exit_server == true) {
      close(*slave_socket);
      break;
    }
    pthread_create(&channel_thread, NULL, connection_handler, (void*)slave_socket);
    THREADID_SOCKET * node = new THREADID_SOCKET;
    node->socket = *slave_socket;
    node->thread_id = channel_thread;
    pthread_t channel_wait;
    pthread_create(&channel_wait, NULL, thread_kill_handler, (void *) node);
  }
  cout<<"***************All server end channels are done***************"<<endl;
}

NetworkRequestChannel::~NetworkRequestChannel(){
  /* Destructor of the local copy of the bus. By default, the Server Side deletes 
     any IPC mechanisms associated with the channel. */
  close(fd);
}

/*--------------------------------------------------------------------------*/
/* READ/WRITE FROM/TO REQUEST CHANNELS  */
/*--------------------------------------------------------------------------*/

const int MAX_MESSAGE = 255;

string NetworkRequestChannel::send_request(string _request) {
  /* Send a string over the channel and wait for a reply. */
  cwrite(_request);
  string s = cread();
  return s;
}

string NetworkRequestChannel::cread() {
  /* Blocking read of data from the channel. Returns a string of characters
     read from the channel. Returns NULL if read failed. */
  char buf[MAX_MESSAGE];
  
  int is_read_error = read(fd, buf, MAX_MESSAGE); // read from file descriptor

  if ((is_read_error < 0) && (errno == EINTR)) { // interrupted by signal; continue monitoring
    cout<<"signal interrupt"<<endl;
    cread();
  } else if (is_read_error < 0) {
    perror(string("Request Channel ERROR: Failed reading from socket!").c_str());
  }
  
  string s = buf;
  return s;
}

int NetworkRequestChannel::cwrite(string _msg) {
  /* Write the data to the channel. The function returns the number of characters written
     to the channel. */
  if (_msg.length() >= MAX_MESSAGE) {
    cerr << "Message too long for Channel!\n";
    return -1;
  }
  const char * s = _msg.c_str();
  int is_write_error = write(fd, s, strlen(s)+1);

  if ((is_write_error < 0) && (errno == EINTR)) { // interrupted by signal; continue monitoring
    cout<<"signal interrupt"<<endl;
    cwrite(_msg);
  } else if (is_write_error < 0) {
    perror(string("Request Channel ERROR: Failed writing to socket!").c_str());
  }
}

