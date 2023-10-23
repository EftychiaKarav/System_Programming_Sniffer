#ifndef LIBRARIES_H_
#define LIBRARIES_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#include <features.h>
#include <dirent.h>


#define FIFO_DIR "Manager_Worker_FIFOS"
#define FIFO_DIR_MAX_NAME_LENGTH 41
#define OUT_DIR "OUT_FILES"
#define BUFF_SIZE 2048   //to read the content from the file
#define PERMS 0644
#define MAX_LENGTH 1024  //to read the names of the files 
#define READ 0
#define WRITE 1

extern bool RUNNING;
extern unsigned int Children;  //total number of workers whether they are in the Queue or not
extern int listener_manager_pipe[2]; //listener writes, manager reads

//gets the filename from the event that inotifwait has returned
char* Extract_FileNamePath(char*, int);  

/*the names of FIFOs have the following pattern: 
    "MW_FIFO_[manager_pid]_[worker_pid]"   */
char* Create_FIFO_Name(unsigned int, unsigned int);

char* Create_FullPath_of_File(char* , char* ); //concat a filename with its directory or path


#endif