/* 
    File: client.C

    Author: Daniel Timothy S. Tan
            Department of Computer Science
            Texas A&M University
    Date  : 07/17/2012

    client main program for MP3 in CSCE 313
*/

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include <cassert>
#include <iostream>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>

#include "netreqchannel.H"
#include "semaphore.H"

using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/
static int number_of_requesters;
static int number_of_data_requests;
static int number_of_worker_threads;

static pthread_t *r_threads;
static pthread_t *w_threads;
static pthread_t *s_threads;

static Semaphore lock_hist1 (1);
static Semaphore lock_hist2 (1);
static Semaphore lock_hist3 (1);
static int histogram_1[10];
static int histogram_2[10];
static int histogram_3[10];

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/
typedef struct request_node {
    int requester_id;
    string request_name;
} REQUEST_NODE;

typedef struct statistic_node {
    int requester_id;
    int statistic_value;
} STATISTIC_NODE;

template <class T>
class buffer {
private:
    vector<T*> bounded_buffer;
    Semaphore* bbuffer_empty;
    Semaphore* bbuffer_full;
    pthread_mutex_t mutex_lock;
public:
    buffer(int size);
    ~buffer();
    void insert(T *to_insert);
    T * remove();
    void print_request_node_buffer(); // testing function just to check the state of the buffer
    void print_stat_node_buffer(); // testing function just to check the state of the buffer
};

template <class T>
buffer<T>::buffer (int size) {
    pthread_mutex_init(&mutex_lock, NULL);
    bbuffer_empty = new Semaphore(size);
    bbuffer_full = new Semaphore(0);
}

template <class T>
buffer<T>::~buffer() {
    pthread_mutex_destroy(&mutex_lock);
    delete bbuffer_empty;
    delete bbuffer_full;
}

template <class T>
void buffer<T>::insert(T *to_insert){
    bbuffer_empty->P();
    pthread_mutex_lock(&mutex_lock);
    bounded_buffer.push_back(to_insert);
    pthread_mutex_unlock(&mutex_lock);
    bbuffer_full->V();
}

template <class T>
T * buffer<T>::remove(){
    bbuffer_full->P();
    pthread_mutex_lock(&mutex_lock);
    T * removed_request = new T;
    removed_request = bounded_buffer.front();
    bounded_buffer.erase(bounded_buffer.begin());
    pthread_mutex_unlock(&mutex_lock);
    bbuffer_empty->V();
    return removed_request;
}

template <class T>
void buffer<T>::print_request_node_buffer(){
    pthread_mutex_lock(&mutex_lock);
    cout<<"********************"<<endl;
    for (int i=0; i<bounded_buffer.size(); i++) {
        cout<<"Vector ["<<i<<"] = "<<(bounded_buffer[i])->request_name<<endl;
    }
    cout<<"********************"<<endl;
    pthread_mutex_unlock(&mutex_lock);
}

template <class T>
void buffer<T>::print_stat_node_buffer(){
    pthread_mutex_lock(&mutex_lock);
    cout<<"********************"<<endl;
    for (int i=0; i<bounded_buffer.size(); i++) {
        cout<<"Vector ["<<i<<"] = "<<(bounded_buffer[i])->statistic_value<<endl;
    }
    cout<<"********************"<<endl;
    pthread_mutex_unlock(&mutex_lock);
}

/*--------------------------------------------------------------------------*/
/* CLASS CONSTANTS */
/*--------------------------------------------------------------------------*/
// Global Constant for the bounded buffers
static buffer<REQUEST_NODE> * bounded_buffer;
static buffer<STATISTIC_NODE> * deposit;

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
    printf(" [sec = %ld, musec = %ld] ", sec, musec);
}

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

void *worker_thread(void * arguments) {
    //cout<<"**************************Thread ID ["<<pthread_self()<<"]"<<endl;
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
}

