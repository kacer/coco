# Colearning in Coevolutionary Algorithms
# Bc. Michal Wiglasz <xwigla00@stud.fit.vutbr.cz>
#
# Master Thesis
# 2014/2015
#
# Supervisor: Ing. Michaela Šikulová <isikulova@fit.vutbr.cz>
#
# Faculty of Information Technologies
# Brno University of Technology
# http://www.fit.vutbr.cz/
#
# Started on 28/07/2014.
#      _       _
#   __(.)=   =(.)__
#   \___)     (___/

CC=gcc
CFLAGS=-g -Wall -std=gnu99 -DDEBUG -DGA_USE_PTHREAD -DxFITNESS_USE_PTHREAD -DNCURSES -D_XOPEN_SOURCE=700
EXECUTABLE=coco
SERVER=merlin
SERVER_LOCATION=~/coco
CMDLINE=
SOURCES=main.c ga.c cgp.c cgp_dump.c cgp_load.c image.c fitness.c vault.c windows.c
OFILES= main.o ga.o cgp.o cgp_dump.o cgp_load.o image.o fitness.o vault.o windows.o
LIBS=-lm -lc -lpthread -lncurses

all: $(EXECUTABLE)

run: $(EXECUTABLE)
	./$(EXECUTABLE) $(CMDLINE)

$(EXECUTABLE): $(OFILES)
	$(CC) $(CFLAGS) -o $@ $(OFILES) $(LIBS)

.PHONY: clean zip tar upload start depend

clean:
	rm -f $(EXECUTABLE) $(EXECUTABLE).exe *.o xwigla00.zip xwigla00.tar.gz

tar:
	tar czf xwigla00.tar.gz *.c *.h Makefile tests/* stb/* images/* #-C doc/ doc.pdf

upload: tar
	scp xwigla00.tar.gz $(SERVER):$(SERVER_LOCATION)
	ssh $(SERVER) "cd $(SERVER_LOCATION) && tar -xzf xwigla00.tar.gz && make"

download:
	ssh $(SERVER) "cd $(SERVER_LOCATION) && tar -czf results.tar.gz results/*.bmp"
	scp $(SERVER):$(SERVER_LOCATION)/results.tar.gz .
	tar -xzf results.tar.gz

start:
	ssh $(SERVER) "$(SERVER_LOCATION)/$(EXECUTABLE) $(CMDLINE)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

depend: .depend

.depend: $(SOURCES)
	rm -f ./.depend
	$(CC) $(CFLAGS) -MM $(SOURCES) >>./.depend;

include .depend

