MACH_CACHE_LINE=`/bin/bash ./conf`
CC = gcc -g -Wall -O0 -I${ORACLE_HOME}/precomp/public -I${ORACLE_HOME}/rdbms/public -DLINUX
PCLINK = proc include=${ORACLE_HOME}/precomp/public UNSAFE_NULL=YES DBMS=V8 MODE=ORACLE char_map=string define=LINUX parse=none
LINKOBJ = $(CC) -L/usr/lib -L${ORACLE_HOME}/lib -lclntsh 
OBJS = palloc.o log.o string_hash.o test.o

all:HASH

HASH:$(OBJS)
	$(LINKOBJ) -o $@ $^ -DMACH_CACHE_LINE=${MACH_CACHE_LINE} 

.c.o:
	@echo $(CC) -c $< -DMACH_CACHE_LINE=${MACH_CACHE_LINE}
	$(CC) -c $< -DMACH_CACHE_LINE=${MACH_CACHE_LINE}

test.o: test.pc
	$(PCLINK) iname=test
	$(CC) -c test.c -DMACH_CACHE_LINE=${MACH_CACHE_LINE}

clean:
	rm -f *.o HASH tp* *.lis test.c
