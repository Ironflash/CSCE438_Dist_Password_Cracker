/* 
    File: dataserver.C

    Author: Daniel Timothy S. Tan
            Department of Computer Science
            Texas A&M University
    Date  : 2012/08/01

    Dataserver main program for MP5 in CSCE 313
*/

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

#include <pthread.h>
#include <errno.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "netreqchannel.H"

using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

const int MAX_MESSAGE = 255;

/*--------------------------------------------------------------------------*/
/* VARIABLES */
/*--------------------------------------------------------------------------*/

static int nthreads = 0;
extern bool exit_server;
extern string int2string(int number);

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

void handle_process_loop(int * _fd);

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS -- TCP Socket Read/Write */
/*--------------------------------------------------------------------------*/

string socket_read(int * _fd) {
  char buf[MAX_MESSAGE];
  
  int is_read_error = read(* _fd, buf, MAX_MESSAGE); // read from file descriptor

  if ((is_read_error < 0) && (errno == EINTR)) { // interrupted by signal; continue monitoring
    cout<<"signal interrupt"<<endl;
    socket_read(_fd);
  } else if (is_read_error < 0) {
    perror(string("Request Channel ERROR: Failed reading from socket!").c_str());
  }
  
  string s = buf;
  return s;
}

int socket_write(string _msg, int * _fd) {
  if (_msg.length() >= MAX_MESSAGE) {
    cerr << "Message too long for Channel!\n";
    return -1;
  }
  const char * s = _msg.c_str();

  int is_write_error = write(* _fd, s, strlen(s)+1);

  if ((is_write_error < 0) && (errno == EINTR)) { // interrupted by signal; continue monitoring
    cout<<"signal interrupt"<<endl;
    socket_write(_msg, _fd);
  } else if (is_write_error < 0) {
    perror(string("Request Channel ERROR: Failed writing to socket!").c_str());
  }
}

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS -- THREAD FUNCTIONS */
/*--------------------------------------------------------------------------*/

void * handle_data_requests(void * args) {
  nthreads++;
  //cout<<"Starting thread, with ID: "<<pthread_self()<<endl;
  int * socket = (int *) args;

  // -- Handle client requests on this channel. 
  
  handle_process_loop(socket);

  // -- Client has quit. We remove channel.

  //cout<<"Closing thread, with ID: "<<pthread_self()<<endl;
  //close(*socket);
  //delete socket;
  pthread_exit(NULL);
}

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS -- INDIVIDUAL REQUESTS */
/*--------------------------------------------------------------------------*/


void process_hello(int * _fd, const string & _request) {
  socket_write("hello to you too", _fd);
}

void process_data(int * _fd, const string & _request) {
  usleep(1000 + (rand() % 5000));
  socket_write((int2string(rand() % 100)), _fd);
}

/*
void process_newthread(RequestChannel & _channel, const string & _request) {
  int error;
  nthreads ++;

  // -- Name new data channel

  string new_channel_name = "data" + int2string(nthreads) + "_";
  cout << "new channel name = " << new_channel_name << endl;

  // -- Pass new channel name back to client

  _channel.cwrite(new_channel_name);

  // -- Construct new data channel (pointer to be passed to thread function)
  
  RequestChannel * data_channel = new RequestChannel(new_channel_name, RequestChannel::SERVER_SIDE);

  // -- Create new thread to handle request channel

  pthread_t thread_id;
  cout << "staring new thread " << nthreads << endl;
  if (error = pthread_create(& thread_id, NULL, handle_data_requests, data_channel)) {
    fprintf(stderr, "p_create failed: %s\n", strerror(error));
  }  

}
//*/

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS -- THE PROCESS REQUEST LOOP */
/*--------------------------------------------------------------------------*/


void process_request(int * _fd, const string & _request) {

  if (_request.compare(0, 5, "hello") == 0) {
    //cout<<"processing '"<<_request<<"' request"<<endl;
    process_hello(_fd, _request);
  } else if (_request.compare(0, 4, "data") == 0) {
    //cout<<"processing '"<<_request<<"' request"<<endl;
    process_data(_fd, _request);
  } else {
    //cout<<"processing '"<<_request<<"' request"<<endl;
    //socket_write("unknown request", _fd);
  }
}

void handle_process_loop(int * _fd) {

  for(;;) {

    //cout << "Reading next request from channel..." << flush;
    string request = socket_read(_fd);
    //cout << "done." << endl;
    //cout << "New request is " << request << endl;

    if (request.compare("quit") == 0) {
      nthreads--;
      if (nthreads == 0) {
        exit_server = true;
      }
      socket_write("bye", _fd);
      usleep(10000);          // give the other end a bit of time.
      break;                  // break out of the loop;
    }

    process_request(_fd, request);
  }
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char **argv) {
  unsigned short port_number = 7000;
  int backlog = 1; // number of client requests in queue
  const int MAX_MESSAGE = 255;
  
  int index;
  int c = 0;
  cout<<"[-p <port number for data server>]"<<endl;
  cout<<"[-b <backlog of the server socket>]"<<endl;
  while ((c = getopt (argc, argv, "p:b:")) != -1) {
    switch (c) {
      case 'p':
        port_number = (unsigned short)(atoi(optarg));
        break;
      case 'b':
        backlog = atoi(optarg);
        break;
      case '?':
        if (optopt == 'p') {
          cout<<"ERROR: Option -p requires an argument"<<endl;
        } else if (isprint (optopt)) {
          cout<<"ERROR: Unknown option for -p"<<endl;
        } else {
          cout<<"ERROR: Unknown option character"<<endl;
        }
        return 1;
        if (optopt == 'b') {
          cout<<"ERROR: Option -b requires an argument"<<endl;
        } else if (isprint (optopt)) {
          cout<<"ERROR: Unknown option for -b"<<endl;
        } else {
          cout<<"ERROR: Unknown option character"<<endl;
        }
        return 1;
      default:
        abort();
      }
    }
    for (index = optind; index < argc; index++) {
        printf ("Non-option argument %s\n", argv[index]);
    }

    cout<<"------------port number = "<<port_number<<endl;
    cout<<"-----------backlog size = "<<backlog<<endl;

    cout<<"Establishing TCP Server Socket..."<<endl;

    NetworkRequestChannel server_channel(port_number, handle_data_requests, backlog);

    server_channel.~NetworkRequestChannel();
    cout<<"Closing TCP Server Socket..."<<endl;
    usleep(1000000);
}
