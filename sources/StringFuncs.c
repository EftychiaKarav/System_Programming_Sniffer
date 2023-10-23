#include "../headers/libraries.h"
#include "../headers/Queue.h"


/*function for extracting the right path for the incoming files after an event is being created*/

char* Extract_FileNamePath(char* event_message, int num_bytes){

    /*event_message is a string and it has the following format: 
    [DIR/filename\n][DIR/filename\n][DIR/filename\n]...*/
    /*we split the above message into the distinct events and 
     we return the string until the first occurence of '\n'*/

    //num_bytes is the length of the event string
    char* FileName_Path = NULL;  //name for the valid file path
    char* endl_position = NULL;
    char* temp_string = NULL;
    int old_num_bytes = num_bytes;
    
    if ((endl_position = strstr(event_message, "\n")) != NULL){
        num_bytes = endl_position - event_message + 1; //1 is for '\0'
        FileName_Path = (char*)calloc(num_bytes, sizeof(char)); 
        memcpy(FileName_Path, event_message, num_bytes-1); //without '\n'
        FileName_Path[strlen(FileName_Path)] = '\0';
    }
    else{
        FileName_Path = event_message;
    }    
    
    //keep the rest message and copy it to a temp buffer and then again to "event_message" string 
    temp_string = (char*)calloc(old_num_bytes - num_bytes + 1, sizeof(char));
    memcpy(temp_string, event_message + num_bytes, old_num_bytes - num_bytes);
    temp_string[strlen(temp_string)] = '\0';

    memset(event_message, '\0', strlen(event_message));
    memcpy(event_message, temp_string, strlen(temp_string));
    free(temp_string);
    return FileName_Path;
}



//create the name for the named_pipe for a given pair child-parent (worker, manager)
char* Create_FIFO_Name(unsigned int child_pid, unsigned int parent_pid){

    /*FIFO name for every pair, manager - worker will have the following format:
                        "MW_FIFO_[manager_pid]_[worker_pid]"
    Each pid can have at most 5 digits so the maximum length of the string is 19*/
    char* fifo_name = (char*)calloc(20, sizeof(char));  //19 + 1 for '\0'.
    char *prefix = "MW_FIFO_";
    char *underscore = "_";
    snprintf(fifo_name, 20,"%s%d%s%d", prefix, parent_pid, underscore, child_pid);
    fifo_name[strlen(fifo_name)] = '\0';
    
    return fifo_name;
}



//concat a filename with the given path or directory
char* Create_FullPath_of_File(char* dirName, char* file_name){

    bool exists = true;
    int total_length = strlen(dirName) + strlen(file_name) + 1; //1 for '\0'
    if(dirName[strlen(dirName)-1] != '/'){
      total_length++;   //allocate memory for the '/' character too
      exists = false;
    }
    //copy the filename to a temp buffer
    char* temp_string = calloc(strlen(file_name) + 1, sizeof(char));
    memcpy(temp_string, file_name, strlen(file_name));
    free(file_name);

    //build the whole path for the filename
    file_name = calloc(total_length, sizeof(char));
    if(exists){ //character '/' exists at the end of the directory name 
      snprintf(file_name, total_length,"%s%s", dirName, temp_string);
    }
    else{ 
      snprintf(file_name, total_length,"%s%c%s", dirName, '/', temp_string);
    }
    
    file_name[strlen(file_name)] = '\0';
    free(temp_string);
    return file_name;
}

