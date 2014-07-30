CC=gcc
CFLAGS=-g -Wall -std=c11 -DDEBUG
EXECUTABLE=coco
SERVER=merlin
SERVER_LOCATION=~/coco
CMDLINE=-h
SOURCES=main.c cgp.c cgp_dump.c
OFILES=main.o cgp.o cgp_dump.o
LIBS=

all: $(EXECUTABLE)

run: $(EXECUTABLE)
	./$(EXECUTABLE) $(CMDLINE)

$(EXECUTABLE): $(OFILES)
	$(CC) $(CFLAGS) -o $@ $(OFILES) $(LIBS)

.PHONY: clean zip tar upload start depend

clean:
	rm -f $(EXECUTABLE) *.o xwigla00.zip xwigla00.tar

zip:
	zip -j xwigla00.zip *.c *.h *.py Makefile  #doc/doc.pdf

tar:
	tar cvzf xwigla00.tar.gz *.cpp *.h *.py Makefile #-C doc/ doc.pdf

upload: tar
	scp xwigla00.tar.gz $(SERVER):$(SERVER_LOCATION)
	ssh $(SERVER) "cd $(SERVER_LOCATION) && tar -xvzf xwigla00.tar.gz && make"

start:
	ssh $(SERVER) "$(SERVER_LOCATION)/$(EXECUTABLE) $(CMDLINE)"

%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

depend: .depend

.depend: $(SOURCES)
	rm -f ./.depend
	$(CC) $(CFLAGS) -MM $(SOURCES) >>./.depend;

include .depend

