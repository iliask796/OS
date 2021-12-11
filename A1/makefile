# the compiler: 
	CC = g++
 
# compiler flags:
	# -pthread - for semaphores
  	CFLAGS  = -std=c++14 -pthread

# The build target
	TARGET1 = main

all: $(TARGET1)

$(TARGET1): main.o SharedData.o
	$(CC) $(CFLAGS) -o main main.o SharedData.o

main.o: main.cpp SharedData.h
	$(CC) $(CFLAGS) -c main.cpp

SharedData.o: SharedData.h

clean:
	$(RM) $(TARGET1)
	$(RM) main.o
	$(RM) SharedData.o