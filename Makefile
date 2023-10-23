# Makefile

FIFOS_DIR = Manager_Worker_FIFOS
OUT_DIR = OUT_FILES
SOURCES = sources
HEADERS = headers
OBJECTS = objects
SNIFFER = sniffer
CC = gcc
CFLAGS = -g -Wall -pedantic -I$(HEADERS)


#find all the .c files in the sources/ directory
SOURCE_FILES = $(shell find $(SOURCES)/ -name '*.c')
#e.g create from sources/file1.c  the name  objects/file1.o
OBJECT_FILES = $(SOURCE_FILES:$(SOURCES)/%.c=$(OBJECTS)/%.o)

############################################################################

.PHONY: clean all run valgrind

############################################################################

all: clean objects_dir $(SNIFFER) finder

#make the objects/ directory
objects_dir:
	mkdir -p objects

#give execute rights to the bash script
finder:
	chmod +x finder.sh

#make the sniffer programm using all the object files in the objects/ directory
$(SNIFFER): $(OBJECT_FILES)
	$(CC) $(CFLAGS) -o $(SNIFFER) $(OBJECT_FILES)

#make the .o files
$(OBJECTS)/%.o: $(SOURCES)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

#################################################################################

run: all $(SNIFFER) 
	./$(SNIFFER) 

#################################################################################

clean:
	rm -rf $(OUT_DIR)
	rm -rf $(OBJECTS) $(FIFOS_DIR)
	rm -f $(SNIFFER) *copy*

#################################################################################

valgrind1: clean all $(SNIFFER)
	valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes --trace-children=yes ./$(SNIFFER)

valgrind2: clean all $(SNIFFER)
	valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes --trace-children=yes ./$(SNIFFER) -p /mnt/d/SEMESTER_6/SYSPRO/ 

# commands:
# 	sudo umount /mnt/d
# 	sudo mount -t drvfs D: /mnt/d -o metadata