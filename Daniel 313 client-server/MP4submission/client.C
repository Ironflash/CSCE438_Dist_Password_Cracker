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
#include <signal.h>
#include <errno.h>

#include "reqchannel.H"
#include "semaphore.H"

using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/
static int number_of_requesters;
static int number_of_data_requests;
static int number_of_worker_threads;

static pthread_t *r_threads;
static pthread_t e_thread;
static pthread_t *s_threads;

static int *r_fd; // read file descriptor array
static int *w_fd; // write file descriptor array
static RequestChannel **channel_pointer; // request channel pointers array

static Semaphore lock_hist1 (1);
static Semaphore lock_hist2 (1);
static Semaphore lock_hist3 (1);
static int histogram_1[10];
static int histogram_2[10];
static int histogram_3[10];

static bool stop_display = false;

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
    int get_size(); // used to find the # of remaining requests before calling a quit
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
int buffer<T>::get_size(){
    pthread_mutex_lock(&mutex_lock);
    return bounded_buffer.size();
    pthread_mutex_unlock(&mutex_lock);
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

void *event_handler_thread(void * arguments) {
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
                */
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
            cout <<"Terminate worker thread"<<endl;
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
    cout<<"Ending stat thread"<<endl;
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
    REQUEST_NODE *request = new REQUEST_NODE;
    request->requester_id = 0;
    request->request_name = "quit";
    bounded_buffer->insert(request);
    pthread_join(e_thread, NULL);
    cout<<"***************Event handler thread is done***************"<<endl;
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
    //*/
}

void print_histograms(){
    lock_hist1.P();
    lock_hist2.P();
    lock_hist3.P();
    cout<<"**************Joe Smith's Histogram**************"<<endl;
    for (int i=0; i<10; i++) {
        cout << "("<<10*i<<"-"<<((10*i)+10)<<"); \t"<<histogram_1[i]<<"\t[";
        for (int j=0; j < histogram_1[i]; j++){
            cout<<":";
        }
        cout<<"]"<<endl;
    }
    cout<<"**************Jane Smith's Histogram*************"<<endl;
    for (int i=0; i<10; i++) {
        cout << "("<<10*i<<"-"<<((10*i)+10)<<"); \t"<<histogram_2[i]<<"\t[";
        for (int j=0; j < histogram_2[i]; j++){
            cout<<":";
        }
        cout<<"]"<<endl;
    }
    cout<<"**************John Doe's Histogram***************"<<endl;
    for (int i=0; i<10; i++) {
        cout << "("<<10*i<<"-"<<((10*i)+10)<<"); \t"<<histogram_3[i]<<"\t[";
        for (int j=0; j < histogram_3[i]; j++){
            cout<<":";
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

// SIGNAL HANDLING:
void timer_handler(int s) {
    signal(SIGALRM, SIG_IGN);
    if (stop_display == false) {
        print_histograms();
    }
    signal(SIGALRM, timer_handler);
}

int setupinterrupt() { // set up myhandler for SIGPROF
    struct sigaction act;
    act.sa_handler = timer_handler;
    act.sa_flags = 0;
    return (sigemptyset(&act.sa_mask) || sigaction(SIGALRM, &act, NULL));
}

int setupitimer() { // set ITIMER_REAL for 2-second intervals
    struct itimerval value;
    value.it_interval.tv_sec = 1;
    value.it_interval.tv_usec = 0;
    value.it_value = value.it_interval;
    return (setitimer(ITIMER_REAL, &value, NULL));
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char **argv) {
    
    int max_buffer_size = 5;
    number_of_requesters = 3;
    number_of_data_requests = 1;
    number_of_worker_threads = 3;
    
    int index;
    int c = 0;
    cout<<"[-n <number of data requests per person>]"<<endl;
    cout<<"[-b <size of bounded buffer in requests>]"<<endl;
    cout<<"[-w <number of worker threads>]"<<endl;
    while ((c = getopt (argc, argv, "n:b:w:")) != -1) {
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
            case '?':
                if (optopt == 'n') {
                    cout<<"ERROR: Option -n requires an argument"<<endl;
                } else if (isprint (optopt)) {
                    cout<<"ERROR: Unknown option for -n"<<endl;
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
                if (optopt == 'w') {
                    cout<<"ERROR: Option -w requires an argument"<<endl;
                } else if (isprint (optopt)) {
                    cout<<"ERROR: Unknown option for -w"<<endl;
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

    // timer for performance metrics
    struct timeval tp_start; // Used to compute elapsed time.
    struct timeval tp_end;
    assert(gettimeofday(&tp_start, 0) == 0);
    
    int status;
    pid_t pid;
    r_threads = new pthread_t [number_of_requesters];
    s_threads = new pthread_t [number_of_requesters];
    r_fd = new int[number_of_worker_threads];
    w_fd = new int[number_of_worker_threads];
    channel_pointer = new RequestChannel*[number_of_worker_threads];

    if ((pid = fork()) < 0) { //create child
        cout<<"ERROR: forking failed"<<endl;
        exit(1);
    }
    cout<<"pid = "<<pid<<endl;
    if (pid == 0) { // child process
        cout<<"Running child process"<<endl;
        cout<<"Starting Server Side"<<endl;
        execvp("./dataserver",argv);
    } else { // parent process
        cout << "CLIENT STARTED:" << endl;
        cout << "Establishing control channel... " << endl;
        RequestChannel * chan = new RequestChannel ("control", RequestChannel::CLIENT_SIDE);
        cout << "done." << endl;

        // Initialize threads:
        cout <<"Starting threads"<<endl;
        bounded_buffer = new buffer<REQUEST_NODE>(max_buffer_size);
        deposit = new buffer<STATISTIC_NODE>(max_buffer_size);
        create_request_threads();

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

        // start periodically printing histograms
        
        if (setupinterrupt() == -1) {
            perror("Failed to set up handler for SIGALRM");
            return 1;
        }
        if (setupitimer() == -1) {
            perror("Failed to set up the ITIMER_REAL interval timer");
            return 1;
        } //*/
        create_statistics_threads();

        wait_for_all_threads();
        cout<<"Main completed successfully"<<endl;
        chan->send_request("quit");
        usleep(1000000);
    }
    stop_display = true;    
    print_histograms();
    assert(gettimeofday(&tp_end, 0) == 0);
    printf("Time taken for computation : "); 
    print_time_diff(&tp_start, &tp_end);
    printf("\n");
    clean_up_fifo();
    atexit(clean_up_fifo);
    return 0;
    //*/
}
