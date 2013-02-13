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
#include <fstream>
#include <vector>
#include <assert.h>
#include <sys/time.h>

#include "worker_lsp_api.c"

//#define DEBUG // uncomment to turn on print outs
#ifdef DEBUG
#define DEBUG_MSG(str) do { std::cout << str << std::endl; } while( false )
#else
#define DEBUG_MSG(str) do { } while ( false )
#endif

using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTANTS/VARIABLES */
/*--------------------------------------------------------------------------*/

static string request_hash;
static bool last = false;

static int number_of_worker_threads;
static pthread_t *w_threads;

static string alphabet[26] = { "a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z" };

static string answer;
static bool stop_searching = false;

// Create Request Client-Server Communication channel
static struct lsp_request* worker_channel;

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

vector<long> musecs;
vector<long> secs;

typedef struct start_stop {
    int start;
    int stop;
    int length;
} START_STOP;

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS */
/*--------------------------------------------------------------------------*/

void print_time_diff(struct timeval * tp1, struct timeval * tp2) {
    /* Prints to stdout the difference, in seconds and museconds, between two
    timevals. */
    long sec = tp2->tv_sec - tp1->tv_sec;
    long musec = tp2->tv_usec - tp1->tv_usec;
    if (musec < 0) {
        musec += 1000000;
        sec--;
    }
    cout<<" [sec = "<<sec<<", musec = "<<musec<<"] "<<endl;
    musecs.push_back(musec);
    secs.push_back(sec);
}

void update_indices(int pos, int int_array[], int limit) {
    int_array[pos]++;
    if (pos+1 == limit) {
        return;
    } else if (int_array[pos] == 26) {
            int_array[pos] = 0;
            update_indices(pos+1, int_array, limit);
    }
}

string run_sha1sum(const char * possible_password){
    const char * system_call_a = "echo -n ";
    const char * system_call_b = " | shasum | awk '{print $1}'";
    char system_call[100];
    strcpy(system_call, system_call_a);
    strcat(system_call, possible_password);
    strcat(system_call, system_call_b);
    
    //Run System Call and extract result from stdout
    char buf [256];
    FILE *p = popen(system_call, "r");
    string s;
    for (size_t count; (count = fread(buf, 1, sizeof(buf), p));)
        s += string(buf, buf + count);
    pclose(p);

    return s.substr(0,40);
}

void *cracker_minion(void * args) {
    START_STOP * range = (START_STOP *) args;

    int start = range->start;
    int stop = range->stop;
    int length = range->length;

    int *index;
    index = new int[length];

    string start_password;
    string last_password;

    for(int i=0; i<length; i++) {
        index[i] = start;
        start_password+=alphabet[start];
        last_password+=alphabet[stop];
    }
    DEBUG_MSG("Thread ID ["<<pthread_self()<<"]: "<<start_password);
    string result = run_sha1sum(start_password.c_str());
    if (result == request_hash.substr(0,40)) {
        stop_searching = true;
        answer = start_password;
    } else {
        while(start_password != last_password && stop_searching != true) {
            start_password = "";
            update_indices(0, index, length);
            for (int j=0; j<length; j++){
                start_password+=alphabet[index[j]];
            }
            DEBUG_MSG("Thread ID ["<<pthread_self()<<"]: "<<start_password);
            result = run_sha1sum(start_password.c_str());
            if (result == request_hash.substr(0,40)) {
                stop_searching = true;
                answer = start_password;
                break;
            }
        }
    }
    pthread_exit(NULL);
}

void wait_for_all_threads() {
    for (int i=0; i<number_of_worker_threads; i++){
        pthread_join(w_threads[i], NULL);
    }    
    DEBUG_MSG("***************All worker threads are done***************");
}

