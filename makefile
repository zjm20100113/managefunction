$(shell chmod +x ./conf)
$(shell ./conf 1>&2)
include config.mk

CC = cc 
CFLAG= -g -Wall -O3  -DLINUX -lpthread
OBJS = multi_process.o share_memory.o atomic_mutex_lock.o palloc.o log.o string_hash.o test.o

all:TEST

TEST:$(OBJS)
	$(CC) -o $@ $^ -DCACHE_LINE=${CACHE_LINE} $(CFLAG)

.c.o:
	@echo $(CC) -c $< -DCACHE_LINE=${CACHE_LINE} $(CFLAG)
	$(CC) -c $< -DCACHE_LINE=${CACHE_LINE} $(CFLAG)

clean:
	rm -f *.o HASH tp* *.lis *.gch TEST testlog.tr
