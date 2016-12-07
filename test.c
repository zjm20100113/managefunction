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
shmm_t  shm;
mutex_t mtx;
log_t   file_log;
log_t  *flog = NULL;


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
  log_msg(flog, 0, "child recv the signo %d\n", signo);
  exit(0);
}

void sig_father_exit(int signo)
{
  log_msg(flog, 0, "father recv the signo %d\n", signo);
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

intptr_t gobal_number_add(void *num)
{
  pid_t pid = getpid();
  atomic_t *number = (atomic_t *) num;
  unsigned long oldval = 0;

  while (*number < 20) {

    /** if (mutex_trylock(&mtx, pid) == OK) { */
    if (mutex_lock(&mtx, pid) == OK) {
    
      oldval = *number;
      *number = *number + 1;

      log_msg(flog, 0, "pid[%P] oldvalue=%ul, newvalue=%ul \n", pid, oldval, *number);

      mutex_unlock(&mtx, pid);
    }
  }
  return 0;
}

int main(int argc, char **argv) 
{
  pool_t             *pool = NULL;
  int                 r;
  hash_init_t         hinit;
  size_t              record_cnt = 0;

  flog = &file_log;

  pool = create_pool(1024 * 5);
  if (NULL == pool) {
    printf("create pool failed \n");
    exit(-2);
  }


  flog->file.file_path.data = (char *)"./testlog.tr";
  flog->file.file_path.len = strlen("./testlog.tr");

  log_open(flog);

  log_msg(flog, 0, "cache size = %d ", cache_size);

  hinit.max_size = 70000000;
  hinit.bucket_size = 64;
  hinit.key = string_hash_key_char;
  hinit.pool = pool;
  hinit.hash = NULL;

  prepare_crypt_table();

  record_cnt = 16;  

  if (0 >= record_cnt) {
    log_msg(flog, -1, "record count is zero");
    return ERROR;
  }

  log_msg(flog, 0, "total count of record is %z", record_cnt);

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


  log_msg(flog, 0, "read datas done");

  hash_init(&hinit, hash_user, i); 
  
  user_t *puser = NULL;

  log_msg(flog, 0, "hash initdone");
 
  int j = 0; 

  for (j = 0; j < i; j++) {
    puser = (user_t *) hash_find(&hinit, test_data[j].user_id);

    if (NULL == puser) {
      log_msg(flog, -2, "userid=%s data is losed", test_data[j].user_id);
    }

    if (NULL != puser) {
      log_msg(flog, 0, "userid= %s, acct_id=%s",
          puser->user_id, puser->acct_id);
    }

  }
  
  destroy_pool(pool);



  gobal_child_status.cur_cnt = 0;
  gobal_child_status.exist_cnt = 0;
  memset(gobal_child_status.pid, 0, sizeof(gobal_child_status.pid));
  memset(gobal_child_status.status, 0, sizeof(gobal_child_status.status));

  shm.size = 256;
  /** mtx.spin = (uintptr_t) -1; */
  mtx.spin = 0;

  create_share_memory(&shm);

  atomic_t *addr, *wait, *num;
  addr = (atomic_t *)shm.addr;
  wait = (atomic_t *)(shm.addr + 128);
  num = (atomic_t *)(shm.addr + 128 + 64);

  *wait = 0;
  *addr = 0;

  mutex_create(&mtx, addr, wait); 

  i = 0;

  for ( ; i < 2; i++) {
    mutli_process(i, gobal_number_add, (void *)num);  
  }

  while (gobal_child_status.exist_cnt != gobal_child_status.cur_cnt) {
    sleep(1);
  }

  i = 0;

  for (; i < gobal_child_status.cur_cnt; i++) {
    log_msg(flog, 0, "idx:%d pid:%P status:%c \n", i, gobal_child_status.pid[i],
                                                   gobal_child_status.status[i]);
  }

  free_share_memory(&shm);

  return 0;
}
