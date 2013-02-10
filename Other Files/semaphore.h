/* 
    File: semaphore.H

    Author: Daniel Timothy S. Tan
            Department of Computer Science
            Texas A&M University
    Date  : 07/11/12

*/

#ifndef _semaphore_H_                   // include file only once
#define _semaphore_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include <pthread.h>

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CLASS   S e m a p h o r e  */
/*--------------------------------------------------------------------------*/

class Semaphore {
private:
  /* -- INTERNAL DATA STRUCTURES
  You may need to change them to fit your implementation. */

  int value;
  pthread_mutex_t mutex;
  pthread_cond_t  cond;

public:
  /* -- CONSTRUCTOR/DESTRUCTOR */

  Semaphore(int _val);
  ~Semaphore();

  /* -- SEMAPHORE OPERATIONS */

  int P();
  int V();
  int return_value();

};

#endif
