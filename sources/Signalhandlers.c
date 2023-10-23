#include "../headers/Signalhandlers.h"


/**************************************** FOR MANAGER *****************************************/

/* signal handler for SIGCHLD when children (workers) stop or continue) */
void Signal_from_Child(int signum){
    
    pid_t pid;
    int status = -1;
    //printf("signalhandler for sigchild\n");
    while((pid = waitpid((pid_t)-1, &status, WUNTRACED | WCONTINUED)) <= 0){   
        perror("signal from child: Waiting child");
        if(errno == EINTR){
            perror("Waitpid was interrupted by a signal\n");
        }
    }
    /* if a worker stops insert him to the workers_queue */
    if(WIFSTOPPED(status)){
        if(WSTOPSIG(status) == SIGSTOP){
            printf("Worker with pid = %d stopped\n\n", pid); 
            QNode process = QueueNode_Create_worker(pid, getpid());
            Queue_Insert(workers_queue, process);
            //Queue_Print(workers_queue);
        }
    }
    
    return;
   //printf("manager got the signal\n");
}



/* signal handler for SIGINT when user types ^C */
void Inform_Manager(int signum){
    
    fprintf(stdout, "\nMANAGER WITH PID = %d GETS ^C\n", getpid());
    pid_t pid = -1;
    /* manager waits for all workers to be in a stopped situation before he resumes*/
    if(Queue_Exists(workers_queue)){
        while(Children != Queue_Size(workers_queue)){
            printf("waiting for children to stop\n");
            if((pid = waitpid(0, 0, WNOHANG | WUNTRACED )) < 0){
                if(errno == ECHILD){
                    perror("Child process does not exist\n");
                }
            }
        }
        printf("TOTAL WORKERS  --> %d\n", Queue_Size(workers_queue));
        printf("All workers have stopped\n");
    }
    /* manager unblocks from read with listener when the flag for the pipe becomes O_NONBLOCK*/
	if ((fcntl(listener_manager_pipe[READ],F_GETFL)) == -1){
		perror("Could not get the flags for manager - listener pipe\n");
		exit(2);
	}
    int non_block = fcntl(listener_manager_pipe[READ], F_SETFL, O_NONBLOCK); 
    if(non_block != 0){
        printf("Failed to unblock the pipe\n");
    }
    RUNNING = false;   //break the loop
    
    return;
}



/* signal handler for SIGCHLD children (workers) exit */
void Wait_Child(){
    
    pid_t pid;
    int status = -1;
    if(Children + 1 != 0){   /* children are workers + 1 for listener */
        while((pid = waitpid(-1, &status, WNOHANG)) < 0){
            if(errno == EINTR){
                printf("Interrupted by signal\n");
            }
            else if(errno == ECHILD){
                printf("Child process does not exist\n");
            }
        }
        if(WIFEXITED(status)){    /* workers exit normally */
            printf("WORKER %d ---------- exits\n", pid);
            Children--;
        }
        else if(WIFSIGNALED(status)){  /* manager terminates listener */
            printf("LISTENER %d ---------- terminates\n", pid);
            Children--;
        }
    }
    /* all children have exited -- manager can finish */
    if(Children + 1 == 0){
        RUNNING = false;
    }
    return;
}


/**************************************** FOR WORKERS *****************************************/

void Finish_Worker(int signum){
    /*child (worker) receives SIGTSTP from manager */
    RUNNING = false;
    return;
}