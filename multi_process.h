#ifndef _MULTI_PROCESS_H_INCLUCE_
#define _MULTI_PROCESS_H_INCLUCE_
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

#define  CHLD_EXIT   '0'
#define  CHLD_ACTIVE '1'


typedef struct {
  pid_t     pid[128];
  char      status[128];
  intptr_t  cur_cnt;
  intptr_t  exist_cnt;
} proc_status_t;

typedef intptr_t (*chld_func)(void *param);

void multi_process_init();

void sig_child_exit(int signo);

void sig_father_exit(int signo);

void sig_chld(int signo); 

int
mutli_process(int idx, chld_func func, void *parm);

#endif
