#ifndef SIGNALHANDLERS_H_
#define SIGNALHANDLERS_H_

#include "libraries.h"
#include "Queue.h"
#include "worker.h"


/* signal handler for SIGINT in manager process */
void Inform_Manager(int );   /* when user types ^C*/

/* signal handler for SIGCHLD in manager process */
void Signal_from_Child(int);  /* when children (workers) send SIGCHLD (when they stop or continue)*/

/* signal handler for SIGTSTP in worker process */
void Finish_Worker(int );    /* manager sends SIGTSTP to workers to make them finish */ 

/* signal handler for SIGCHLD in manager process */
void Wait_Child(); /* when children (workers) send SIGCHLD (when they exit)*/


#endif