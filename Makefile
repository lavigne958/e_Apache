CC = gcc
CFLAGS = -W -Wall -ansi -pedantic -Iinclude
LDFLAGS = -lpthread

DIR=.
CGI=$(DIR)/cgi/
BIN=$(DIR)/bin/
OBJ=$(DIR)/obj/
INCLUDE=$(DIR)/include/
LIB=$(DIR)/lib/
SRC=$(DIR)/src/


all: $(BIN)server

launch: $(BIN)server $(CGI)test_cgi
	cd ./files/ && ../bin/server 8080 1 45 

$(BIN)server: $(OBJ)server.o $(OBJ)requete.o
	$(CC) -o $@ $^ $(LDFLAGS);

$(CGI)test_cgi: $(OBJ)test_pipe_prog.o 
	$(CC) -o $@ $^ $(LDFLAGS);
	cp -r $@ files/

$(OBJ)%.o: $(SRC)%.c
	@if [ -d $(OBJ) ]; then : ; else mkdir $(OBJ); fi
	$(CC) $(CFLAGS) -o $@ -c $< $(LDFLAGS)

clean:
	rm -f $(OBJ)*.o $(SRC)*~ $(BIN)* $(DIR)/*~

