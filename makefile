MACH_CACHE_LINE=`/bin/bash ./conf`
CC = cc 
CFLAG= -g -Wall -O3  -DLINUX -lpthread
OBJS = multi_process.o share_memory.o atomic_mutex_lock.o palloc.o log.o string_hash.o test.o

all:TEST

TEST:$(OBJS)
	$(CC) -o $@ $^ -DMACH_CACHE_LINE=${MACH_CACHE_LINE} $(CFLAG)

.c.o:
	@echo $(CC) -c $< -DMACH_CACHE_LINE=${MACH_CACHE_LINE} $(CFLAG)
	$(CC) -c $< -DMACH_CACHE_LINE=${MACH_CACHE_LINE} $(CFLAG)

clean:
	rm -f *.o HASH tp* *.lis *.gch TEST testlog.tr
