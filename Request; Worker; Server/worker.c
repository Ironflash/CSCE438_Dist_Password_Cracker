/* 
    File: worker.c

    Author: Daniel Timothy S. Tan
            Department of Computer Science
            Texas A&M University
    Date  : 01/30/2013

    worker client main program for HW2 in CSCE 438-500
*/

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include <string>
#include <iostream>
#include "worker_lsp_api.c"

using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTANTS/VARIABLES */
/*--------------------------------------------------------------------------*/
static int number_of_worker_threads;

static pthread_t e_thread;
static pthread_t *w_threads;

// Create Request Client-Server Communication channel
static struct lsp_request* worker_channel;

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS */
/*--------------------------------------------------------------------------*/

void password_cracker(string hash) {
    // write the algorithm for password cracking
    // for loop - iterate from a-z, aa-zz, .. aaaaaa - zzzzzz
    // password found
    string found = "Found: ";
    string password = "test";
    // password not found: answer = Not found
    string answer = found+password;
    cout<<"FOUND PASSWORD: "<<password<<endl;
    lsp_request_write(worker_channel,answer,answer.length()); //UDP-LSP
}

void send_join_request(){
    // send a join request to the server
    int msg_length = 4;
    string request_msg = "join";
    lsp_request_write(worker_channel,request_msg,msg_length); //UDP-LSP
}

//313 worker thread/event handler functions:

void *worker_thread(void * arguments) {
    //cout<<"**************************Thread ID ["<<pthread_self()<<"]"<<endl;
    /*
    NetworkRequestChannel * worker_channel = ((NetworkRequestChannel*)arguments);
    while (true) {
        REQUEST_NODE * removed_request = new REQUEST_NODE;
        removed_request = bounded_buffer->remove();
        int requester = removed_request->requester_id;
        if ((removed_request->request_name) == "quit") {
            worker_channel->send_request(removed_request->request_name);
            break;
        }
        string reply = (*worker_channel).send_request(removed_request->request_name);
        int random_response = atoi(reply.c_str());
        STATISTIC_NODE * new_statistic = new STATISTIC_NODE;
        new_statistic->requester_id = requester;
        new_statistic->statistic_value = random_response;
        deposit->insert(new_statistic);
    }
    delete worker_channel;
    pthread_exit(NULL);
    */
}

void *event_handler_thread(void * arguments) {
    /*
    cout<<"**************************Running Event Handler"<<endl;
    int have_read[number_of_worker_threads]; //keeps track of what channels have been read from
    int have_written[number_of_worker_threads]; //keeps track of what channels have been written to
    int requester_ids[number_of_worker_threads]; //keeps track of what channels have been written to
    int requests_left = 0;
    int max_fd = 0;
    fd_set readset;
    bool exit_thread = false;
    for (int i=0;i <number_of_worker_threads; i++){
        have_read[i] = 0;
        have_written[i] = 0;
        requester_ids[i] = 0;
    }
    // loop till bounded_buffer is completely empty
    while (!exit_thread) {
        FD_ZERO(&readset);
        for (int i = 0; i < number_of_worker_threads; i++) {
            // we skip all the necessary error checking
            FD_SET(r_fd[i], &readset);
            max_fd = max(r_fd[i], max_fd);
        }
        // pump requests through all the channels that haven't been read from
        for (int i=0; i<number_of_worker_threads; i++){
            if (have_read[i] == 0) { // this fd has been read thus we can write to it
                have_written[i] = 0;
                REQUEST_NODE * removed_request = new REQUEST_NODE;
                removed_request = bounded_buffer->remove();
                requester_ids[i] = removed_request->requester_id;
                channel_pointer[i]->cwrite(removed_request->request_name);
                /*
                const char * s = (removed_request->request_name).c_str();
                if (write(w_fd[i], s, strlen(s)+1) < 0) {
                    perror(string("ERROR: Writing to pipe! FD = %d\n", w_fd[i]).c_str());
                }
                //*/
                /*
                have_written[i] = 1;
                requests_left++;
                if ((removed_request->request_name) == "quit") {
                    have_read[i] = -1;
                    cout<<"...Quitting..."<<endl;
                    exit_thread = true;
                    break;
                }
            }
        }
        if (exit_thread) {
            while (requests_left > 0) {
                FD_ZERO(&readset);
                for (int i = 0; i < number_of_worker_threads; i++) {
                    // we skip all the necessary error checking
                    FD_SET(r_fd[i], &readset);
                    max_fd = max(r_fd[i], max_fd);
                }
                int numready = select(max_fd+1, &readset, NULL, NULL, NULL);
                if ((numready == -1) && (errno == EINTR)) { // interrupted by signal; continue monitoring
                    cout<<"signal interrupt"<<endl;
                    continue;
                } else if (numready == -1) { // a real error happened; abort monitoring
                    cout<<"select() error"<<endl;
                    break;
                }
                for (int i = 0; i < number_of_worker_threads; i++) {
                    if (FD_ISSET(r_fd[i], &readset)) { // this file descriptor is ready
                        //char buf[255];
                        string read_response = channel_pointer[i]->cread();
                        //int bytesread = read(r_fd[i], buf, 255);
                        //string read_response = buf;
                        if (read_response != "bye") {
                            int random_response = atoi(read_response.c_str());
                            STATISTIC_NODE * new_statistic = new STATISTIC_NODE;
                            new_statistic->requester_id = requester_ids[i];
                            new_statistic->statistic_value = random_response;
                            deposit->insert(new_statistic);
                        }
                        requests_left--;
                    }
                }
            }
            break;
        }
        bool done = false;
        while (!done) {
            int numready = select(max_fd+1, &readset, NULL, NULL, NULL);
            if ((numready == -1) && (errno == EINTR)) {
                // interrupted by signal; continue monitoring
                cout<<"signal interrupt"<<endl;
                continue;
            } else if (numready == -1) {
                // a real error happened; abort monitoring
                cout<<"select() error"<<endl;
                break;
            }
            for (int i = 0; i < number_of_worker_threads; i++) {
                have_read[i] = 1;
                if (FD_ISSET(r_fd[i], &readset)) { // this descriptor is ready
                    char buf[255];
                    int bytesread = read(r_fd[i], buf, 255);
                    string read_response = buf;
                    int random_response = atoi(read_response.c_str());
                    STATISTIC_NODE * new_statistic = new STATISTIC_NODE;
                    new_statistic->requester_id = requester_ids[i];
                    new_statistic->statistic_value = random_response;
                    deposit->insert(new_statistic);
                    have_read[i] = 0;
                    requests_left--;
                    done = true;
                }
            }
        }
    }
    for (int i=1; i<number_of_worker_threads; i++){
        if ((have_read[i] == 0) || (have_read[i] == 1)) {
            string temp = "quit";
            const char * s = (temp).c_str();
            if (write(w_fd[i], s, strlen(s)+1) < 0) {
                perror(string("ERROR: Writing to pipe! FD = %d\n", w_fd[i]).c_str());
            }
        }
    }
    pthread_exit(NULL);
    */
}

