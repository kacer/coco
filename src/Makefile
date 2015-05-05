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
# _XOPEN_SOURCE>=600 required by image.c for aligned allocation

CC=gcc
CFLAGS=-g -Wall -std=c11 -fopenmp -O2 -D_XOPEN_SOURCE=700 \
	-DSSE2 -DxAVX2 -DDEBUG -DxVERBOSE -DxCGP_LIMIT_FUNCS
LIBS=-lm -lc

SRCDIR=.
BUILDDIR=build

# for .depend only
SRCS=\
	main.c cpu.c ga.c cgp/cgp_core.c cgp/cgp_dump.c cgp/cgp_load.c \
	fitness.c predictors.c archive.c config.c algo.c baldwin.c utils.c \
	logging/history.c logging/base.c logging/text.c logging/csv.c \
	logging/summary.c logging/predictor.c \

IFILTER_SRCS=$(SRCS) ifilter/cgp.c ifilter/inputdata.c ifilter/fitness.c \
	ifilter/cgp_sse.c ifilter/fitness_sse.c \
	ifilter/cgp_avx.c ifilter/fitness_avx.c \
	ifilter/image.c

SYMREG_SRCS=$(SRCS) symreg/cgp.c symreg/inputdata.c symreg/fitness.c

IFILTER_CFLAGS=$(CFLAGS) -DCGP_COLS=8 -DCGP_ROWS=4 -DCGP_LBACK=1
IFILTER_EXECUTABLE=coco
IFILTER_BUILDDIR=$(BUILDDIR)/ifilter
IFILTER_OBJS = $(IFILTER_SRCS:%.c=$(IFILTER_BUILDDIR)/%.o)
IFILTER_DEPS = $(IFILTER_SRCS:%.c=$(IFILTER_BUILDDIR)/%.d)
IFILTER_CMDLINE=-i ../images/lena_gray_256.png -n ../images/lena_gray_256_saltpepper_25.png -g 100000 -a baldwin \
	-S 100 -I 0.05 -k 10000 -l cocolog --log-pred-file cocolog/predictors.log \
	--bw-inac-tol 1.2 --bw-inac-coef 2 --bw-zero-eps 0.001 --bw-zero-coef 0.9 --bw-decr-coef 0.96 --bw-slow-thr 0.1 --bw-slow-coef 1.07 --bw-fast-coef 1 --bw-interval 0 \
	--pred-type repeated

SYMREG_CFLAGS=$(CFLAGS) -DSYMREG -DCGP_COLS=32 -DCGP_ROWS=1 -DCGP_LBACK=32
SYMREG_EXECUTABLE=coco_symreg
SYMREG_BUILDDIR=$(BUILDDIR)/symreg
SYMREG_OBJS = $(SYMREG_SRCS:%.c=$(SYMREG_BUILDDIR)/%.o)
SYMREG_DEPS = $(SYMREG_SRCS:%.c=$(SYMREG_BUILDDIR)/%.d)
SYMREG_CMDLINE=-g 100000 -k 10000 -l cocolog \
	-i ../functions/f004.dta -e 0.025 \
	-a baldwin -S 100 -I 3 \
	-f 100 \
	-p 12 -m 8 \
	-P 32 -M 0.2 \
	--bw-zero-eps 0.5 \
	--bw-slow-thr 0.1 \
	--bw-zero-coef 0.9 \
	--bw-decr-coef 0.96 \
	--bw-slow-coef 1.07 \
	--bw-fast-coef 1 \
	--bw-interval 1000

APPLY_CFLAGS=$(CFLAGS)
APPLY_EXECUTABLE=coco_apply
APPLY_BUILDDIR=$(IFILTER_BUILDDIR)
APPLY_OBJS=$(IFILTER_BUILDDIR)/ifilter/image.o $(IFILTER_BUILDDIR)/ga.o \
	$(IFILTER_BUILDDIR)/cgp/cgp_core.o $(IFILTER_BUILDDIR)/cgp/cgp_load.o \
	$(IFILTER_BUILDDIR)/cgp/cgp_dump.o $(IFILTER_BUILDDIR)/ifilter/cgp.o \
	$(IFILTER_BUILDDIR)/ifilter/main_apply.o
APPLY_DEPS = $(APPLY_OBJS:%.o=%.d)

