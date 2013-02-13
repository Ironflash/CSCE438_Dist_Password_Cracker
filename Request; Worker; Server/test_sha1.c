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

using namespace std;

#if defined(__APPLE__) && defined(__MACH__)
    #include <CommonCrypto/CommonDigest.h>
    #define SHA1 CC_SHA1
    #define SHA_DIGEST_LENGTH 20
#else
    #include <openssl/sha.h>
#endif

//#define DEBUG // uncomment to turn on print outs
#ifdef DEBUG
#define DEBUG_MSG(str) do { std::cout << str << std::endl; } while( false )
#else
#define DEBUG_MSG(str) do { } while ( false )
#endif

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

string run_sha1sum(string possible_password){
    char * s1 = (char*)possible_password.c_str();

    unsigned char temp[SHA_DIGEST_LENGTH];
    char buf[SHA_DIGEST_LENGTH*2];
 
    memset(buf, 0x0, SHA_DIGEST_LENGTH*2);
    memset(temp, 0x0, SHA_DIGEST_LENGTH);

    char * data[] = { s1 };

    SHA1((unsigned char *)data[0], strlen(data[0]), temp);

    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf((char*)&(buf[i*2]), "%02x", temp[i]);
    }

    string s = buf;
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
    string result = run_sha1sum(start_password);
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
            result = run_sha1sum(start_password);
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
    request_hash = "";
    last = false;
    answer = "";
    stop_searching = false;
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char **argv) {
    int password_length;

    cin>>password_length;
    cin>>number_of_worker_threads;

    if (number_of_worker_threads > 25) {
        number_of_worker_threads = 25;
    }
    //worst case testing
    if (password_length == 1) {
        // original password = z
        //request_hash = "8efd86fb78a56a5145ed7739dcb00c78581c5375"; //t
        request_hash = "395df8f7c51f007019cb30201c49e884b46b92fa";
    } else if (password_length == 2) {
        // original password = zz
        //request_hash = "33e9505d12942e8259a3c96fb6f88ed325b95797"; //te
        request_hash = "d7dacae2c968388960bf8970080a980ed5c5dcb7";
    } else if (password_length == 3) {
        // original password = zzz
        //request_hash = "d1c056a983786a38ca76a05cda240c7b86d77136"; //tes
        request_hash = "40fa37ec00c761c7dbb6ebdee6d4a260b922f5f4";
    } else if (password_length == 4) {
        // original password = zzzz
        //request_hash = "a94a8fe5ccb19ba61c4c0873d391e987982fbbd3"; //test
        request_hash = "cb990257247b592eaaed54b84b32d96b7904fd95";
    } else if (password_length == 5) {
        // original password = zzzzz
        //request_hash = "f1e1c6ea766397606475ab41d7f124258da887b9"; //testi
        request_hash = "a2b7caddbc353bd7d7ace2067b8c4e34db2097a3";
    } else if (password_length == 6) {
        // original password = zzzzzz
        //request_hash = "cfa81791b86dd893086557134603dfa4c58fd977"; //testin
        request_hash = "984ff6ee7c78078d4cb1ca08255303fb8741d986";
    } else if (password_length == 7) {
        // original password = zzzzzzz
        //request_hash = "dc724af18fbdd4e59189f5fe768a5f8311527050"; //testing
        request_hash = "30986c81059b460882d5416484691d178c367ff6";
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

    //*/
    return 0;
}
