/* 
    File: semaphore.C

    Author: Daniel Timothy S. Tan
            Department of Computer Science
            Texas A&M University
    Date  : 07/11/12

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include <cassert>
#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <errno.h>

#include "semaphore.h"

using namespace std;

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S e m a p h o r e  */
/*--------------------------------------------------------------------------*/

/* -- CONSTRUCTOR/DESTRUCTOR */
Semaphore::Semaphore(int _val) {
	this->value = _val;
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
}

Semaphore::~Semaphore() {
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
}

int Semaphore::P() {
	pthread_mutex_lock(&mutex);
	value--;
	if (value < 0) {
		pthread_cond_wait(&cond, &mutex);
	}
	pthread_mutex_unlock(&mutex);
	return 0;
}

int Semaphore::V() {
	pthread_mutex_lock(&mutex);
	value++;
	if (value <= 0) {
		pthread_cond_signal(&cond);
	}
	pthread_mutex_unlock(&mutex);
	return 0;
}

int Semaphore::return_value() {
	return value;
}
