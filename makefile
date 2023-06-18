#makefile Multi Client TCP

TARGET = server.out
TARGET2 = mclient.out
CC = gcc
CFLAGS = -ansi -pedantic -g
OBJS = Server.o ServerMain.o
OBJS2 = MClient.o

server: $(OBJS)
	$(CC) -o  $(TARGET)  $(OBJS) -L. -lDS -lrt

client: $(OBJS2)
	$(CC) -o  $(TARGET2)  $(OBJS2) -L. -lDS -lrt
	
Server.o: Server.c Server.h ListItr.h ListFunc.h Internal.h GenericDoubleList.h 
	$(CC)  $(CFLAGS) -c Server.c 	

ServerMain.o: ServerMain.c
	$(CC)  $(CFLAGS) -c ServerMain.c 

MClient.o: MClient.c
	$(CC)  $(CFLAGS) -c MClient.c	
		
clean:
	rm -f $(TARGET) $(OBJS)
	rm -f $(TARGET2) $(OBJS2)
