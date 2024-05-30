# Directories
SRCDIR = src/
BLDDIR = build/
BINDIR = bin/
INCDIR = include/

CC	= gcc
FLAGS   = -g -c 
# -g option enables debugging mode 
# -c flag generates object code for separate files

OBJS1 	= jobCommander.o queue_implementation.o help_server.o
OBJS2	= JobExecutorServer.o queue_implementation.o help_server.o
HEADER  = queue.h help.h
OUT1  	= jobCommander
OUT2 	= JobExecutorServer
OUT3	= progDelay


all: jobCommander \
	JobExecutorServer \
	progDelay

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(OUT1) : $(OBJS1) 
	$(CC) -g $(OBJS1) -o $(OUT1)

JobExecutorServer: $(OBJS2) 
	$(CC) -g $(OBJS2) -o $(OUT2)

progDelay: progDelay.c
	$(CC) -g progDelay.c -o progDelay

# create/compile the individual files >>separately<< 
jobCommander.o: jobCommander.c $(HEADER)
	$(CC) $(FLAGS) jobCommander.c

JobExecutorServer.o: JobExecutorServer.c $(HEADER)
	$(CC) $(FLAGS) JobExecutorServer.c

queue_implementation.o: queue_implementation.c	$(HEADER)
	$(CC) $(FLAGS) queue_implementation.c

help_server.o: help_server.c help.h
	$(CC) $(FLAGS) help_server.c

# clean house
clean:
	rm -f $(OBJS1) $(OBJS2) $(OUT1) $(OUT2) $(OUT3)

restart:
	rm -f jobExecutorServer.txt
	rm -f myfifo
	rm -f issue
	rm -f exit