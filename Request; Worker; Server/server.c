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
queue<string> message_queue;
static int total_received_requests = 0;
static int requests_being_processed = 0;
static int number_available_workers = 0;
static int workers_processing_requests = 0;
static int pop_tracker = 0;
static string worker_number[10] = {"1","2","3","4","5","6","7","8","9","10"};

// Print Debugging:
static int state_tracker = 0;
bool print_out = true;

pthread_mutex_t print_status_lock;
pthread_mutex_t workers_lock;
pthread_mutex_t messages_lock;
pthread_mutex_t pop_lock;

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

void print_server_status(bool print_out);

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS */
/*--------------------------------------------------------------------------*/

void * get_available_worker(void * args) {
	string * request = (string *) args;
	//DEBUG_MSG("Give this to the next available worker: "<<*request);
	for (;;) {
		if ( (number_available_workers > 0) && 
			(message_queue.size() != 0) ) {
			pthread_mutex_lock(&messages_lock);
			string request_to_process = message_queue.front();
			message_queue.pop();
			pthread_mutex_unlock(&messages_lock);

			pthread_mutex_lock(&workers_lock);
			DEBUG_MSG("****Giving request ("<<request_to_process<<") to worker # "<<workers.front());
			uint32_t worker_id = workers.front();
			DEBUG_MSG("worker fd to pop = "<<worker_id);
			workers.pop();
			number_available_workers = workers.size();
			pthread_mutex_unlock(&workers_lock);

			pthread_mutex_lock(&pop_lock);
			pop_tracker++;
			// always split to 3 workers
			if (pop_tracker == 4) {
				pop_tracker = 1;
			}
			string part = worker_number[pop_tracker-1];
			pthread_mutex_unlock(&pop_lock);

			workers_processing_requests++;
			print_server_status(print_out);
			lsp_server_write(server_channel,(request_to_process+part),0,worker_id);
		}
	}
	pthread_exit(NULL);
}

void process_request_udp(uint32_t client_id, string * message){
	cout<<"Request = "<<*message<<endl;
	if (client_id % 2 == 1) {
		DEBUG_MSG("******REQUESTER******");
		if (*message == "") {
			DEBUG_MSG("request is empty");
			total_received_requests--;
		} else {
			requesters.push(client_id);
			requests_being_processed++;

			// push 3 messages for the next 3 workers to process
			pthread_mutex_lock(&messages_lock);
			for (int i=0; i<3; i++) {
				message_queue.push(*message);
			}
			pthread_mutex_unlock(&messages_lock);

			print_server_status(print_out);
			DEBUG_MSG("Sending request to worker");

			//pthread_t thread_id;
	  		//pthread_create(& thread_id, NULL, get_available_worker, (void *)message);
		}
	} else if ((*message) == "join") {
		DEBUG_MSG("******WORKER******");
		pthread_mutex_lock(&workers_lock);
		workers.push(client_id);
		DEBUG_MSG("pushing worker (address): "<<client_id);
		pthread_mutex_unlock(&workers_lock);
		number_available_workers = workers.size();
		print_server_status(print_out);
	} else if ( ((*message).compare(0, 6, "Found:") == 0) ||
				((*message).compare(0, 10, "Not Found5") == 0) ) {
		DEBUG_MSG("******WORKER******");
		DEBUG_MSG("return password to requester");
		uint32_t requester_id = requesters.front();
		DEBUG_MSG("requester fd to pop = "<<requester_id);

		pthread_mutex_lock(&pop_lock);
		pop_tracker = 0;
		pthread_mutex_unlock(&pop_lock);

		requesters.pop();
		requests_being_processed--;
		workers_processing_requests--;
		print_server_status(print_out);
		lsp_server_write(server_channel,*message,0,requester_id);
		DEBUG_MSG("******Finished sending to requester******");
	} else if ( ((*message).compare(0, 9, "Not Found") == 0) ) {
		DEBUG_MSG("******WORKER******");
		DEBUG_MSG("return password to requester");
		uint32_t requester_id = requesters.front();
		DEBUG_MSG("requester fd to pop = "<<requester_id);

		cout<<"NOT FOUND"<<endl;
		workers_processing_requests--;
		print_server_status(print_out);
		DEBUG_MSG("******Finished sending to requester******");
		//*/
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
		cout<<"] "<<total_received_requests<<endl;;

		cout<<"Requests Being Processed"<<endl;
		cout<<"[";
		for (int i=0; i<requests_being_processed; i++) {
			cout<<":";
		}
		for (int i=0; i<(line_width-requests_being_processed); i++) {
			cout<<" ";
		}
		cout<<"] "<<requests_being_processed<<endl;;
		
		cout<<"Available Workers"<<endl;
		cout<<"[";
		for (int i=0; i<number_available_workers; i++) {
			cout<<":";
		}
		for (int i=0; i<(line_width-number_available_workers); i++) {
			cout<<" ";
		}
		cout<<"] "<<number_available_workers<<endl;;

		cout<<"Workers Processing Requests"<<endl;
		cout<<"[";
		for (int i=0; i<workers_processing_requests; i++) {
			cout<<":";
		}
		for (int i=0; i<(line_width-workers_processing_requests); i++) {
			cout<<" ";
		}
		cout<<"] "<<workers_processing_requests<<endl;;
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
	int num_read;
	print_server_status(print_out);

	pthread_t thread_id;
  	pthread_create(& thread_id, NULL, get_available_worker, (void *)"temp");

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
	
	// ***********************************************************
	// Close the server when done
	// lsp_server_close(server_channel,1); // UDP-LSP 1 is just for testing needs to change
	
	cout<<"Server main completed successfully"<<endl;
	usleep(1000000);
	// ***********************************************************

	return 0;
}
