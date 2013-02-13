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
#include "server_lsp_api.c"

#include <pthread.h>
#include <queue>
#include <errno.h>

using namespace std;

//#define DEBUG // uncomment to turn on print outs
#ifdef DEBUG
#define DEBUG_MSG(str) do { std::cout << str << std::endl; } while( false )
#else
#define DEBUG_MSG(str) do { } while ( false )
#endif

/*--------------------------------------------------------------------------*/
/* CONSTANTS/VARIABLES */
/*--------------------------------------------------------------------------*/

// Create Server
struct lsp_server* server_channel;

const int MAX_MESSAGE = 255;
queue<uint32_t> requesters;
queue<uint32_t> workers;
static int total_received_requests = 0;
static int requests_being_processed = 0;
static int number_available_workers = 0;
static int workers_processing_requests = 0;

// Print Debugging:
static int state_tracker = 0;
bool print_out = false;

pthread_mutex_t print_status_lock;

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

//void handle_process_loop(int * _fd);
void print_server_status(bool print_out);

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

/*
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
//*/

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS */
/*--------------------------------------------------------------------------*/

void * get_available_worker(void * args) {
//void get_available_worker(string request) {
	string * request = (string *) args;

	DEBUG_MSG("Give this to the next available worker: "<<*request);
	for (;;) {
		if (number_available_workers > 0) {
			DEBUG_MSG("****Giving request ("<<*request<<") to worker # "<<workers.front());
			uint32_t worker_id = workers.front();
			DEBUG_MSG("worker fd to pop = "<<worker_id);
			workers.pop();
			number_available_workers = workers.size();
			workers_processing_requests++;
			print_server_status(print_out);

			lsp_server_write(server_channel,*request,0,worker_id);
			break;
		}
	}
	pthread_exit(NULL);
}

void process_request_udp(uint32_t client_id, string * message){
	//From requester or worker?
	// if ((*message).length() == 40) { // From requester
	if (client_id % 2 == 1) {
		DEBUG_MSG("******REQUESTER******");
		requesters.push(client_id);
		requests_being_processed++;
		print_server_status(print_out);
		DEBUG_MSG("Send request to worker");
		pthread_t thread_id;
  		pthread_create(& thread_id, NULL, get_available_worker, (void *)message);  		
	} else if ((*message) == "join") {
		DEBUG_MSG("******WORKER******");
		workers.push(client_id);
		DEBUG_MSG("pushing worker (address): "<<client_id);
		number_available_workers = workers.size();
		print_server_status(print_out);
	} else if ( ((*message).compare(0, 6, "Found:") == 0) || 
				((*message).compare(0, 9, "Not Found") == 0) ) {
		DEBUG_MSG("******WORKER******");
		DEBUG_MSG("return password to requester");
		uint32_t requester_id = requesters.front();
		DEBUG_MSG("requester fd to pop = "<<requester_id);
		//socket_write(request, _fd);
		requesters.pop();
		requests_being_processed--;
		workers_processing_requests--;
		print_server_status(print_out);
		lsp_server_write(server_channel,*message,0,requester_id);
		DEBUG_MSG("******Finished sending to requester******");
	} else {

	}
}

void print_server_status(bool print_out_bool){
	if (print_out_bool == true) {
		int line_width = 30;
		state_tracker++;
		pthread_mutex_lock(&print_status_lock);
		cout<<"******************************************"<<state_tracker<<endl;
		cout<<"Total Requests Received"<<endl;
		cout<<"[";
		for (int i=0; i<total_received_requests; i++) {
			cout<<":";
		}
		for (int i=0; i<(line_width-total_received_requests); i++) {
			cout<<" ";
		}
		cout<<"]"<<endl;;

		cout<<"Requests Being Processed"<<endl;
		cout<<"[";
		for (int i=0; i<requests_being_processed; i++) {
			cout<<":";
		}
		for (int i=0; i<(line_width-requests_being_processed); i++) {
			cout<<" ";
		}
		cout<<"]"<<endl;;
		
		cout<<"Available Workers"<<endl;
		cout<<"[";
		for (int i=0; i<number_available_workers; i++) {
			cout<<":";
		}
		for (int i=0; i<(line_width-number_available_workers); i++) {
			cout<<" ";
		}
		cout<<"]"<<endl;;

		cout<<"Workers Processing Requests"<<endl;
		cout<<"[";
		for (int i=0; i<workers_processing_requests; i++) {
			cout<<":";
		}
		for (int i=0; i<(line_width-workers_processing_requests); i++) {
			cout<<" ";
		}
		cout<<"]"<<endl;;
		pthread_mutex_unlock(&print_status_lock);
	}
}


/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char **argv) {

	unsigned short port_number = 7000;
	pthread_mutex_init(&print_status_lock, NULL);

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
    server_channel = lsp_server_create(port_number); // UDP-LSP

    string input;
    uint32_t fake_id = 0;
    //uint32_t send_response_to;
	int num_read;
	//int answer_length = 4;
	print_server_status(print_out);
	while(true) {
		//cout<<"!!!!!!!!!!!!! Attempting to read !!!!!!!!!!!!!"<<endl;
		num_read = lsp_server_read(server_channel,(void*) &input, &fake_id);
		if(num_read > 0) {
			DEBUG_MSG("*********Received a Request*********");
			total_received_requests++;
			string * process = new string;
			*process = input;
			process_request_udp(fake_id, process);
			//process_request_udp(fake_id, &input);
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
	
	//handle_process_loop();

	// ***********************************************************

	// Close the server when done
	// lsp_server_close(server_channel,1); // UDP-LSP 1 is just for testing needs to change
	
	cout<<"Server main completed successfully"<<endl;
	usleep(1000000);
	// ***********************************************************

	return 0;
}