void *statistics_thread(void * arguments){
    while(true) {
        STATISTIC_NODE * removed_statistic = new STATISTIC_NODE;
        removed_statistic = deposit->remove();

        //temporary random number response generator:
        int requester = removed_statistic->requester_id;
        int stat_value = removed_statistic->statistic_value;
        int bin_number;
        if (stat_value == -1) {
            //cout <<"Terminate statistic thread"<<endl;
            break;
        }

        if (stat_value >= 0 && stat_value <= 9) {
            bin_number = 0;
        } else if (stat_value >= 10 && stat_value <= 19) {
            bin_number = 1;
        } else if (stat_value >= 20 && stat_value <= 29) {
            bin_number = 2;
        } else if (stat_value >= 30 && stat_value <= 39) {
            bin_number = 3;
        } else if (stat_value >= 40 && stat_value <= 49) {
            bin_number = 4;
        } else if (stat_value >= 50 && stat_value <= 59) {
            bin_number = 5;
        } else if (stat_value >= 60 && stat_value <= 69) {
            bin_number = 6;
        } else if (stat_value >= 70 && stat_value <= 79) {
            bin_number = 7;
        } else if (stat_value >= 80 && stat_value <= 89) {
            bin_number = 8;
        } else if (stat_value >= 90 && stat_value <= 100) {
            bin_number = 9;
        }

        if (requester == 0) {
            lock_hist1.P();
            histogram_1[bin_number] ++;
            lock_hist1.V();
        } else if (requester == 1) {
            lock_hist2.P();
            histogram_2[bin_number] ++;
            lock_hist2.V();
        } else if (requester == 2) {
            lock_hist3.P();
            histogram_3[bin_number] ++;
            lock_hist3.V();
        }
    }
    //cout<<"Ending stat thread"<<endl;
    pthread_exit(NULL);
}

void create_statistics_threads(){
    int new_thread;
    // create a child thread for each requesters statistic
    for (int i = 0; i<number_of_requesters; i++){
        new_thread = pthread_create(&s_threads[i], NULL, statistics_thread, NULL);
        if (new_thread){
            cout<<"ERROR: return code from pthread_create() is "<<new_thread<<endl;
            exit(-1);
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

void print_histograms(){
    lock_hist1.P();
    lock_hist2.P();
    lock_hist3.P();
    cout<<"**************Joe Smith's Histogram**************"<<endl;
    for (int i=0; i<10; i++) {
        cout << "("<<10*i<<"-"<<((10*i)+10)<<"); "<<histogram_1[i]<<" [";
        for (int j=0; j < histogram_1[i]; j++){
            //cout<<":";
        }
        cout<<"]"<<endl;
    }
    cout<<"**************Jane Smith's Histogram*************"<<endl;
    for (int i=0; i<10; i++) {
        cout << "("<<10*i<<"-"<<((10*i)+10)<<"); "<<histogram_2[i]<<" [";
        for (int j=0; j < histogram_2[i]; j++){
            //cout<<":";
        }
        cout<<"]"<<endl;
    }
    cout<<"**************John Doe's Histogram***************"<<endl;
    for (int i=0; i<10; i++) {
        cout << "("<<10*i<<"-"<<((10*i)+10)<<"); "<<histogram_3[i]<<" [";
        for (int j=0; j < histogram_3[i]; j++){
            //cout<<":";
        }
        cout<<"]"<<endl;
    }
    lock_hist1.V();
    lock_hist2.V();
    lock_hist3.V();
}

void clean_up_fifo() {
    system("rm -f fifo*");
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char **argv) {
    int max_buffer_size = 5;
    number_of_requesters = 3;
    number_of_data_requests = 10;
    number_of_worker_threads = 2;

    string host_name = "localhost";
    unsigned short port_number = 7000;
    
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

    cout<<"------------number of data requests = "<<number_of_data_requests<<endl;
    cout<<"-size of bounded buffer in requests = "<<max_buffer_size<<endl;
    cout<<"-----------number of worker threads = "<<number_of_worker_threads<<endl;
    cout<<"----------------name of server host = "<<host_name<<endl;
    cout<<"---------port number of server host = "<<port_number<<endl;

    // timer for performance metrics
    struct timeval tp_start; /* Used to compute elapsed time. */
    struct timeval tp_end;
    assert(gettimeofday(&tp_start, 0) == 0);
    
    int status;
    pid_t pid;
    r_threads = new pthread_t [number_of_requesters];
    w_threads = new pthread_t [number_of_worker_threads];
    s_threads = new pthread_t [number_of_requesters];

    // Initialize threads:
    cout <<"Starting threads"<<endl;
    bounded_buffer = new buffer<REQUEST_NODE>(max_buffer_size);
    deposit = new buffer<STATISTIC_NODE>(max_buffer_size);

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