PREDVIS_CFLAGS=$(CFLAGS)
PREDVIS_EXECUTABLE=coco_predvis
PREDVIS_BUILDDIR=$(IFILTER_BUILDDIR)
PREDVIS_OBJS=$(IFILTER_BUILDDIR)/ifilter/image.o \
	$(IFILTER_BUILDDIR)/utils.o $(IFILTER_BUILDDIR)/cpu.o \
	$(IFILTER_BUILDDIR)/ifilter/main_predvis.o
PREDVIS_DEPS = $(PREDVIS_OBJS:%.o=%.d)

PREDHIST_CFLAGS=$(CFLAGS)
PREDHIST_EXECUTABLE=coco_predhist
PREDHIST_BUILDDIR=$(IFILTER_BUILDDIR)
PREDHIST_OBJS=$(IFILTER_BUILDDIR)/ifilter/image.o \
	$(IFILTER_BUILDDIR)/ifilter/main_predhist.o
PREDHIST_DEPS = $(PREDVIS_OBJS:%.o=%.d)

EXECUTABLES=$(IFILTER_EXECUTABLE) $(APPLY_EXECUTABLE) $(SYMREG_EXECUTABLE) $(PREDVIS_EXECUTABLE) $(PREDHIST_EXECUTABLE)

ANSELM_HOST=anselm
ANSELM_PATH=~/xwigla00
MERLIN_HOST=merlin
MERLIN_PATH=~/coco

.PHONY: clean run minirun zip tar upload start depend anselmup anselmdown merlinup rebuild callgraph

all: $(EXECUTABLES)

