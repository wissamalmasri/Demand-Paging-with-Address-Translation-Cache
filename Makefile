CC = g++
CFLAGS = -std=c++11 -g -Wall -c

OBJS = main.o pageTableLevel.o tracereader.o log.o

PROGRAM = pagingwithatc

$(PROGRAM) : $(OBJS)
	$(CC) -o $(PROGRAM) $(OBJS)

main.o : main.cpp
	$(CC) $(CFLAGS) main.cpp

pageTableLevel.o : pageTableLevel.cpp pageTableLevel.h
	$(CC) $(CFLAGS) pageTableLevel.cpp

tracereader.o : tracereader.cpp tracereader.h
	$(CC) $(CFLAGS) tracereader.cpp

log.o : log.cpp log.h
	$(CC) $(CFLAGS) log.cpp

clean :
	rm -f $(OBJS) $(PROGRAM)