void password_cracker(int length, int start, int stop) {
    // Multithreaded algorithm for password cracking
    w_threads = new pthread_t [number_of_worker_threads];

    int *index;
    index = new int[number_of_worker_threads+1];
    index[0] = start;

    int breakdown = (stop+1-start)/number_of_worker_threads;
    int high = breakdown*number_of_worker_threads;
    for (int i = 1; i<(number_of_worker_threads+1); i++){
        index[i] = index[i-1]+breakdown;
    }
    int full = (stop+1-start)-high;
    if (full > 0) {
        int temp;
        for (int i = 0; i<full; i++){
            temp = (number_of_worker_threads+1)-i;
            index[temp]=index[temp]+(full-i);
        }
    }
    for (int i=0; i<(number_of_worker_threads+1); i++){
        DEBUG_MSG("index ["<<i<<"] = "<<index[i]);
    }

    int worker_thread;
    for (int i = 0; i<number_of_worker_threads; i++){
        START_STOP *range = new START_STOP;
        range->start = index[i];
        range->stop = index[i+1];
        range->length = length;
        worker_thread = pthread_create(&w_threads[i], NULL, cracker_minion, (void *)range);
        if (worker_thread){
            cout<<"ERROR: return code from pthread_create() is "<<worker_thread<<endl;
            exit(-1);
        }
    }
    wait_for_all_threads();
    //delete index;
    if (stop_searching == true) {
        cout<<"****************YAY FOUND PASSWORD****************"<<endl;
        cout<<"Hash : "<<request_hash<<endl;
        string found_string = "Found: ";
        answer = found_string+answer;
        cout<<answer<<endl;
    } else {
        cout<<"**************ALL THAT FOR NOTHING?!**************"<<endl;
        cout<<"Hash : "<<request_hash<<endl;
        if (last == true) {
            answer = "Not Found5";
        } else {
            answer = "Not Found";
        }
    }
    //lsp_request_write(worker_channel,result,result.length()); //UDP-LSP
    lsp_request_write(worker_channel,answer,answer.length()); //UDP-LSP
    request_hash = "";
    last = false;
    answer = "";
    stop_searching = false;
}

void send_join_request(){
    // send a join request to the server
    int msg_length = 4;
    string request_msg = "join";
    lsp_request_write(worker_channel,request_msg,msg_length); //UDP-LSP
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char **argv) {
    // Initialization

    const char * host_name = "localhost";
    unsigned short port_number = 7000;
    port_number++;

    number_of_worker_threads = 3;

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

    if (number_of_worker_threads > 25) {
        number_of_worker_threads = 25;
    }

    cout<<"-----------number of worker threads = "<<number_of_worker_threads<<endl;
    cout<<"----------------name of server host = "<<host_name<<endl;
    cout<<"---------port number of server host = "<<port_number<<endl;
    
    w_threads = new pthread_t [number_of_worker_threads];

    // Initialize threads:
    cout <<"Initializing Worker Channel...."<<endl;
    worker_channel = lsp_request_create(host_name,port_number);

    send_join_request();

    int start_char;
    int stop_char;
    int password_length;

    // ***********************************************************
    // Initialize Worker Loop
    string input;
    while(true) {
        int numRead = lsp_request_read(worker_channel,(void*) &input);
        if(numRead > 0) {
            cout<<"!!! Let's Crack This Password !!!"<<endl;
            cout<<"Input: "<<input<<endl;
            request_hash = input.substr(0,40);
            cout<<"Hash: "<<request_hash<<endl;
            string worker_position = input.substr(input.length()-1, input.length());
            cout <<"worker_position = "<<worker_position<<endl;
            input = input.substr(0,input.length()-1);
            password_length = atoi((input.substr(46,input.length()-46)).c_str());
            cout<<"Length: "<<password_length<<endl;
            if (worker_position == "1") {
                start_char = 0;
                stop_char = 8;
            } else if (worker_position == "2") {
                start_char = 8;
                stop_char = 16;
            } else if (worker_position == "3") {
                start_char = 16;
                stop_char = 25;
                last = true;
            } else if (worker_position == "4") {
                start_char = 15;
                stop_char = 20;
            } else if (worker_position == "5") {
                start_char = 20;
                stop_char = 25;
                last = true;
            } 
            //password_cracker(input);
            password_cracker(password_length, start_char, stop_char);
            // now that a password has been cracked, restart

            // TODO worker will close when the server closes it:
            // Write a function to handle a server closing the worker msg
            send_join_request();
        }
    }
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
