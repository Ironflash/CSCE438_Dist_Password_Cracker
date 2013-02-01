/* 
    File: request.c

    Author: Daniel Timothy S. Tan
            Department of Computer Science
            Texas A&M University
    Date  : 01/30/2013

    request client main program for HW2 in CSCE 438-500
*/

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include <string>
#include <iostream>
#include "semaphore.H"

//#include "request_lsp_api.c"

// temporary_class.c for temporary read/write to inbox/outbox
//#include "temporary_class.c"

using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTANTS/VARIABLES */
/*--------------------------------------------------------------------------*/

static int number_of_requesters;
static int number_of_data_requests;

static pthread_t *r_threads;

static Semaphore serv_response (1);

// INCOMPLETE: these will be called when LSP is complete
// Create Request Client-Server Communication channel
//static struct lsp_request* request_channel;

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

struct $
{
    /* data */
};

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS */
/*--------------------------------------------------------------------------*/

//313 Request thread functions:
void *request_thread(void * arguments) {
    // 
    for (int j=0; j<number_of_data_requests; j++){
    	string * request = new string;
    	*request = *(string *)arguments;
    	cout<<"Thread ID: "<<pthread_self()<<"; Attempting to write message: "<< *(request) <<endl;
        
        // temp: simulation of server response:
        string response;
        while (true) {
            serv_response.P();
            cin >> response;
            if (response == "ack") {
                serv_response.V();
                break;
            } else {
                cout<<"Server retrying..."<<endl;
            }
            serv_response.V();
        }

        cout<<"Server acknowledges Thread: "<<pthread_self()<<endl;

        // INCOMPLETE: these will be called when LSP is complete
        
        // write to request channel *************
        //lsp_request_write(request_channel,example,msg_length);
        //??The server needs to know which client and what thread requested
        //-possible solution: package message with client and thread IDs

        // read server response *****************
        //lsp_request_read();
    }
    cout<<"Thread: "<<pthread_self()<<" Quits"<<endl;
    pthread_exit(NULL);
}

void create_request_threads (){

	string request_message = "hello";
	string * test  = new string;
	*test = "a94a8fe5ccb19ba61c4c0873d391e987982fbbd3";
	string * testa = new string;
	*testa = "6d3c60eeb2ddd9cce8de6c092c091ae23ffd2264";
	string * testb = new string;
	*testb = "d159ab067e22da46146cc9841ad1cba346153523";

	int msg_length = 40; // default = 40 characters for a hash signature

	int new_thread;
	// Create a thread for each requester
    // INCOMPLETE: depending upon the input method for hash signatures, the thread input may change

    for (int i = 0; i<number_of_requesters; i++){
        if (i == 0) { // test
            new_thread = pthread_create(&r_threads[i], NULL, request_thread, (void*)test);
            if (new_thread){
                cout<<"ERROR: return code from pthread_create() is "<<new_thread<<endl;
                exit(-1);
            }
        } else if (i == 1) { // testa
            new_thread = pthread_create(&r_threads[i], NULL, request_thread, (void*)testa);
            if (new_thread){
                cout<<"ERROR: return code from pthread_create() is "<<new_thread<<endl;
                exit(-1);
            }
        } else if (i == 2) { // testb
            new_thread = pthread_create(&r_threads[i], NULL, request_thread, (void*)testb);
            if (new_thread){
                cout<<"ERROR: return code from pthread_create() is "<<new_thread<<endl;
                exit(-1);
            }
        }
    }
}

void wait_for_all_threads() {
    for (int i=0; i<number_of_requesters; i++){
        pthread_join(r_threads[i], NULL);
    }
    cout<<"***************All request threads are done***************"<<endl;

    // Idea: send a single quit request to the server; 
    // the server interprets this as the request client closing
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char **argv) {
	// INCOMPLETE: Needs to be able to take an input from test file

	// Initialization
	number_of_requesters = 3; // # of request_threads
    number_of_data_requests = 1; // how many messages assigned to 1 thread
    
    string host_name = "localhost";
    unsigned short port_number = 7000;

    // Possible use of getopt for command line user input to specify the following:
    // -n <number of data requests per person>]"<<endl;
    // -b <size of bounded buffer in requests>]"<<endl;
    // -w <number of worker threads>]"<<endl;
    // -h <name of server host>]"<<endl;
    // -p <port number of server host>]"<<endl;
    
    cout<<"----number of data requests = "<<number_of_data_requests<<endl;
    cout<<"--------name of server host = "<<host_name<<endl;
    cout<<"-port number of server host = "<<port_number<<endl;

    int status;
    pid_t pid;
    r_threads = new pthread_t [number_of_requesters];

    // INCOMPLETE: Initialize Request Client-Server Communication channel
    //request_channel = lsp_request_create(host_name,123);

    // Initialize threads:
    cout <<"Starting request threads"<<endl;
    create_request_threads();

	// ***********************************************************
    wait_for_all_threads();
    // Close the request client when done
    //lsp_request_close(request_channel);
    cout<<"Request client main completed successfully"<<endl;
    usleep(1000000);
    // ***********************************************************

    return 0;
}

/*
int main(int argc, char **argv) {

	// ***********************************************************
	// getopt code

    int index;
    int c = 0;
    cout<<"[-n <number of data requests per person>]"<<endl;
    cout<<"[-b <size of bounded buffer in requests>]"<<endl;
    cout<<"[-w <number of worker threads>]"<<endl;
    cout<<"[-h <name of server host>]"<<endl;
    cout<<"[-p <port number of server host>]"<<endl;
    while ((c = getopt (argc, argv, "n:b:w:h:p:")) != -1) {
        switch (c) {
            case 'n':
                number_of_data_requests = atoi(optarg);
                break;
            case 'b':
                max_buffer_size = atoi(optarg);
                break;
            case 'w':
                number_of_worker_threads = atoi(optarg);
                break;
            case 'h':
                host_name = optarg;
                break;
            case 'p':
                port_number = (unsigned short)(atoi(optarg));
                break;
            case '?':
                if ((optopt == 'n') ||
                    (optopt == 'b') || 
                    (optopt == 'w') ||
                    (optopt == 'h') ||
                    (optopt == 'p')) {
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
    
    // ***********************************************************
    // timer for performance metrics
    struct timeval tp_start; // Used to compute elapsed time.
    struct timeval tp_end;
    assert(gettimeofday(&tp_start, 0) == 0);
    // ***********************************************************
    
    // ***********************************************************
    assert(gettimeofday(&tp_end, 0) == 0);
    printf("Time taken for computation : "); 
    print_time_diff(&tp_start, &tp_end);
    printf("\n");
    clean_up_fifo();
    atexit(clean_up_fifo);
    return 0;
    // ***********************************************************
}
*/
