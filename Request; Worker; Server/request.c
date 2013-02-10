/* 
    File: request.c

    Authors: Daniel Timothy S. Tan
             Department of Computer Science
             Texas A&M University
    Date   : 01/30/2013

    request client main program for HW2 in CSCE 438-500
*/

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include <iostream>
#include <assert.h>
#include <sys/time.h>
#include "request_lsp_api.c"

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

// Create Request Client-Server Communication channel
static struct lsp_request* request_channel;

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

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
}

void clean_exit(){
    cout<<"Closing Request Channel..."<<endl;
    lsp_request_close(request_channel); // UDP-LSP
    cout<<"Request client main completed successfully"<<endl;
    usleep(1000000);
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

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char **argv) {
    
    const char * host_name = "localhost";
    unsigned short port_number = 1234;

    // "test" = "a94a8fe5ccb19ba61c4c0873d391e987982fbbd3";

    string test = "te";
    string test_string = run_sha1sum(test.c_str());

    //string request_msg = "a94a8fe5ccb19ba61c4c0873d391e987982fbbd3";
    string request_msg = test_string;
    //request_msg = "a94a8fe5ccb19ba61c4c0873d391e987982fbbd3";
    int msg_length = test.length();

    // ***********************************************************
    // getopt code
    int index;
    int c = 0;
    cout<<"[-h <name of server host>]"<<endl;
    cout<<"[-p <port number of server host>]"<<endl;
    cout<<"[-r <hash signature request>]"<<endl;
    cout<<"[-l <original password length>]"<<endl;
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
    cout<<"---original password length = "<<msg_length<<endl;

    // Initialize Request Client-Server Communication Channel
    cout <<"Initializing Request Channel...."<<endl;
    
    request_channel = lsp_request_create(host_name, port_number); // UDP-LSP

    DEBUG_MSG("****************Perform Error Checking****************");

    //Error Checking:
    if (request_msg.length() != 40) {
        cout<<"The HASH SIGNATURE you inputted is NOT VALID"<<endl;
        return 0;
    }

    // ***********************************************************
    // Timer for performance metrics; Used to compute elapsed time.
    struct timeval tp_start;
    struct timeval tp_end;
    assert(gettimeofday(&tp_start, 0) == 0);
    // ***********************************************************

    lsp_request_write(request_channel,request_msg,msg_length);

    // ***********************************************************
    // Initialize Requester Loop
    string input;
    bool cracked = false;
    while(!cracked) {
        int numRead = lsp_request_read(request_channel,(void*) &input);
        if(numRead > 0) {
            // TODO: design a nice and clean output for both cases
            // cout Password cracked only when the returned input
            // was "Found: ..."
            if (input.substr(0,6) == "Found:") {
                cout<<"!!!!!! Password cracked !!!!!!"<<endl;
            }
            cout<<input<<endl;
            cracked = true;
        }
    }

    // ***********************************************************
    // Timer for performance metrics; Used to compute elapsed time.
    assert(gettimeofday(&tp_end, 0) == 0);
    printf("Time taken for computation : "); 
    print_time_diff(&tp_start, &tp_end);
    // ***********************************************************

	// ***********************************************************
    // Close the request client when done
    clean_exit();
    // ***********************************************************

    //*/
    return 0;
}
