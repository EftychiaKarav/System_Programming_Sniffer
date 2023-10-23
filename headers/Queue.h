#ifndef QUEUE_H_
#define QUEUE_H_

#include "libraries.h"

/**************************************************************************************************/

typedef struct Queue_Node*       QNode;
typedef struct Queue*            Queue;

/**************************************************************************************************/

//  methods for Datatype Queue_Node -> a QNode in our case is either a link or a worker 
QNode QueueNode_Create_worker(unsigned int, unsigned int);
QNode QueueNode_Create_Link(char*);
unsigned int QueueNode_GetPid(QNode);
char* QueueNode_GetFIFO_Name(QNode);
void QueueNode_Increase_Appearances(QNode);
unsigned int QueueNode_Get_Appearances(QNode);
char* QueueNode_GetLink_Name(QNode);
QNode QueueNode_Next(QNode);



//  methods for Queue
Queue Queue_Initialize();
bool Queue_isEmpty(const Queue);
bool Queue_Exists(const Queue);
QNode Queue_Pop(Queue);
QNode Queue_First(Queue);
QNode Queue_Last(Queue);  
unsigned int Queue_Size(Queue);
void Queue_Destroy(Queue);  
void Queue_Insert(Queue, QNode);
void Queue_Delete(Queue, QNode);
QNode Queue_Find(Queue, unsigned int);
void Queue_Print(const Queue);
QNode Queue_Find_NameLocation(Queue, char*);

#endif