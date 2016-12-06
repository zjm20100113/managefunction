#include "string_hash.h"
#include "log.h"
#include "share_memory.h"
#include "atomic_mutex_lock.h"
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define  CHLD_EXIT  '0'
#define  CHLD_ACTIVE '1'

extern int cache_size;

typedef struct {
  char user_id[17];
  char acct_id[17];
} user_t;

typedef struct {
  pid_t     pid[128];
  char      status[128];
  intptr_t  cur_cnt;
  intptr_t  exist_cnt;
} proc_status_t;

typedef intptr_t (*chld_func)(void *param);

user_t test_data[] = {
   {"391117", "103102527"}, {"391118", "103102528"},
   {"391119", "103102529"}, {"391122", "103102531"},
   {"391123", "103102532"}, {"391124", "103102533"},
   {"391125", "103102534"}, {"391126", "103102535"},
   {"391127", "103102536"}, {"391128", "103102537"},
   {"391129", "103102538"}, {"391130", "103102539"},
   {"391131", "103102540"}, {"391132", "103102541"},
   {"391133", "103102542"}, {"391134", "103102543"}
};

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

void sig_chld(int signo) {
  pid_t     pid;
  int       i, status;

  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    
    gobal_child_status.exist_cnt++;

    for (i = 0; i < gobal_child_status.cur_cnt; i++) {

      if (gobal_child_status.pid[i] == pid) {
        gobal_child_status.status[i] = CHLD_ACTIVE; 
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

  sigaction(SIGQUIT, &act_father, 0);
  sigaction(SIGINT, &act_father, 0);
  sigaction(SIGTERM, &act_father, 0);

  act_chld.sa_handler = sig_chld;
  sigemptyset(&act_chld.sa_mask);
  act_chld.sa_flags = 0;

  sigaction(SIGCHLD, &act_chld, 0);

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

    sigaction(SIGQUIT, &act_child, 0);
    sigaction(SIGINT, &act_child, 0);
    sigaction(SIGTERM, &act_child, 0);

    func(parm);

    exit(0);
  } else {
    return -1;
  }
}

int main(int argc, char **argv) 
{
  pool_t             *pool = NULL;
  log_t              *log;
  int                 r;
  hash_init_t         hinit;
  size_t              record_cnt = 0;


  pool = create_pool(1024 * 5);
  if (NULL == pool) {
    printf("create pool failed \n");
    exit(-2);
  }

  log = (log_t *)pcalloc(pool, sizeof(log_t));
  if (NULL == log) {
    exit(-2);
  }

  log->file.file_path.data = (char *)"./testlog.tr";
  log->file.file_path.len = strlen("./testlog.tr");

  log_open(log);

  log_msg(log, 0, "cache size = %d ", cache_size);

  hinit.max_size = 70000000;
  hinit.bucket_size = 64;
  hinit.key = string_hash_key_char;
  hinit.pool = pool;
  hinit.hash = NULL;

  prepare_crypt_table();

  record_cnt = 16;  

  if (0 >= record_cnt) {
    log_msg(log, -1, "record count is zero");
    return ERROR;
  }

  log_msg(log, 0, "total count of record is %z", record_cnt);

  hash_key_t *hash_user = (hash_key_t *) pcalloc(pool, 
                            record_cnt * sizeof(hash_key_t)); 
  if (NULL == hash_user) {
    exit(-1);
  }
  
  for (r = 0; r < record_cnt; r++) {
    hash_user[r].key = NULL;
    hash_user[r].value = NULL;
  }


  int i =0;

  for (; i < record_cnt; i++) {
       
    hash_user[i].key = test_data[i].user_id;
    hash_user[i].key_hash = hinit.key(test_data[i].user_id, 16);
    hash_user[i].value = (void *)( &test_data[i]);
  }


  log_msg(log, 0, "read datas done");

  hash_init(&hinit, hash_user, i); 
  
  user_t *puser = NULL;

  log_msg(log, 0, "hash initdone");
 
  int j = 0; 

  for (j = 0; j < i; j++) {
    puser = (user_t *) hash_find(&hinit, test_data[j].user_id);

    if (NULL == puser) {
      log_msg(log, -2, "userid=%s data is losed", test_data[j].user_id);
    }

    if (NULL != puser) {
      log_msg(log, 0, "userid= %s, acct_id=%s",
          puser->user_id, puser->acct_id);
    }

  }
  
  destroy_pool(pool);



  gobal_child_status.cur_cnt = 0;
  gobal_child_status.exist_cnt = 0;
  memset(gobal_child_status.pid, 0, sizeof(gobal_child_status.pid));
  memset(gobal_child_status.status, 0, sizeof(gobal_child_status.status));

  shmm_t shm;
  mutex_t mtx;

  shm.size = 256;

  create_share_memory(&shm);
  
  /** mutex_create(&mtx, (atomic_t *)shm.addr, (atomic_t *)); */

  return 0;
}
