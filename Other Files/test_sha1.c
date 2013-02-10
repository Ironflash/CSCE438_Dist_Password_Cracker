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
#include <vector>
#include <assert.h>
#include <sys/time.h>

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

static int number_of_worker_threads;
static pthread_t *w_threads;

static string alphabet[26] = { "a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z" };

static string answer;
static bool stop_searching = false;

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
    const char * system_call_b = " | sha1sum | awk '{print $1}'";
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
    index[0] = 0;

    int breakdown = 26/number_of_worker_threads;
    int high = breakdown*number_of_worker_threads;
    for (int i = 1; i<(number_of_worker_threads+1); i++){
        index[i] = index[i-1]+breakdown;
    }
    int full = 26-high;
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
        cout<<"Found: "<<answer<<endl;
    } else {
        cout<<"**************ALL THAT FOR NOTHING?!**************"<<endl;
        cout<<"Hash : "<<request_hash<<endl;
        answer = "Not found";
    }
    //lsp_request_write(worker_channel,result,result.length()); //UDP-LSP
}

void gather_statistics(int password_length) {
    if (number_of_worker_threads > 25) {
        number_of_worker_threads = 25;
    }
    //temp if statement for testing
    if (password_length == 1) {
        // original password = t
        request_hash = "8efd86fb78a56a5145ed7739dcb00c78581c5375";
    } else if (password_length == 2) {
        // original password = te
        request_hash = "33e9505d12942e8259a3c96fb6f88ed325b95797";
    } else if (password_length == 3) {
        // original password = tes
        request_hash = "d1c056a983786a38ca76a05cda240c7b86d77136";
    } else if (password_length == 4) {
        // original password = test
        request_hash = "a94a8fe5ccb19ba61c4c0873d391e987982fbbd3";
    } else if (password_length == 5) {
        // original password = testi
        request_hash = "f1e1c6ea766397606475ab41d7f124258da887b9";
    } else if (password_length == 6) {
        // original password = testin
        request_hash = "cfa81791b86dd893086557134603dfa4c58fd977";
    } else if (password_length == 7) {
        // original password = testing
        request_hash = "dc724af18fbdd4e59189f5fe768a5f8311527050";
    } else {
        cout<<"ERROR"<<endl;
    }

    // ***********************************************************
    // Timer for performance metrics; Used to compute elapsed time.
    struct timeval tp_start;
    struct timeval tp_end;
    assert(gettimeofday(&tp_start, 0) == 0);
    // ***********************************************************

    int start_char = 0;
    int stop_char = 25;
    
    password_cracker(password_length, start_char, stop_char);

    // ***********************************************************
    // Timer for performance metrics; Used to compute elapsed time.
    assert(gettimeofday(&tp_end, 0) == 0);
    printf("Time taken for computation : "); 
    print_time_diff(&tp_start, &tp_end);
    // ***********************************************************
}

void print_out_statistics(int password_length) {
    cout<<"Password Length = "<<password_length<<endl;
    cout<<"1\t2\t3\t4\t5\t6\t7\t8\t9\t10\t11\t12\t13\t14\t15\t16\t17\t18\t19\t20"<<endl;
    for (int j=0; j<20; j++) {
        cout<<musecs[j]<<"\t";
    }
    cout<<endl;
    for (int j=0; j<20; j++) {
        cout<<secs[j]<<"\t";
    }
    cout<<endl;
    /*
    ofstream write_to1 ("example.txt");
    
    cout << "Prepare to write results to text file! =D"<<endl;
    
    if (write_to1.is_open()) {
        write_to1<<"Password Length = "<<password_length<<endl;
        write_to1<<"1\t2\t3\t4\t5\t6\t7\t8\t9\t10\t11\t12\t13\t14\t15\t16\t17\t18\t19\t20"<<endl;
        for (int j=1; j<21; j++) {
            write_to1<<musecs[j]<<"\t";
        }
        write_to1<<endl;
        for (int j=1; j<21; j++) {
            write_to1<<secs[j]<<"\t";
        }
        write_to1<<endl;

        cout<<endl<<"*******************************************"<<endl;
        write_to1.close();
    } else {
        cout << "Unable to open file"<<endl; 
    }
    //*/
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char **argv) {

    int password_length = 3;
    cin>>password_length;
    cin>>number_of_worker_threads;
    gather_statistics(password_length);

    /*
    // special case: password_length = 0 & number_of_worker_threads = 0
    if (password_length == 0 && number_of_worker_threads == 0) {
        cin>>password_length;
        for (int j=1; j<21; j++) {
            number_of_worker_threads = j;
            gather_statistics(password_length);
        }
        print_out_statistics(password_length);
    } else if (password_length > 0 && number_of_worker_threads > 0) {
        gather_statistics(password_length);
    } else {
        cout<<"Bad Input: Try again...."<<endl;
    }
    */

    return 0;
}
