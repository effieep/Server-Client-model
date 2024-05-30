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

OBJS1 	= $(BLDDIR)/jobCommander.o $(BLDDIR)/queue_implementation.o $(BLDDIR)/help_server.o
OBJS2	= $(BLDDIR)/JobExecutorServer.o $(BLDDIR)/queue_implementation.o $(BLDDIR)/help_server.o
HEADER  = $(INCDIR)/queue.h $(INCDIR)/help.h
OUT1  	= $(BINDIR)/jobCommander
OUT2 	= $(BINDIR)/JobExecutorServer
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

$(BLDDIR)/JobExecutorServer.o: $(SRCDIR)/JobExecutorServer.c $(HEADER)
	$(CC) -c $(FLAGS) $(SRCDIR)/JobExecutorServer.c -o $(BLDDIR)/JobExecutorServer.o

$(BLDDIR)/queue_implementation.o: $(SRCDIR)/queue_implementation.c $(HEADER)
	$(CC) -c $(FLAGS) $(SRCDIR)/queue_implementation.c -o $(BLDDIR)/queue_implementation.o

$(BLDDIR)/help_server.o: $(SRCDIR)/help_server.c $(INCDIR)/help.h
	$(CC) -c $(FLAGS) $(SRCDIR)/help_server.c -o $(BLDDIR)/help_server.o

# clean house
clean:
	rm -f $(OBJS1) $(OBJS2) $(OUT1) $(OUT2) $(OUT3)

restart:
	rm -f jobExecutorServer.txt
	rm -f myfifo
	rm -f issue
	rm -f exit