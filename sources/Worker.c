#include "../headers/worker.h"
#include "../headers/Signalhandlers.h"

bool RUNNING = true;


void Worker(){

    char* manager_worker_FIFO = NULL;
    int fifo_fd = -1;
    char buffer_to_read_from[MAX_LENGTH] = {'\0'}; //buffer to read the names of the files from manager
    int num_bytes_fifo = -1;
    manager_worker_FIFO = Create_FIFO_Name(getpid(), getppid());
    manager_worker_FIFO = Create_FullPath_of_File(FIFO_DIR, manager_worker_FIFO);
    
    struct sigaction worker_action;
    memset(&worker_action, 0, sizeof(struct sigaction));

    sigfillset(&worker_action.sa_mask);
    sigdelset(&worker_action.sa_mask, SIGTSTP);
    sigdelset(&worker_action.sa_mask, SIGCONT);

    worker_action.sa_handler = SIG_DFL;
    sigaction(SIGCONT, &worker_action, NULL);

    worker_action.sa_handler = Finish_Worker;
    sigaction(SIGTSTP, &worker_action, NULL);

    //block all the signals except for SIGSTSP, SIGCONT
    sigprocmask(SIG_SETMASK, &worker_action.sa_mask, NULL);

    while ((fifo_fd = open(manager_worker_FIFO, O_RDONLY)) < 0){
        if (errno == EINTR) continue;
        perror("Worker fifo open problem");
        //exit(3);	
    }
    while (RUNNING){
        
        //printf("child with id %d and parent id %d\n", getpid(), getppid());

        //else{
            while((num_bytes_fifo = read(fifo_fd, buffer_to_read_from, (size_t)MAX_LENGTH)) < 0){
                if(errno == EINTR){
                    perror("Read from FIFO was interrupted by a signal\n");
                    continue;
                }
                if(errno == EAGAIN){
                    perror("Read error in worker\n");
                    break;
                }
                perror("Worker reads");
            }
            while(strlen(buffer_to_read_from) > 0){
                //printf("WORKER reads num bytes: %d from FIFO with fd %d and name %s\n", num_bytes_fifo, fifo_fd, manager_worker_FIFO);
                char* file_to_open = (char*)calloc(num_bytes_fifo+1, sizeof(char));
                memcpy(file_to_open, buffer_to_read_from, num_bytes_fifo);
                usleep(20000);
                printf("WORKER FROM FIFO with fd %d and name %s HAS READ: %s from %d\n", fifo_fd, manager_worker_FIFO, buffer_to_read_from, fifo_fd);
                Process_File(file_to_open);
                memset(buffer_to_read_from, '\0', (size_t)MAX_LENGTH);
                memset(file_to_open, '\0', strlen(file_to_open));
                free(file_to_open);
                

                raise(SIGSTOP); //worker gets stopped
            }
        //}
    }
    if(close(fifo_fd) == -1){
        if(errno == EBADF){
            printf("Bad file descriptor\n");
        }
        if(errno == EINTR){
            printf("Close at worker was interrupted by signal\n");
        }
        perror("Close Worker");
    }
    if (unlink(manager_worker_FIFO) == -1){
        perror("Unlinking FIFO");
    }
    free(manager_worker_FIFO);
    
    return;
}


/*worker processes the file which the manager sent
 through the named_pipe and the worker read it*/

