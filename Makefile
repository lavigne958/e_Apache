CC = gcc
CFLAGS = -W -Wall -ansi -pedantic -Iinclude
LDFLAGS = -lpthread

DIR=.
INCLUDE=$(DIR)/include/
DEST=/tmp/3200583-3502363/
SRC=$(DIR)/src/

all: $(DEST)server $(DEST)test_cgi

test: $(DEST)server $(DEST)test_cgi
	@if [ -d $(DEST) ]; then : ; else mkdir $(DEST); fi
	@cp -r index.html edt.pdf favicon.ico img.jpg $(DEST)
	cd $(DEST) && $(DEST)server 8080 3 5000000

$(DEST)server: $(DEST)server.o $(DEST)requete.o $(DEST)vigilante.o
	@if [ -d $(DEST) ]; then : ; else mkdir $(DEST); fi
	$(CC) -o $@ $^ $(LDFLAGS);

$(DEST)test_cgi: $(DEST)test_cgi.o
	@if [ -d $(DEST) ]; then : ; else mkdir $(DEST); fi
	$(CC) -o $@ $^ $(LDFLAGS);

$(DEST)%.o: $(SRC)%.c
	@if [ -d $(DEST) ]; then : ; else mkdir $(DEST); fi
	$(CC) $(CFLAGS) -o $@ -c $< $(LDFLAGS)

clean:
	rm -f *.o $(SRC)*~ $(DIR)/*~
	rm -rf $(DEST)
	rm -f rapport/*.aux rapport/*.log
