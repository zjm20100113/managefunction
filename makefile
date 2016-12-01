MACH_CACHE_LINE=`/bin/bash ./conf`
CC = gcc -g -Wall -O3  -DLINUX
OBJS = palloc.o log.o string_hash.o test.o

all:HASH

HASH:$(OBJS)
	$(CC) -o $@ $^ -DMACH_CACHE_LINE=${MACH_CACHE_LINE} 

.c.o:
	@echo $(CC) -c $< -DMACH_CACHE_LINE=${MACH_CACHE_LINE}
	$(CC) -c $< -DMACH_CACHE_LINE=${MACH_CACHE_LINE}

clean:
	rm -f *.o HASH tp* *.lis 
