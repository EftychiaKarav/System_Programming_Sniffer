#include "../headers/Queue.h"

struct Queue_Node{
    
    char* Name;    //either name of the FIFO for the worker, or the name of the link location for the links
    unsigned int Number; //either pid for worker or number of occurences for a link
    QNode next;
};


/**************************************************************************************************/

// For workers

QNode QueueNode_Create_worker(unsigned int pid, unsigned int parent_pid){

    QNode queue_node = malloc(sizeof(struct Queue_Node));
    if (queue_node == NULL){
        return NULL;    //se error kati na valo
    }
    queue_node->Name = Create_FIFO_Name(pid, parent_pid);
    queue_node->Number = pid;
    queue_node->Name = Create_FullPath_of_File(FIFO_DIR, queue_node->Name);
    queue_node->next = NULL;

    //free(temp_name);
    return queue_node;
}

unsigned int QueueNode_GetPid(QNode queue_node){
    return queue_node->Number;
}

char* QueueNode_GetFIFO_Name(QNode q_node){
    return q_node->Name;
}

QNode QueueNode_Next(QNode q_node){
    return q_node->next;
}


/**************************************************************************************************/

//For links

QNode QueueNode_Create_Link(char* LinkName){
    
    QNode queue_node = malloc(sizeof(struct Queue_Node));
    if (queue_node == NULL){
        return NULL;    //se error kati na valo
    }
    queue_node->Number = 0;
    queue_node->Name = (char*)calloc(strlen(LinkName)+1, sizeof(char));
    strcpy(queue_node->Name, LinkName);
    queue_node->next = NULL;
    //printf("%s, %ld\n", queue_node->FIFO_name, strlen(queue_node->FIFO_name));

    //free(LinkName);
    return queue_node;

}

void QueueNode_Increase_Appearances(QNode queue_node){
    queue_node->Number++;
}

unsigned int QueueNode_Get_Appearances(QNode queue_node){
    return queue_node->Number;
}

char* QueueNode_GetLink_Name(QNode q_node){
    return q_node->Name;
}



/**************************************************************************************************/

//   Queue Methods

struct Queue{
    QNode first;
    QNode last;
    unsigned int size;
};


Queue Queue_Initialize(){

    Queue queue = malloc(sizeof(struct Queue));
    if (queue == NULL){
        return NULL;    //se error kati na valo
    }
    queue->first = NULL;
    queue->last = NULL;
    queue->size = 0;

    return queue;
}

bool Queue_Exists(const Queue queue){
    return (queue != NULL) ? true : false; 
}

bool Queue_isEmpty(const Queue queue){
    
    return (queue->size == 0) ? true : false;
}


void Queue_Insert(Queue queue, QNode queue_node){

    if (Queue_isEmpty(queue)){
        queue->first = queue_node;
    }
    else{
        queue->last->next = queue_node;
    }
    queue->last = queue_node;
    queue->size++;

}


void Queue_Destroy(Queue queue){

    while (queue->first != NULL && queue->size != 0){
        Queue_Delete(queue, queue->first);
    }
    free(queue);

}

void Queue_Delete(Queue queue, QNode queue_node){

    queue->first = queue->first->next;
    queue->size--;
    // if (queue->size == 1){
    //     queue->last = queue->first;
    // }
    free(queue_node->Name);
    free(queue_node);
    
}


QNode Queue_Pop(Queue queue){

    if(Queue_isEmpty(queue)){
        return NULL;
    }
    QNode node_to_be_popped = queue->first;
    queue->first = queue->first->next;
    queue->size--;
    // if (queue->size == 1){
    //     queue->last = queue->first;
    // }
    return node_to_be_popped;
    
}

QNode Queue_Find(Queue queue, unsigned int pid){

    QNode q_node = queue->first;
    while(q_node != NULL){
        if(q_node->Number == pid){
            break;
        }
        q_node = q_node->next;
    }
    return q_node;
}

void Queue_Print(const Queue queue){

    QNode q_node = queue->first;
    int i = 1;
    while(q_node != NULL){
        printf("i = %d\t, pid = %d\t, fifo = %s\n", i, q_node->Number, q_node->Name);
        q_node = q_node->next;
    }
    printf("Queue Size = %d\n", queue->size);
}


QNode Queue_First(Queue queue){
    return queue->first;
}

QNode Queue_Last(Queue queue){
    return queue->last;
}


unsigned int Queue_Size(Queue queue){
    return queue->size;
}

QNode Queue_Find_NameLocation(Queue queue, char* Location_Name){

    QNode q_node = queue->first;
    while(q_node != NULL){
        if( !strcmp(q_node->Name, Location_Name) ){
            break;
        }
        q_node = q_node->next;
    }
    return q_node;
}