void wait_for_all_threads() {
    // Idea: the server sends a quit request to the worker;
    // since the server is closing, the worker shuts down all worker threads
    cout<<"***************All request threads are done***************"<<endl;
    /*
    REQUEST_NODE *request = new REQUEST_NODE;
    request->requester_id = 0;
    request->request_name = "quit";
    bounded_buffer->insert(request);
    pthread_join(e_thread, NULL);
    */
    cout<<"***************Event handler thread is done***************"<<endl;
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char **argv) {
    // Initialization

    const char * host_name = "localhost";
    unsigned short port_number = 1235;

    number_of_worker_threads = 2;

    // ***********************************************************
    // getopt code
    int index;
    int c = 0;
    cout<<"[-w <number of worker threads>]"<<endl;
    cout<<"[-h <name of server host>]"<<endl;
    cout<<"[-p <port number of server host>]"<<endl;
    while ((c = getopt (argc, argv, "h:p:r:l:")) != -1) {
        switch (c) {
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
                if ((optopt == 'w') ||
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

    cout<<"-----------number of worker threads = "<<number_of_worker_threads<<endl;
    cout<<"----------------name of server host = "<<host_name<<endl;
    cout<<"---------port number of server host = "<<port_number<<endl;
    
    int status;
    pid_t pid;
    w_threads = new pthread_t [number_of_worker_threads];

    // Initialize threads:
    cout <<"Initializing Worker Channel...."<<endl;
    worker_channel = lsp_request_create(host_name,port_number);

    send_join_request();

    // ***********************************************************
    // Initialize Worker Loop
    string input;
    while(true) {
        int numRead = lsp_request_read(worker_channel,(void*) &input);
        if(numRead > 0) {
            cout<<"!!! Let's Crack This Password !!!"<<endl;
            cout<<"Password: "<<input<<endl;
            password_cracker(input);
            // now that a password has been cracked, restart

            // TODO worker will close when the server closes it:
            // Write a function to handle a server closing the worker msg
            send_join_request();
        }
    }
    // ***********************************************************

    // ***********************************************************
    // Worker thread implementation
    /*
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
    */
    // ***********************************************************    

    // ***********************************************************
    // Event handler implementation
    /*
    // create event handler thread
        for (int i = 0; i<number_of_worker_threads; i++){
            // remove requests from the bounded buffer
            string new_thread_name = chan->send_request("newthread");
            RequestChannel *worker_channel = new RequestChannel (new_thread_name, RequestChannel::CLIENT_SIDE);
            r_fd[i] = worker_channel->read_fd();
            w_fd[i] = worker_channel->write_fd();
            channel_pointer[i] = worker_channel;
        }
        int event_handler = pthread_create(&e_thread, NULL, event_handler_thread, NULL);
        if (event_handler){
            cout<<"ERROR: return code from pthread_create() is "<<event_handler<<endl;
            exit(-1);
        }
    */
    // ***********************************************************

    // ***********************************************************
    //wait_for_all_threads();
    // Close the request client when done
    //lsp_worker_close(worker_channel);
    cout<<"Request client main completed successfully"<<endl;
    usleep(1000000);
    // ***********************************************************

    return 0;
}
