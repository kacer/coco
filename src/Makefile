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


# _XOPEN_SOURCE=700 required by scandir and alphasort from vault.c

CC=gcc
CFLAGS=-g -Wall -std=c11 -fopenmp -march=native -D_XOPEN_SOURCE=700 -O2 \
	-DSSE2 -DxAVX2 -DDEBUG -DxVERBOSE -DCGP_LIMIT_FUNCS
EXECUTABLE=coco
CMDLINE=-i ../images/lena_gray_256.png -n ../images/lena_gray_256_saltpepper_10.png -a cgp -g 5000
SERVER=merlin
SERVER_LOCATION=~/coco
SOURCES=main.c cpu.c ga.c cgp_core.c cgp_dump.c cgp_load.c cgp_avx.c cgp_sse.c \
	predictors.c image.c fitness.c fitness_avx.c fitness_sse.c vault.c \
	archive.c config.c logging.c algo.c
OFILES= main.o cpu.o ga.o cgp_core.o cgp_dump.o cgp_load.o cgp_avx.o cgp_sse.o \
	predictors.o image.o fitness.o fitness_avx.o fitness_sse.o vault.o \
	archive.o config.o logging.o algo.o
LIBS=-lm -lc

all: $(EXECUTABLE)

run: $(EXECUTABLE)
	rm -rf cocolog/*
	./$(EXECUTABLE) $(CMDLINE)

valgrind: $(EXECUTABLE)
	rm -rf cocolog/*
	valgrind --track-origins=yes ./$(EXECUTABLE) $(CMDLINE)

$(EXECUTABLE): $(OFILES)
	$(CC) $(CFLAGS) -o $@ $(OFILES) $(LIBS)

apply: image.o ga.o cgp_core.o cgp_load.o main_apply.o
	$(CC) $(CFLAGS) -o coco_apply image.o ga.o cgp_core.o cgp_load.o main_apply.o $(LIBS)

.PHONY: clean zip tar upload start depend

clean:
	rm -rf cocolog/*
	rm -f $(EXECUTABLE) $(EXECUTABLE).exe $(EXECUTABLE).exe.stackdump *.o
	rm -f xwigla00.zip xwigla00.tar.gz

tar:
	cd .. && tar czf xwigla00.tar.gz src/*.c src/*.h src/Makefile src/tests/* src/stb/* images/* #-C doc/ doc.pdf

upload: tar
	scp ../xwigla00.tar.gz $(SERVER):$(SERVER_LOCATION)
	ssh $(SERVER) "cd $(SERVER_LOCATION) && tar -xzf xwigla00.tar.gz && cd src && make"

download:
	ssh $(SERVER) "cd $(SERVER_LOCATION) && tar -czf results.tar.gz results/*.bmp"
	scp $(SERVER):$(SERVER_LOCATION)/results.tar.gz .
	tar -xzf results.tar.gz

start:
	ssh $(SERVER) "$(SERVER_LOCATION)/$(EXECUTABLE) $(CMDLINE)"

# AVX2 support

cgp_avx.o: cgp_avx.c
	$(CC) $(CFLAGS) -mavx2 -c $< -o $@

fitness_avx.o: fitness_avx.c
	$(CC) $(CFLAGS) -mavx2 -c $< -o $@

# SSE2 support

cgp_sse.o: cgp_sse.c
	$(CC) $(CFLAGS) -msse2 -c $< -o $@

fitness_sse.o: fitness_sse.c
	$(CC) $(CFLAGS) -msse2 -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

depend: .depend

.depend: $(SOURCES)
	rm -f ./.depend
	$(CC) $(CFLAGS) -MM $(SOURCES) >>./.depend;

include .depend

