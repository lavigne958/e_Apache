CC = gcc
CFLAGS = -W -Wall -ansi -pedantic -Iinclude
LDFLAGS = -lpthread

DIR=.
INCLUDE=$(DIR)/include/
SRC=$(DIR)/src/


all: server test_cgi

launch: server test_cgi 
	./server 8080 3 35000000000

server: server.o requete.o vigilante.o
	$(CC) -o $@ $^ $(LDFLAGS);

test_cgi: test_pipe_prog.o 
	$(CC) -o $@ $^ $(LDFLAGS);

%.o: $(SRC)%.c
	$(CC) $(CFLAGS) -o $@ -c $< $(LDFLAGS)

clean:
	rm -f *.o $(SRC)*~ $(DIR)/*~ test_cgi server
