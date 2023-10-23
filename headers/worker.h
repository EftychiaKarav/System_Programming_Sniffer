#ifndef WORKER_H_
#define WORKER_H_

#include "libraries.h"
#include "Queue.h"

extern Queue workers_queue;


void Worker();    //process of the worker


void Process_File(char*);   //reads the file line by line 


void Process_Line(char* , Queue);   //checks for the "http://" at every line


/*checks if the rest criteria for the existence of a link are
 satisfied and extracts the location of the link along with the TLD */
void Extract_Link_Location(char*, Queue); 


//gets the names of the links and their occurences and writes them to the .out file
void Write_Links_To_File(int, Queue);  


/*we need the max_length from the total set of links so that  
all lines have the same length in the .out file*/
int Find_Max_Length_Of_Link(Queue);


/*Creates the format of the line to write in the .out file;
the format is: [link location     number of occurences]*/
char* Create_Line_To_Write(QNode, int, char*, char*, char* );


#endif