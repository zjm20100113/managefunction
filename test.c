#include "string_hash.h"
#include "log.h"
#include "share_memory.h"
#include "atomic_mutex_lock.h"
#include "multi_process.h"

extern int cache_size;
extern proc_status_t gobal_child_status;
shmm_t  shm;
mutex_t mtx;
log_t   file_log;
log_t  *flog = NULL;


typedef struct {
  char user_id[17];
  char acct_id[17];
} user_t;


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

intptr_t gobal_number_add(void *num)
{
  pid_t pid = getpid();
  atomic_t *number = (atomic_t *) num;
  unsigned long oldval = 0;
  int  deal_cnt = 0;
  int  sleep_time = 500;

  while (*number < 20) {

    /** if (mutex_trylock(&mtx, pid) == OK) { */
    if (mutex_lock(&mtx, pid) == OK) {

      if (*number < 20) {
        oldval = *number;
        *number = *number + 1;

        log_msg(flog, 0, "oldvalue=%ul, newvalue=%ul ", oldval, *number);

        ++deal_cnt;
      }
      
      mutex_unlock(&mtx, pid);

      if (deal_cnt > 5) {
        usleep(sleep_time);
        sleep_time <<= 2;
      }
    }
  }
  return 0;
}

int main(int argc, char **argv) 
{
  pool_t             *pool = NULL;
  int                 r;
  hash_init_t         hinit;
  atomic_t *addr, *wait, *num;
  sem_t    *sm;
  size_t              record_cnt = 0;

  flog = &file_log;

  pool = create_pool(1024 * 5);
  if (NULL == pool) {
    printf("create pool failed ");
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


  
  shm.size = 256;

  /** mtx.spin = (uintptr_t) -1; */
  mtx.spin = 0;

  create_share_memory(&shm);

  
  
  addr = (atomic_t *)shm.addr;
  wait = (atomic_t *)(shm.addr + 64);
  sm = (sem_t *)(shm.addr + 2 * 64);
  num = (atomic_t *)(shm.addr + 3 * 64);

  *wait = 0;
  *addr = 0;
  *num = 0;
  mtx.sem = sm;

  mutex_create(&mtx, addr, wait); 

  i = 0;


  multi_process_init();

  for ( ; i < 2; i++) {
    mutli_process(i, gobal_number_add, (void *)num);  
  }

  while (gobal_child_status.exist_cnt != gobal_child_status.cur_cnt) {
    sleep(1);
  }

  i = 0;

  for (; i < gobal_child_status.cur_cnt; i++) {
    log_msg(flog, 0, "idx:%d pid:%P status:%c ", i, gobal_child_status.pid[i],
                                                   gobal_child_status.status[i]);
  }

  if (mtx.semaphore) {
    mutex_destroy(&mtx);
  }

  free_share_memory(&shm);

  return 0;
}
