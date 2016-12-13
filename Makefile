CC = gcc
CFLAGS = -W -Wall -ansi -pedantic -Iinclude
LDFLAGS = -lpthread

DIR=.
BIN=$(DIR)/bin/
OBJ=$(DIR)/obj/
INCLUDE=$(DIR)/include/
LIB=$(DIR)/lib/
SRC=$(DIR)/src/


all: $(BIN)server

launch: $(DIR)/bin/server
	cd ./files/ && ../bin/server 8080 3 45 

$(BIN)server: $(OBJ)server.o $(OBJ)requete.o
	$(CC) -o $@ $^ $(LDFLAGS);

$(OBJ)%.o: $(SRC)%.c
	@if [ -d $(OBJ) ]; then : ; else mkdir $(OBJ); fi
	$(CC) $(CFLAGS) -o $@ -c $< $(LDFLAGS)

clean:
	rm -f $(OBJ)*.o $(SRC)*~ $(BIN)* $(DIR)/*~

