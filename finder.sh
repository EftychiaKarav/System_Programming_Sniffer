#!/bin/bash

#if no arguments are given
if [ $# -eq 0 ]
then 
    echo "Please give 1 or more Top Level Domains to search for and run the script again."
    echo "Exiting."
    exit 1
fi

echo "Number of TLDs:" $#    #total number of given arguments
echo "These TLDs are:" $*    #names of the arguments


#All the .out files are in the "OUT_FILES/" directory
OUT_DIR="OUT_FILES/"

if [ ! -d $OUT_DIR ]   #if the directory does not exist
then
    echo "No directory with name "\"$OUT_DIR"\" and no "\".out"\" files to search from."
    echo "Execute ./sniffer in order for the directory and the "\".out"\" files to be created."    
    exit 1
fi

#go into the directory
cd $OUT_DIR; 
OUT_FILES_LIST=`ls *.out`   #list of all .out files
 
if [ "${#OUT_FILES_LIST}" -eq 0 ]   #if there are no .out files
then
    echo "There are no .out files in the directory to search from." 
    echo "Exiting."
    exit 1
fi

#print the names of the .out files
echo "List of all "\".out"\" files to serach from:" 
for out_file in $OUT_FILES_LIST
do
  echo "--"$out_file 
done
echo


#total occurences for each TLD in all .out files
TOTAL=0

#iterate over each arg; TLD
for arg in $*
do
    TLD="."$arg    #add the '.' to each TLD
    for file in ${OUT_FILES_LIST}
    do
        #check if the files are empty
        words=`wc -w < ${file}`
        if [ "$words" -eq 0 ]
        then
            continue
        fi
        #1. Stdin replaced by each file in order to search the tld
        exec < ${file}
        #2. find with help of grep the rows where a specific tld exists   
        TEXT=`grep -E -e ${TLD}[[:space:]] ${file}`

        #3. load the result of grep into an array; the array has #elements = 2*num_rows of the grep's result
        #Each row is a pair (location, occurences) and every element of this pair is inserted at 
        #a different position in the array; so 
        #--elements with even index in the array are the locations of the links
        #--elements with odd index, i, in the array are the total occurences for the location 
        #that is in the i-1 position each time
        TEXT=($(echo $TEXT))  #the array

        #4. iterate over each element in the array
        for ((index=0; index < ${#TEXT[@]}; index++))
        do
            #if the element is in a position with odd index then it is a number 
            if [ `expr $index % 2` -eq 1 ]
            then
                occurences=${TEXT[index]}
                TOTAL=$(($TOTAL+$occurences))   #add them to total
            fi
        done

    done
    echo "$TLD: $TOTAL"
    TOTAL=0   #TOTAL is 0, so that we can start count for the next TLD
done

exit 0