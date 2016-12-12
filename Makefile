CC = gcc
CFLAGS = -W -Wall -ansi -pedantic -Iinclude
LDFLAGS = -lpthread

DIR=.
BIN=$(DIR)/bin/
OBJ=$(DIR)/obj/
INCLUDE=$(DIR)/include/
LIB=$(DIR)/lib/
SRC=$(DIR)/src/

all: $(BIN)server $(BIN)test

$(BIN)server: $(OBJ)server.o
	$(CC) -o $@ $^ $(LDFLAGS);
	cp $(BIN)server files/server

$(BIN)%: $(OBJ)%.o
	@if [ -d $(BIN) ]; then : ; else mkdir $(BIN); fi
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJ)%.o: $(SRC)%.c
	@if [ -d $(OBJ) ]; then : ; else mkdir $(OBJ); fi
	$(CC) $(CFLAGS) -o $@ -c $< $(LDFLAGS)

clean:
	rm -f $(OBJ)*.o $(SRC)*~ $(BIN)* $(DIR)/*~

