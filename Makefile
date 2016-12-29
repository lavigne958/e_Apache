CC = gcc
CFLAGS = -W -Wall -ansi -pedantic -Iinclude
LDFLAGS = -lpthread

DIR=.
INCLUDE=$(DIR)/include/
DEST=/tmp/3200583/
SRC=$(DIR)/src/

all: $(DEST)server $(DEST)test_cgi

test: $(DEST)server $(DEST)test_cgi
	@if [ -d $(DEST) ]; then : ; else mkdir $(DEST); fi
	$(DEST)server 8080 3 35000

$(DEST)server: $(DEST)server.o $(DEST)requete.o $(DEST)vigilante.o
	@if [ -d $(DEST) ]; then : ; else mkdir $(DEST); fi
	$(CC) -o $@ $^ $(LDFLAGS);

$(DEST)test_cgi: $(DEST)test_pipe_prog.o
	@if [ -d $(DEST) ]; then : ; else mkdir $(DEST); fi
	$(CC) -o $@ $^ $(LDFLAGS);

$(DEST)%.o: $(SRC)%.c
	@if [ -d $(DEST) ]; then : ; else mkdir $(DEST); fi
	$(CC) $(CFLAGS) -o $@ -c $< $(LDFLAGS)

clean:
	rm -f *.o $(SRC)*~ $(DIR)/*~ $(DEST)* 
