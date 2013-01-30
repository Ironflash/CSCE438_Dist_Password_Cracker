/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

// #include "request_lsp_api.c"

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS */
/*--------------------------------------------------------------------------*/

/* 313 Request thread functions:
	void *request_thread(void * arguments) {
	    for (int j=0; j<number_of_data_requests; j++){
	        REQUEST_NODE * request = new REQUEST_NODE;
	        *request = *(REQUEST_NODE *)arguments;
	        bounded_buffer->insert(request);
	    }
	    pthread_exit(NULL);
	}

	void create_request_threads (){
	    int new_thread;
	    // create a child thread for each requester
	    for (int i = 0; i<number_of_requesters; i++){
	        // create requests and store in bounded buffer
	        REQUEST_NODE *request = new REQUEST_NODE;
	        request->requester_id = i;
	        if (i == 0) { // Joe Smith
	            request->request_name = "data Joe Smith";
	            new_thread = pthread_create(&r_threads[i], NULL, request_thread, (void*)request);
	            if (new_thread){
	                cout<<"ERROR: return code from pthread_create() is "<<new_thread<<endl;
	                exit(-1);
	            }
	        } else if (i == 1) { // Jane Smith
	            request->request_name = "data Jane Smith";
	            new_thread = pthread_create(&r_threads[i], NULL, request_thread, (void*)request);
	            if (new_thread){
	                cout<<"ERROR: return code from pthread_create() is "<<new_thread<<endl;
	                exit(-1);
	            }
	        } else if (i == 2) { // John Doe
	            request->request_name = "data John Doe";
	            new_thread = pthread_create(&r_threads[i], NULL, request_thread, (void*)request);
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
	    for (int i=0; i<number_of_worker_threads; i++){
	        REQUEST_NODE *request = new REQUEST_NODE;
	        request->requester_id = 0;
	        request->request_name = "quit";
	        bounded_buffer->insert(request);
	    }
	    for (int i=0; i<number_of_worker_threads; i++){
	        pthread_join(w_threads[i], NULL);
	    }
	    cout<<"***************All worker threads are done***************"<<endl;
	    for (int i=0; i<number_of_requesters; i++){
	        STATISTIC_NODE *quit_stat = new STATISTIC_NODE;
	        quit_stat->requester_id = 0;
	        quit_stat->statistic_value = -1;
	        deposit->insert(quit_stat);
	    }
	    for (int i=0; i<number_of_requesters; i++){
	        pthread_join(s_threads[i], NULL);
	    }
	    cout<<"***************All statistic threads are done***************"<<endl;
	}

	*/

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char **argv) {
	// Needs to be able to take an input from test file

	// Initialization

	number_of_requesters = 3; // # of request_threads
    number_of_data_requests = 10;
    
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

    // Initialize threads:
    cout <<"Starting threads"<<endl;
    
	// Create request
	//struct lsp_request* request_channel = lsp_request_create(host_name,123);

	string request_message = "new test";
	// "test" = a94a8fe5ccb19ba61c4c0873d391e987982fbbd3
	// "testa" = 6d3c60eeb2ddd9cce8de6c092c091ae23ffd2264
	// "testb" = d159ab067e22da46146cc9841ad1cba346153523

	int msg_length = 0; // 40 characters

	// write 
	//lsp_request_write(request_channel,example,msg_length);
	//lsp_request_read();

	// Close the request when done
	//lsp_request_close(request_channel);
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
    
    
    create_request_threads();
    // create worker threads
    int new_worker_thread;
    for (int i = 0; i<number_of_worker_threads; i++){
        // remove requests from the bounded buffer
        NetworkRequestChannel *worker_channel = new NetworkRequestChannel (host_name, port_number);
        new_worker_thread = pthread_create(&w_threads[i], NULL, worker_thread, worker_channel);
        if (new_worker_thread){
            cout<<"ERROR: return code from pthread_create() is "<<new_worker_thread<<endl;
            exit(-1);
        }
    }
    create_statistics_threads();
    wait_for_all_threads();
    NetworkRequestChannel quit(host_name, port_number);
    cout<<"Main completed successfully"<<endl;
    usleep(1000000);

    print_histograms();
    assert(gettimeofday(&tp_end, 0) == 0);
    printf("Time taken for computation : "); 
    print_time_diff(&tp_start, &tp_end);
    printf("\n");
    clean_up_fifo();
    atexit(clean_up_fifo);
    return 0;
}
*/