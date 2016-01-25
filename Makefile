#You can use either a gcc or g++ compiler
#CC = g++
CC = gcc
EXECUTABLES = test_basic test_coalesce test_stress1 test_stress2
CFLAGS = -I. -Wall -lm -DNDEBUG
#Disable the -DNDEBUG flag for the printing the freelist
#CFLAGS = -Wall -I.
PTFLAG = -O2
DEBUGFLAG = -g

all: ${EXECUTABLES}

test: CFLAGS += $(OPTFLAG)
test: ${EXECUTABLES}
	for exec in ${EXECUTABLES}; do \
    		./$$exec ; \
	done

debug: CFLAGS += $(DEBUGFLAG)
debug: $(EXECUTABLES)
	for dbg in ${EXECUTABLES}; do \
		gdb ./$$dbg ; \
	done

test_basic: test_basic.c dmm.o
	$(CC) $(CFLAGS) -o test_basic test_basic.c dmm.o
test_coalesce: test_coalesce.c dmm.o
	$(CC) $(CFLAGS) -o test_coalesce test_coalesce.c dmm.o
test_stress1: test_stress1.c dmm.o
	$(CC) $(CFLAGS) -o test_stress1 test_stress1.c dmm.o
test_stress2: test_stress2.c dmm.o
	$(CC) $(CFLAGS) -o test_stress2 test_stress2.c dmm.o
dmm.o: dmm.c
	$(CC) $(CFLAGS) -c dmm.c 
clean:
	rm -f *.o ${EXECUTABLES} a.out