clean:
	rm -rf build
	rm -rf cocolog/*
	rm -f $(IFILTER_EXECUTABLE) $(IFILTER_EXECUTABLE).exe $(IFILTER_EXECUTABLE).exe.stackdump
	rm -f $(APPLY_EXECUTABLE) $(APPLY_EXECUTABLE).exe $(APPLY_EXECUTABLE).exe.stackdump
	rm -f $(PREDVIS_EXECUTABLE) $(PREDVIS_EXECUTABLE).exe $(PREDVIS_EXECUTABLE).exe.stackdump
	rm -f $(PREDHIST_EXECUTABLE) $(PREDHIST_EXECUTABLE).exe $(PREDHIST_EXECUTABLE).exe.stackdump
	rm -f $(SYMREG_EXECUTABLE) $(SYMREG_EXECUTABLE).exe $(SYMREG_EXECUTABLE).exe.stackdump
	rm -f xwigla00.zip xwigla00.tar.gz
	find -name '*.expand' | xargs rm -f

rebuild: clean all

$(IFILTER_EXECUTABLE): $(IFILTER_OBJS)
	$(CC) $(IFILTER_CFLAGS) -o $@ $(IFILTER_OBJS) $(LIBS)

$(SYMREG_EXECUTABLE): $(SYMREG_OBJS)
	$(CC) $(SYMREG_CFLAGS) -o $(SYMREG_EXECUTABLE) $(SYMREG_OBJS) $(LIBS)

$(APPLY_EXECUTABLE): $(APPLY_OBJS)
	$(CC) $(APPLY_CFLAGS) -o $(APPLY_EXECUTABLE) $(APPLY_OBJS) $(LIBS)

$(PREDVIS_EXECUTABLE): $(PREDVIS_OBJS)
	$(CC) $(PREDVIS_CFLAGS) -o $(PREDVIS_EXECUTABLE) $(PREDVIS_OBJS) $(LIBS)

$(PREDHIST_EXECUTABLE): $(PREDHIST_OBJS)
	$(CC) $(PREDHIST_CFLAGS) -o $(PREDHIST_EXECUTABLE) $(PREDHIST_OBJS) $(LIBS)

run: $(IFILTER_EXECUTABLE)
	rm -rf cocolog/*
	./$(IFILTER_EXECUTABLE) $(IFILTER_CMDLINE)

symrun: $(SYMREG_EXECUTABLE)
	rm -rf cocolog/*
	./$(SYMREG_EXECUTABLE) $(SYMREG_CMDLINE)

minirun: $(IFILTER_EXECUTABLE)
	rm -rf cocolog/*
	./$(IFILTER_EXECUTABLE) -i ../images/10x10.png -n ../images/10x10_sp25.png -a baldwin -g 1000 -S 75 -I 50 -N 50

# deployment

tar:
	cd .. && tar czf xwigla00.tar.gz src/*.c src/*.h src/*/*.c src/*/*.h src/Makefile src/tests/* src/stb/* # images/* #-C doc/ doc.pdf

anselmup: tar
	find -type f | xargs chmod ug+rw
	find -type d | xargs chmod ug+rwx
	scp ../xwigla00.tar.gz $(ANSELM_HOST):$(ANSELM_PATH)
	ssh $(ANSELM_HOST) "cd $(ANSELM_PATH) && tar -xzvf xwigla00.tar.gz && cd src && module load gcc/4.9.0 && make clean all"

merlinup: tar
	find -type f | xargs chmod ug+rw
	find -type d | xargs chmod ug+rwx
	scp ../xwigla00.tar.gz $(MERLIN_HOST):$(MERLIN_PATH)
	ssh $(MERLIN_HOST) "cd $(MERLIN_PATH) && tar -xzvf xwigla00.tar.gz && cd src && make clean all"

# some stuff to increase average developer happiness

plot:
	gnuplot cocolog.gp

symplot:
	gnuplot cocolog_symreg.gp

valgrind: $(IFILTER_EXECUTABLE)
	rm -rf cocolog/*
	valgrind --track-origins=yes --leak-check=full ./$(IFILTER_EXECUTABLE) $(IFILTER_CMDLINE)

gdb: $(IFILTER_EXECUTABLE)
	rm -rf cocolog/*
	gdb --args ./$(IFILTER_EXECUTABLE) $(IFILTER_CMDLINE)

leaks: $(IFILTER_EXECUTABLE)
	rm -rf cocolog/*
	valgrind --track-origins=yes --leak-check=full --show-reachable=yes ./$(IFILTER_EXECUTABLE) $(IFILTER_CMDLINE)

callgraph:
	$(MAKE) CFLAGS='$(CFLAGS) -fdump-rtl-expand' clean all
	find -name '*.expand' | grep -v 'main_apply' | xargs egypt --omit stbi_load,stbi_write_png,log_entry_prolog,can_use_sse2 | dot -Grankdir=LR  -Tpng -o callgraph.png
	rm *.expand cgp/*.expand logging/*.expand

# rules to build image filters

$(IFILTER_BUILDDIR)/%_avx.o: %_avx.c
	@mkdir -p `dirname $@`
	@echo CC -mavx2 $@
	@$(CC) $(IFILTER_CFLAGS) -mavx2 -c $< -o $@

$(IFILTER_BUILDDIR)/%_sse.o: %_sse.c
	@mkdir -p `dirname $@`
	@echo CC -msse2 $@
	@$(CC) $(IFILTER_CFLAGS) -msse2 -c $< -o $@

$(IFILTER_BUILDDIR)/%.o: %.c
	@mkdir -p `dirname $@`
	@echo CC $@
	@$(CC) $(IFILTER_CFLAGS) -c $< -o $@

$(IFILTER_BUILDDIR)/%.d: %.c
	@mkdir -p `dirname $@`
	@echo CC -MM $@
	@@$(CC) $(IFILTER_CFLAGS) -MM -MT $(<:%.c=$(IFILTER_BUILDDIR)/%.o) $< -o $@

-include $(IFILTER_DEPS)
-include $(APPLY_DEPS)
-include $(PREDVIS_DEPS)

# rules to build symbolic regression

$(SYMREG_BUILDDIR)/%_avx.o: %_avx.c
	@mkdir -p `dirname $@`
	@echo CC -mavx2 $@
	@$(CC) $(SYMREG_CFLAGS) -mavx2 -c $< -o $@

$(SYMREG_BUILDDIR)/%_sse.o: %_sse.c
	@mkdir -p `dirname $@`
	@echo CC -msse2 $@
	@$(CC) $(SYMREG_CFLAGS) -msse2 -c $< -o $@

$(SYMREG_BUILDDIR)/%.o: %.c
	@mkdir -p `dirname $@`
	@echo CC $@
	@$(CC) $(SYMREG_CFLAGS) -c $< -o $@

$(SYMREG_BUILDDIR)/%.d: %.c
	@mkdir -p `dirname $@`
	@echo CC -MM $@
	@@$(CC) $(SYMREG_CFLAGS) -MM -MT $(<:%.c=$(SYMREG_BUILDDIR)/%.o) $< -o $@

-include $(SYMREG_DEPS)