void Process_File(char* filename){
    
    int filedes_read = -1, filedes_write =-1;;
    size_t num_bytes_read = 0; 
    off_t total_bytes = 0;
    char text_buffer[BUFF_SIZE];   //buffer to read from 
    memset(text_buffer, '\0', BUFF_SIZE);
    char* temp_text_buffer = NULL;
    struct stat file_info;
    memset(&file_info, '\0', sizeof(file_info));
    
    // user: read, write, execute  group and others: read 
    mode_t open_mode = S_IRUSR|S_IRGRP|S_IROTH;

    /* find the size of the file */
    if(stat(filename, &file_info) < 0)
        perror("Worker could not get info for the file to process");
    else
        total_bytes = file_info.st_size;
        //printf("total file size %ld, %ld\n", total_bytes, file_info.st_blocks);
    
    while ( (filedes_read = open(filename, O_RDONLY , open_mode)) == -1){
        if(errno ==  EINTR)   continue;
        perror("Worker: Could not open file for processing");
        return;
    }

    printf("Worker --> %d opened file %s with file desc %d to read from\n", getpid(), filename, filedes_read); 

    Queue links = Queue_Initialize();   //stores the link_location names along with their occurences

    /*process of the file -- worker reads the file*/
    while(total_bytes != 0){
        while((num_bytes_read = read(filedes_read, text_buffer, BUFF_SIZE - 1)) < 0){
            if(num_bytes_read < 0){
                if(errno == EINTR){
                    perror("Reading file interrupted by signal");
                    continue;
                }     
                perror("Worker error while reading the file\n");
            }
        }
        char* line =  NULL;
        char* rest_lines = text_buffer; 
        //text is split according to the delimeters below
        while ((line = strtok_r(rest_lines, " \t\r\n\v\f", &rest_lines)) != NULL){
            //there is an interrupted line
            if(temp_text_buffer != NULL){
                char* temp_buff = (char*)calloc(strlen(temp_text_buffer)+ strlen(line)+1, sizeof(char));
                memcpy(temp_buff, temp_text_buffer, strlen(temp_text_buffer));
                memcpy(temp_buff + strlen(temp_buff), line, strlen(line));
                temp_text_buffer[strlen(temp_text_buffer)] = '\0';
                Process_Line(temp_buff, links);
                free(temp_text_buffer);
                free(temp_buff);
                temp_text_buffer = NULL;
            }
            //store the last line at each iteration as interrupted line     
            char ch = line[strlen(line)-1];
            char* res = strstr(" \t\r\n\v\f", &ch);
            if((strlen(rest_lines) == 0) && (res == NULL)){
                temp_text_buffer = (char*)calloc(strlen(line)+1, sizeof(char));
                memcpy(temp_text_buffer, line, strlen(line));
                memset(text_buffer, '\0', BUFF_SIZE);

            }
            else{
                Process_Line(line, links);
            }

        }
        total_bytes -= num_bytes_read;
    }
    //last line 
    if(temp_text_buffer != NULL){
        Process_Line(temp_text_buffer, links);
        free(temp_text_buffer);
        temp_text_buffer = NULL;
    } 
    printf("TOTAL LINKS FOUND: %d\n", Queue_Size(links));
    
    //new file to write  -- create the name of the .out file
    char file_suffix[] = ".out";

    char* filename_no_path = strrchr(filename, '/');
    if(filename_no_path != NULL)  filename_no_path += 1;
    else      filename_no_path = filename; 

    char* out_file = (char*)calloc(strlen(filename_no_path)+ strlen(file_suffix) + 1, sizeof(char));
    memcpy(out_file, filename_no_path, strlen(filename_no_path));
    memcpy(out_file + strlen(out_file), file_suffix, strlen(file_suffix));
    out_file[strlen(out_file)] = '\0';
    out_file = Create_FullPath_of_File(OUT_DIR, out_file);

    //open this file -- if it already exists it is overwritten
    if ( (filedes_write = open(out_file, O_CREAT|O_TRUNC|O_RDWR, 0644)) == -1){
        perror("Creating file for writing links");
        exit(1);
    }
    else{ 
        printf("Worker --> %d opens file %s with file desc %d to write to\n", getpid(), out_file, filedes_write); 
    }

    //writes all the links to file
    Write_Links_To_File(filedes_write, links);
    
    close(filedes_read);  //close the file descriptor of the file, which the worker has read
    close(filedes_write);   //close the file descriptor of the filethe worker wrote to

    Queue_Destroy(links);
    free(out_file);
    return;
}


//checks for the "http://" at every line
void Process_Line(char* line_buffer, Queue links){
    
    char prefix[] = "http://";  

    while(strlen(line_buffer) != 0){
        char* prefix_position = NULL;
        
        //check where "http://" starts
        if ((prefix_position = strstr(line_buffer, prefix)) != NULL){   
            prefix_position += strlen(prefix);
            Extract_Link_Location(prefix_position, links);
            memset(line_buffer, '\0', strlen(line_buffer));

        }
        else{   //we do not have http:// or it was interrupted
            if(strlen(line_buffer) <= strlen(prefix)){
                if( !strncmp(line_buffer, prefix, strlen(line_buffer))){
                    return;  // http:// was interrupted so we continue reading
                } 
            }
            memset(line_buffer, '\0', strlen(line_buffer));
        }
    }
    return;
}


/*checks if the rest criteria for the existence of a link are
 satisfied and extracts the location of the link along with the TLD */
