/* 
    File: server.c

    Author: Daniel Timothy S. Tan
            Department of Computer Science
            Texas A&M University
    Date  : 01/30/2013

    server main program for HW2 in CSCE 438-500
*/

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "server_lsp_api.c"

using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTANTS/VARIABLES */
/*--------------------------------------------------------------------------*/

// INCOMPLETE: these will be called when LSP is complete
// Create Server
//struct lsp_server* server_channel;

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS */
/*--------------------------------------------------------------------------*/

void server_loop() {
	bool exit = false;
	while(exit == false) {
		//num_read = lsp_server_read(serv,(void*) &input, &fake_id);

		if(num_read > 0)
		{
			printf("From Client: %s\n",input.c_str());
		}
	}
	//*/
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char **argv) {

	unsigned short port_number = 7000;
	//int backlog = 1; // number of client requests in queue
	//const int MAX_MESSAGE = 255;

	cout<<"------------port number = "<<port_number<<endl;
    cout<<"-----------backlog size = "<<backlog<<endl;

    cout<<"Establishing UDP-LSP Server Socket..."<<endl;

    // ***********************************************************
    // Temp TCP connection to test client-server communication
    /*
    NetworkRequestChannel server_channel(port_number, handle_data_requests, backlog);

    server_channel.~NetworkRequestChannel();
    cout<<"Closing TCP Server Socket..."<<endl;
    usleep(1000000);
    */
    // ***********************************************************

    // ***********************************************************
    // INCOMPLETE: Implement LSP eventually
    // server_channel = lsp_server_create(123);
    /*
    string input;
	uint32_t fake_id = 0;
	// int num_read = lsp_server_read(serv,(void*) &input, &fake_id);
	int num_read;
	*/
	// ***********************************************************


	// Initialize Server Loop
	server_loop();

	// ***********************************************************

	// Close the server when done
	// lsp_server_close(serv,1); //1 is just for testing needs to change
	cout<<"Server main completed successfully"<<endl;
	usleep(1000000);
	// ***********************************************************

	return 0;
}

// 313 server implementation
/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------/

void handle_process_loop(int * _fd);

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS -- TCP Socket Read/Write */
/*--------------------------------------------------------------------------/

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
/*--------------------------------------------------------------------------/

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
/*--------------------------------------------------------------------------/

void process_hello(int * _fd, const string & _request) {
	socket_write("hello to you too", _fd);
}

void process_data(int * _fd, const string & _request) {
	usleep(1000 + (rand() % 5000));
	socket_write((int2string(rand() % 100)), _fd);
}

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS -- THE PROCESS REQUEST LOOP */
/*--------------------------------------------------------------------------/

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
*/

/*
int main(int argc, char **argv) {

	// ***********************************************************
	// getopt code

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
    // ***********************************************************
}
*/
