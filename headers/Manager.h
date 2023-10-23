#ifndef MANAGER_H_
#define MANAGER_H_

#include "libraries.h"
#include "Queue.h"


/* listener process */
void Listener(char* );  


/* manager process */
void Manager(DIR*, char*);


/* memory clean up*/
void Clean_Memory(bool, DIR*, char*, char*);


/* manager sends SIGINT to children to notify them to finish and exit*/
void Terminate_Workers();    


#endif