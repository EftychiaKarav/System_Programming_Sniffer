#include "../headers/Signalhandlers.h"
#include "../headers/worker.h"
#include "../headers/Manager.h"


int main(int argc, char* argv[]){

    
    DIR * dir_ptr = NULL;
    char* dirname = NULL;
    /* check the arguments from command line*/
    if(argc > 3){
        fprintf(stdout, "Too many arguments in command line.\n");
        exit(EXIT_FAILURE);
    }
    else if(argc == 2){
        fprintf(stdout, "One argument is missing.\n");
        exit(EXIT_FAILURE);
    }    
    else if(argc == 3){
        if(strcmp(argv[1], "-p") != 0){
            fprintf(stdout, "Second argument should be -p.\n");
            exit(EXIT_FAILURE);
        }
        if((dir_ptr = opendir(argv[2])) == NULL ){
            fprintf(stdout, "Cannot open %s directory\n", argv[2]);
            printf("Please specify a valid directory and execute the program again.\n");
            exit(EXIT_FAILURE);
        }
        dirname = argv[2];
        printf("%s our directory\n", dirname);
    }

    /* create the directories for storing the FIFOs and the .out files */
    if(mkdir(FIFO_DIR, 0744) == -1){
        if(errno != EEXIST){
            perror("Could not create new directory");
            exit(EXIT_FAILURE);
        } 
    }

    if(mkdir(OUT_DIR, 0744) == -1){
        if(errno != EEXIST){
            perror("Could not create new directory");
            exit(EXIT_FAILURE);
        } 
    }

    /* create the pipe for the communication between manager - listener */
    if (pipe(listener_manager_pipe) == -1) { 
        perror("Could not create a pipe"); 
        exit(EXIT_FAILURE);
    }

    pid_t pid_manager = getpid();
    printf("MANAGER STARTS WITH PID = %d AND PARENT PID = %d\n", pid_manager, getppid());

    /* Ignore all signals at this point */
    struct sigaction manager_action;
    memset(&manager_action, 0, sizeof(struct sigaction));
    manager_action.sa_handler = SIG_IGN;
    //manager_action.sa_flags = SA_RESTART;
    sigfillset(&manager_action.sa_mask);
	sigaction(SIGINT, &manager_action, NULL);
	sigaction(SIGCHLD, &manager_action, NULL);

    pid_t pid_listener = -1;

    pid_listener = fork();   //produce listener process
    
    if (pid_listener < 0){
        perror ("Error in Fork");
        exit(EXIT_FAILURE);
    }
    else if (pid_listener == 0){      //listener process
        Listener(dirname);
    }

    //else{
        //manager process
        close(listener_manager_pipe[1]); //close the write end of the pipe for manager

        /* stop ignoring some signals - initialize sigaction for manager process*/
        /* 1. when the user send SIGINT (types ^C) */
        manager_action.sa_handler = Inform_Manager;
        sigaction(SIGINT, &manager_action, NULL);
        /* 2. when workers send SIGCHLD (when they stop or continue) */
        manager_action.sa_handler = Signal_from_Child;
        sigaction(SIGCHLD, &manager_action, NULL);

        /* communication of manager with listener and workers*/
        Manager(dir_ptr, dirname);

        /* after this point the termination of all processes begins*/
        RUNNING = true;

        /* change the action of the signal handler for SIGCHLD*/
        /* handle SIGCHLD when workers exit */
        manager_action.sa_handler = Wait_Child;
        sigaction(SIGCHLD, &manager_action, NULL);
        
        Terminate_Workers();  /* manager notifies workers to finish */
        kill(pid_listener, SIGKILL);   /* manager termminated listener */

        /* manager waits -- stops when all children have exited */
        while(RUNNING);   

        Clean_Memory(true, dir_ptr, dirname, NULL);
        printf("MANAGER %d ---------- terminates\n", pid_manager);
        printf("------------ Exit Program ------------\n");
        close(STDOUT_FILENO);
        close(STDIN_FILENO);
        close(STDERR_FILENO);
        exit(EXIT_SUCCESS);
    //}
}