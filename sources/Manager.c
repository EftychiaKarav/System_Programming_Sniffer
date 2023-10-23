#include "../headers/libraries.h"
#include "../headers/Signalhandlers.h"
#include "../headers/worker.h"
#include "../headers/Manager.h"


Queue workers_queue = NULL;    
unsigned int Children = 0;    //total number of workers whether they are in the Queue or not
int listener_manager_pipe[2]; //listener writes, manager reads


/* implementation of listener */
void Listener(char* directory){
    
    printf("LISTENER STARTS WITH PID = %d AND PARENT PID = %d\n", getpid(), getppid());
    close(listener_manager_pipe[READ]);   //close the read end of the pipe for listener
    
    /* redirection of stdout to the write end of the pipe */
    dup2(listener_manager_pipe[WRITE], STDOUT_FILENO);
    
    if(directory == NULL){ /* cwd is being monitored*/
    execlp("inotifywait", "inotifywait", "-mq", "-e", "create", "-e", "moved_to", "--format", "%f", ".", (char*)NULL);
    }
    else{  /* the directory specified by argv[2] is being monitored*/
    execlp("inotifywait", "inotifywait", "-mq", "-e", "create", "-e", "moved_to", "--format", "%w%f", directory, (char*)NULL);
    }
}


/* implementation of manager */
void Manager(DIR* dir_ptr, char* dirname){

    pid_t pid_worker = -1;
    char* manager_worker_FIFO = NULL;
    int num_bytes = 0;
    int fifo_fd = -1;
    char inbuf[MAX_LENGTH] = {0};

    if(!Queue_Exists(workers_queue)){
        workers_queue = Queue_Initialize();
    }
    while(RUNNING){
        /* manager waits to read a message which listener sends him */
        while((num_bytes = read(listener_manager_pipe[0], inbuf, (size_t)MAX_LENGTH)) < 0){

            if(errno == EAGAIN)  //when manager gets ^C pipe's flag is set to O_NONBLOCK
                break;
            
            if((errno != EINTR) && (num_bytes < 0))
                perror("Manager reading from pipe");
            
            if(errno == EINTR) //syscall read interrupted by signal so we restart the syscall
                //perror("Read in pipe was interrupted by a signal\n");
                continue;
        }
        
        while(strlen(inbuf) > 0){   

            char* FileName_Path = NULL;            
            /* NO available workers*/
            if(Queue_isEmpty(workers_queue)){

                pid_worker = fork();
                if (pid_worker < 0){
                    perror ("Error in Fork");
                    exit(EXIT_FAILURE);
                }
                else if (pid_worker == 0){      //worker process
                    printf("NEW WORKER STARTS WITH PID = %d\n", getpid());                    
                    Worker();
                    Clean_Memory(false, dir_ptr, dirname, FileName_Path);
                    exit(EXIT_SUCCESS);
                } 
                else{  //manager process
                    Children++;

                    manager_worker_FIFO = Create_FIFO_Name(pid_worker, getpid());
                    manager_worker_FIFO = Create_FullPath_of_File(FIFO_DIR, manager_worker_FIFO);
                    if (mkfifo(manager_worker_FIFO, 0666) != 0 ){
                        if(errno == EEXIST)
                            printf("fifo already exists\n");
                        
                        if (errno != EEXIST){ 
                            perror("MANAGER mkfifo failed");
                            exit(6);
                        }
                    }
                }
            }
            /* available workers in the queue */
            else{  
                /* get the 1st worker in queue with his pid and fifo name */
                QNode popped_node = Queue_First(workers_queue);
                pid_worker = QueueNode_GetPid(popped_node);
                char* FIFO = QueueNode_GetFIFO_Name(popped_node);
                manager_worker_FIFO = (char*)calloc(strlen(FIFO)+1, sizeof(char));
                memcpy(manager_worker_FIFO, FIFO, strlen(FIFO));
                /* remove worker from queue */
                Queue_Delete(workers_queue, popped_node);
            
                /* manager signals to worker to continue */
                kill(pid_worker, SIGCONT);  

            }

            //printf("MANAGER FROM LISTENER GETS: %s\n", inbuf);
            /* get the filename along with its path if needed, which manager
                has read from his communication with listener */
            num_bytes = strlen(inbuf);
            FileName_Path = Extract_FileNamePath(inbuf, num_bytes);
            // if(dir_ptr != NULL){
            //     FileName_Path = Create_FullPath_of_File(dirname, FileName_Path);
            // }
            //printf("MESSAGE TO OPEN %s\n", FileName_Path);


            /* manager writes the name of the file to the FIFO */
            while ( (fifo_fd = open(manager_worker_FIFO , O_WRONLY)) < 0){ 
                if(errno == EINTR)
                    continue;
                perror ("Manager open FIFO"); 
            }
            //kill(pid_worker, SIGCONT);   
//else{
            int num_bytes_write;
            while (( num_bytes_write = write (fifo_fd , FileName_Path , strlen(FileName_Path))) < 0){ 
                
                if(errno == EINTR)
                    /* Manager writes was interrupted by a signal */
                    continue;
                else
                    perror ("Manager while Writing FIFO error");
                //exit (2);
            }
            /* close fifo file descriptor */
            if(close(fifo_fd) == -1){

                if(errno == EBADF) 
                    perror("Bad file descriptor\n");
                if(errno == EINTR)  
                    perror("Close at manager was interrupted by signal\n");
                
                perror("Close at manager");
            }  
            free(FileName_Path); 
            free(manager_worker_FIFO);
        }
        memset(inbuf, '\0', MAX_LENGTH);
    }
    //}
}


/* send to all workers signal to finish */
void Terminate_Workers(void ){
    
    if(Queue_Exists(workers_queue)){
        
        printf("Manager notifies workers to terminate\n");

        while(Children > 0){
            // printf("Queue size at terminate worker is %d\n", Queue_Size(workers_queue));
            QNode process = Queue_First(workers_queue);
            while(process != NULL){
                pid_t worker_pid = QueueNode_GetPid(process);
                // printf("worker pid from term %d\n", worker_pid);
                kill(worker_pid, SIGCONT);  //they are stopped 
                kill(worker_pid, SIGTSTP);  // they change RUNNING to false 
                process = QueueNode_Next(process);
            }
        }
    }
    return;
}

void Clean_Memory(bool parent, DIR* dir_ptr, char* directory, char* FileName){

    /* close the directory if specified in command line */
    if(dir_ptr != NULL){
        if(closedir(dir_ptr) < 0 ){
            fprintf(stdout, "Cannot close open %s directory\n", directory);
            exit(EXIT_FAILURE);
        }
    }

    /* close the read end of the pipe */
    if(close(listener_manager_pipe[READ]) == -1){
        if(errno == EBADF){
            perror("Bad file descriptor\n");
        }
        if(errno == EINTR){
            perror("Close at worker was interrupted by signal\n");
        }
        perror("Close Manager - Listener Pipe");
    }

    /* destroy the workers queue*/
    if(Queue_Exists(workers_queue)){
        Queue_Destroy(workers_queue);
    }

    if (parent){   
        /* delete the directory which had the FIFOs */
        if(rmdir(FIFO_DIR) == -1)
            perror("Could not delete directory\n");
    }
    else{  /* child -> frees memory which it has inhereted from parent */
        free(FileName);
        close(STDOUT_FILENO);
        close(STDIN_FILENO);
        close(STDERR_FILENO);
    }
    return;
}