void Extract_Link_Location(char* buffer, Queue links){
    
    char* end_pos1 = NULL;
    char* end_pos2 = NULL;
    char* new_line = NULL;
    char prefix[] = "www.", prefix2[] = "http://";
    static int i = 0;
    int bytes = 0, extra_bytes = 0;

    /*if a line was interrupted there may be a chance
     that it has a 2nd http from another link*/
    if((new_line = strstr(buffer, prefix2)) != NULL){
        char* temp_buff = (char*)calloc(strlen(new_line) + 1, sizeof(char));
        memcpy(temp_buff, new_line, strlen(new_line));
        Extract_Link_Location(temp_buff, links);
        memset(new_line, '\0', strlen(new_line));
        free(temp_buff);
    }
    
    //check where '/' or ':' is
    end_pos1 = strstr(buffer, ":");
    end_pos2 = strstr(buffer, "/");

    if ( !strncmp(buffer, prefix, strlen(prefix)) ){   //check where "www." is
        extra_bytes = strlen(prefix);
    }

    if((end_pos1 == NULL) && (end_pos2 == NULL)){
        bytes = strlen(buffer) - extra_bytes;
    }
    else{   //check where '/' or ':' is and which of the both we encounter first
        
        if((end_pos1 == NULL) && (end_pos2 != NULL)){
            end_pos1 = end_pos2;
        }
        else if((end_pos1 != NULL) && (end_pos2 != NULL)){
            
            if(end_pos1 > end_pos2){
                end_pos1 = end_pos2;
            }
        }
        bytes = strlen(buffer) - strlen(end_pos1) - extra_bytes;
    
    }
    
    //copy the link
    char* location = (char*)calloc(bytes + 1, sizeof(char));
    memcpy(location, buffer + extra_bytes, bytes);
    location[bytes] = '\0';
    i++;

    //check if it already exists
    QNode loc = Queue_Find_NameLocation(links, location);
    if(loc == NULL){
        loc = QueueNode_Create_Link(location);
        Queue_Insert(links, loc);
    }
    QueueNode_Increase_Appearances(loc);
    
    free(location);

    return;
}


//gets the names of the links and their occurences and writes them to the .out file
void Write_Links_To_File(int fd, Queue links){

    int num_bytes_write = -1;
    char tabs[] = {"\t\t\t"};
    char msg1[] = {"LINK LOCATION "};
    char msg2[] = {"TIMES OF APPEARANCE\n"};
    QNode link = NULL;

    int max_links_length = Find_Max_Length_Of_Link(links);
    if(max_links_length > 0){
        if(max_links_length < strlen(msg1)){
            max_links_length = strlen(msg1);
        }
    }
    else{
        perror("There are no links to write");
        return;
    }

    int max_line_length =  max_links_length + strlen(msg2) + strlen(tabs) + 1;

    //write the title
    char* title = Create_Line_To_Write(link, max_line_length, msg1, msg2, tabs);
    if((num_bytes_write = write(fd, title, strlen(title))) != 0){
        if(num_bytes_write < 0){
            if(errno == EINTR){
                perror("Write link was interrupted by signal\n");
            }
        }
    }
    free(title);

    //iterate through the Queue with stores the links and write them one by one to the file
    link = Queue_First(links);
    while(link != NULL){
        char* line = Create_Line_To_Write(link, max_line_length, msg1, msg2, tabs);
        if((num_bytes_write = write(fd, line, strlen(line))) != 0){
            if(num_bytes_write < 0){
                if(errno == EINTR){
                    perror("Write link was interrupted by signal\n");
                }
            }
        }
        free(line);
        link = QueueNode_Next(link);
    }
}



/*we need it in order to find the maximum length of the line to write
in the .out file so that all lines have the same length*/
int Find_Max_Length_Of_Link(Queue links){

    int max_length = -1, curr_length = -1;
    QNode link = Queue_First(links);
    while(link != NULL){

        curr_length = strlen(QueueNode_GetLink_Name(link));
        if(curr_length > max_length){
            max_length = curr_length;
        }
        link = QueueNode_Next(link);
    }

    return max_length;   /*na balo kati gia error an mhkos ison -1*/
}



/*Create the format of the line to write in the .out file*/
/*the format is: [link location     number of occurences]*/
char* Create_Line_To_Write(QNode link, int max_line_length, char* msg1, char* msg2, char* tabs){
 
    char* spaces = NULL, *line = NULL;
    char space = ' ';
    int num_spaces = -1;

    /*calculate the needed number of spaces to apply between link name
    and occurences so that the result is in two columns in the .out file*/
    
    if(link == NULL){   //title is written
        line = (char*)calloc(max_line_length + 1, sizeof(char));
        num_spaces = max_line_length - strlen(msg1) - strlen(msg2) - strlen(tabs) - 1;
        spaces = (char*)calloc(num_spaces + 1, sizeof(char));
        memset(spaces, space, num_spaces);

        snprintf(line, max_line_length, "%s%s%s%s", msg1, spaces, tabs, msg2);
    }
    else{  //link is written
        char* link_name = QueueNode_GetLink_Name(link);
        unsigned int times_appears = QueueNode_Get_Appearances(link);
        int link_length = strlen(link_name);
        
        num_spaces = max_line_length - link_length - strlen(msg2) - strlen(tabs) - 1;
        spaces = (char*)calloc(num_spaces + 1, sizeof(char));
        memset(spaces, space, num_spaces);

        line = (char*)calloc(max_line_length + 1, sizeof(char)); //max_length
        snprintf(line, max_line_length, "%s%s%s%d%c", link_name, spaces,tabs, times_appears, '\n');

    }
    free(spaces);
    return line;  //written line in the .out file
}