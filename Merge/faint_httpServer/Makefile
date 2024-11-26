CC=gcc
CFLAGS=-Wall -g
OBJ=http_server.o http_utils.o
TARGET=http_server

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

http_server.o: http_server.c http_utils.h
	$(CC) $(CFLAGS) -c http_server.c

http_utils.o: http_utils.c http_utils.h
	$(CC) $(CFLAGS) -c http_utils.c

clean:
	rm -f $(OBJ) $(TARGET)
