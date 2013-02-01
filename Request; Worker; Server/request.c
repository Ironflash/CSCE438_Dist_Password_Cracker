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
//#include "semaphore.h"
#include "request_lsp_api.c"

// temporary_class.c for temporary read/write to inbox/outbox
//#include "temporary_class.c"

using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTANTS/VARIABLES */
/*--------------------------------------------------------------------------*/

// Create Request Client-Server Communication channel
static struct lsp_request* request_channel;

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char **argv) {
    
    string host_name = "localhost";
    unsigned short port_number = 123;

    // "test" = "a94a8fe5ccb19ba61c4c0873d391e987982fbbd3";
    string request_msg = "a94a8fe5ccb19ba61c4c0873d391e987982fbbd3";
    int msg_length = 40;

    // ***********************************************************
    // getopt code
    int index;
    int c = 0;
    cout<<"[-h <name of server host>]"<<endl;
    cout<<"[-p <port number of server host>]"<<endl;
    cout<<"[-r <hash signature request>]"<<endl;
    cout<<"[-l <hash signature length>]"<<endl;
    while ((c = getopt (argc, argv, "h:p:r:l:")) != -1) {
        switch (c) {
            case 'h':
                host_name = optarg;
                break;
            case 'p':
                port_number = (unsigned short)(atoi(optarg));
                break;
            case 'r':
                request_msg = optarg;
                break;
            case 'l':
                msg_length = atoi(optarg);
                break;
            case '?':
                if ((optopt == 'h') ||
                    (optopt == 'p') || 
                    (optopt == 'm') ||
                    (optopt == 'l')) {
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
    
    cout<<"--------name of server host = "<<host_name<<endl;
    cout<<"-port number of server host = "<<port_number<<endl;
    cout<<"-----hash signature request = "<<request_msg<<endl;
    cout<<"------hash signature length = "<<msg_length<<endl;

    // Initialize Request Client-Server Communication Channel
    cout <<"Initializing Request Channel...."<<endl;
    request_channel = lsp_request_create(host_name.c_str(),port_number);

    string temp;
    for (;;){
        cin>>temp;
        if (temp == "exit") {
            break;
        } else {

        }
    }

	// ***********************************************************
    // Close the request client when done
    lsp_request_close(request_channel);
    cout<<"Request client main completed successfully"<<endl;
    usleep(1000000);
    // ***********************************************************

    return 0;
}

/*
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

int main(int argc, char **argv) {
    
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
