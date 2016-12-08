#include "multi_process.h"


proc_status_t gobal_child_status;

void sig_child_exit(int signo)
{
  exit(0);
}

void sig_father_exit(int signo)
{
  kill(0, 15);
  exit(0);
}

void sig_chld(int signo) 
{
  pid_t     pid;
  int       i, status;

  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    
    gobal_child_status.exist_cnt++;

    for (i = 0; i < gobal_child_status.cur_cnt; i++) {

      if (gobal_child_status.pid[i] == pid) {
        gobal_child_status.status[i] = CHLD_EXIT; 
        break;
      }
    }
  }
}

int
mutli_process(int idx, chld_func func, void *parm)
{
  setpgrp();

  struct sigaction act_chld, act_father, act_child;
  act_father.sa_handler = sig_father_exit;
  sigemptyset(&act_father.sa_mask);
  act_father.sa_flags = 0;

  sigaction(SIGQUIT, &act_father, NULL);
  sigaction(SIGINT, &act_father, NULL);
  sigaction(SIGTERM, &act_father, NULL);

  act_chld.sa_handler = sig_chld;
  sigemptyset(&act_chld.sa_mask);
  act_chld.sa_flags = 0;

  sigaction(SIGCHLD, &act_chld, NULL);

  pid_t pid = -1;

  pid = fork();

  if (pid > 0) {

    gobal_child_status.pid[gobal_child_status.cur_cnt] = pid;
    gobal_child_status.status[gobal_child_status.cur_cnt] = CHLD_ACTIVE;
    gobal_child_status.cur_cnt++;
    return 0;
  } else if (0 == pid) {

    act_child.sa_handler = sig_child_exit;
    sigemptyset(&act_child.sa_mask);
    act_child.sa_flags = 0;

    sigaction(SIGQUIT, &act_child, NULL);
    sigaction(SIGINT, &act_child, NULL);
    sigaction(SIGTERM, &act_child, NULL);

    func(parm);

    exit(0);
  } else {
    return -1;
  }
}

void multi_process_init()
{
  gobal_child_status.cur_cnt = 0;
  gobal_child_status.exist_cnt = 0;
  memset(gobal_child_status.pid, 0, sizeof(gobal_child_status.pid));
  memset(gobal_child_status.status, 0, sizeof(gobal_child_status.status));
}
