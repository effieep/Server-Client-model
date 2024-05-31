# Directories
SRCDIR = ./src
BLDDIR = ./build
BINDIR = ./bin
INCDIR = ./include

CC	= gcc
FLAGS   = -I$(INCDIR) -g 
#-Wall -Wextra -Werror NA TA VALOOOOOOO
# -g option enables debugging mode 
# -c flag generates object code for separate files

OBJS1 	= $(BLDDIR)/jobCommander.o $(BLDDIR)/queue_implementation.o $(BLDDIR)/help_client.o
OBJS2	= $(BLDDIR)/jobExecutorServer.o $(BLDDIR)/queue_implementation.o $(BLDDIR)/help_server.o
HEADER1 = $(INCDIR)/queue.h $(INCDIR)/help_client.h
HEADER2  = $(INCDIR)/queue.h $(INCDIR)/help_server.h
OUT1  	= $(BINDIR)/jobCommander
OUT2 	= $(BINDIR)/jobExecutorServer
OUT3	= $(BINDIR)/progDelay


all: $(OUT1) $(OUT2) $(OUT3)

$(OUT1) : $(OBJS1) 
	$(CC) $(OBJS1) -o $(OUT1)

$(OUT2) : $(OBJS2) 
	$(CC) $(OBJS2) -o $(OUT2)

$(OUT3) : $(SRCDIR)/progDelay.c
	$(CC) $(SRCDIR)/progDelay.c -o $(OUT3)

$(BLDDIR)/jobCommander.o: $(SRCDIR)/jobCommander.c $(HEADER)
	$(CC) -c $(FLAGS) $(SRCDIR)/jobCommander.c -o $(BLDDIR)/jobCommander.o

$(BLDDIR)/jobExecutorServer.o: $(SRCDIR)/jobExecutorServer.c $(HEADER)
	$(CC) -c $(FLAGS) $(SRCDIR)/jobExecutorServer.c -o $(BLDDIR)/jobExecutorServer.o

$(BLDDIR)/queue_implementation.o: $(SRCDIR)/queue_implementation.c $(HEADER)
	$(CC) -c $(FLAGS) $(SRCDIR)/queue_implementation.c -o $(BLDDIR)/queue_implementation.o

$(BLDDIR)/help_server.o: $(SRCDIR)/help_server.c $(INCDIR)/help_server.h
	$(CC) -c $(FLAGS) $(SRCDIR)/help_server.c -o $(BLDDIR)/help_server.o

$(BLDDIR)/help_client.o: $(SRCDIR)/help_client.c $(INCDIR)/help_client.h
	$(CC) -c $(FLAGS) $(SRCDIR)/help_client.c -o $(BLDDIR)/help_client.o

# clean house
clean:
	rm -f $(OBJS1) $(OBJS2) $(OUT1) $(OUT2) $(OUT3)

restart:
	rm -f jobExecutorServer.txt