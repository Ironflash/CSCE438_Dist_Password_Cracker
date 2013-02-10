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
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
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

void clean_exit(){
    cout<<"Closing Request Channel..."<<endl;
    lsp_request_close(request_channel); // UDP-LSP
    cout<<"Request client main completed successfully"<<endl;
    usleep(1000000);
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char **argv) {

    string request_msg = "a94a8fe5ccb19ba61c4c0873d391e987982fbbd3";

    char buf [256];
    
    system("echo -n test | shasum | awk '{print $1}'");


    FILE *p = popen("echo -n test | shasum | awk '{print $1}'", "r");
    //FILE *p = popen("echo foo", "r");
    string s;
    size_t c = fread(buf, 1, sizeof(buf), p);
    //cout<<"size = "<<c<<endl;
    s += string(buf, buf + c);
    cout<<"s = "<<s<<endl;

    for (size_t count; (count = fread(buf, 1, sizeof(buf), p));)
        s += string(buf, buf + count);
    pclose(p);
    //*/

    /*
    FILE *p = popen("echo hello", "r");
    sprintf(buf, "%d", fileno(p));
    ifstream p2(buf);
    string s;
    p2 >> s;
    p2.close();
    pclose(p);
    */

    /*
    stringstream buffer;
    streambuf * old = cout.rdbuf(buffer.rdbuf());
    //std::cout << "Bla" << std::endl;
    system("echo foo");
    string text = buffer.str(); // text will now contain "Bla\n"
    */

    cout<<endl<<"Found: "<<s<<endl;
    // compare:
    if (s == request_msg) {
        //cout<<"THEY ARE THE SAME! =D"<<endl;
    } else {
        //cout<<"they are not the same =("<<endl;
    }

    //cout<<"return"<<ret<<endl;
    //fprintf(stdout,"system ret:[%d]\n",ret>>8);
    //system("echo -n test | shasum");
    return 0;
    
    /*
    const char * host_name = "localhost";
    unsigned short port_number = 1234;

    // "test" = "a94a8fe5ccb19ba61c4c0873d391e987982fbbd3";
    string request_msg = "a94a8fe5ccb19ba61c4c0873d391e987982fbbd3";
    int msg_length = 4;

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
            cout<<"!!!!!! Password cracked !!!!!!"<<endl;
            cout<<input<<endl;
            cracked = true;
        }
    }
    // ***********************************************************

	// ***********************************************************
    // Close the request client when done
    clean_exit();
    // ***********************************************************

    return 0;
    */
}
