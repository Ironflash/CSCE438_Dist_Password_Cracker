/* 
    File: server.c

    Authors: Daniel Timothy S. Tan
             Department of Computer Science
             Texas A&M University
    Date   : 01/30/2013

    server main program for HW2 in CSCE 438-500
*/

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include <iostream>
//#include "server_lsp_api.c"

#include <queue>
#include <errno.h>
#include "netreqchannel.h"

using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTANTS/VARIABLES */
/*--------------------------------------------------------------------------*/

// Create Server
struct lsp_server* server_channel;

const int MAX_MESSAGE = 255;
queue<int*> requesters;
queue<int*> workers;
static int number_available_workers = 0;

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
/* FORWARDS */
/*--------------------------------------------------------------------------*/

void handle_process_loop(int * _fd);

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS */
/*--------------------------------------------------------------------------*/

void get_available_worker(string request) {
	cout<<"Give this to the next available worker: "<<request<<endl;
	for (;;) {
		if (number_available_workers > 0) {
			cout<<"****Giving request ("<<request<<") to worker # "<<workers.front()<<endl;
			cout<<"workers size = "<<number_available_workers;
			int * _fd = (int *)workers.front();
			cout<<"worker fd to pop = "<<*(_fd)<<endl;
			socket_write(request, _fd);
			workers.pop();
			number_available_workers = workers.size();
			cout<<"workers size = "<<number_available_workers;
			break;
		}
	}
}

void * handle_data_requests(void * args) {
  //nthreads++;
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

void process_request(int * _fd, const string & _request) {
	if (_request.length() == 40) {
		socket_write("****Server successfully received hash****", _fd);
		
		requesters.push(_fd);
		cout<<"Send request to worker"<<endl;
		get_available_worker(_request);
	} else if (_request.compare(0, 4, "join") == 0) {
		socket_write("****Server accepts your join request****", _fd);

		workers.push(_fd);
		cout<<"pushing worker (address): "<<_fd<<endl;
		number_available_workers = workers.size();
	}else {
		socket_write("INVALID hash signature", _fd);
	}
}

void handle_process_loop(int * _fd) {

  for(;;) {

    string request = socket_read(_fd);
    //cout << "done." << endl;
    //cout << "New request is " << request << endl;

    if (request.compare("quit") == 0) {
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

	// ***********************************************************
    // getopt code
    int index;
    int c = 0;
    cout<<"[-p <port number of server host>]"<<endl;
    while ((c = getopt (argc, argv, "p:")) != -1) {
        switch (c) {
            case 'p':
                port_number = (unsigned short)(atoi(optarg));
                break;
            case '?':
                if ((optopt == 'p')) {
                    cout<<"ERROR: Option -"<<optopt<<" requires an argument"<<endl;
                } else if (isprint (optopt)) {
                    cout<<"ERROR: Unknown option for -"<<optopt<<endl;
                } else {
                    cout<<"ERROR: Unknown option character"<<endl;
                }
                return 1;
            default:
                abort ();
        }
    }
    for (index = optind; index < argc; index++) {
        printf ("Non-option argument %s\n", argv[index]);
    }
    // ***********************************************************

	cout<<"------------port number = "<<port_number<<endl;

	// Initialize Server Communication Channel
    cout<<"Establishing UDP-LSP Server Socket..."<<endl;

    // ***********************************************************
    // Implement LSP eventually
    // server_channel = lsp_server_create(port_number); // UDP-LSP
    NetworkRequestChannel* server_channel;
    server_channel = lsp_server_create(port_number, handle_data_requests);

    string input;
    uint32_t fake_id = 0;
    uint32_t send_response_to;
	//int num_read = lsp_server_read(server_channel,(void*) &input, &fake_id);
	int num_read;
	int answer_length = 4;
	/*
	while(true) {
		num_read = lsp_server_read(server_channel,(void*) &input, &fake_id);
		if(num_read > 0) {
			cout<<"From Client (ID="<<fake_id<<"): "<<input.c_str()<<endl;
			//usleep(1000000);
			//cout<<"response to: "<<fake_id<<endl;
			string answer = "test";
			lsp_server_write(server_channel, &answer, answer_length, fake_id);
			//break;
			for (;;){
				//block
			}
		}
	}
	//*/
	cout<<"EXIT WHILE loop =) "<<endl;
	//lsp_server_write(server_channel, void* pld, int lth, uint32_t conn_id);
	//bool lsp_server_write(lsp_server* a_srv, void* pld, int lth, uint32_t conn_id);
    /*
    string input;
	uint32_t fake_id = 0;
	// int num_read = lsp_server_read(serv,(void*) &input, &fake_id);
	int num_read;
	//*/
	// ***********************************************************

	// Initialize Server Loop
	//handle_process_loop();

	// ***********************************************************

	// Close the server when done
	// lsp_server_close(server_channel,1); // UDP-LSP 1 is just for testing needs to change
	
	// lsp_server_close(server_channel,1); // TCP
	cout<<"Server main completed successfully"<<endl;
	usleep(1000000);
	// ***********************************************************

	return 0;
}