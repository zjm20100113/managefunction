#include "string_hash.h"
#include "log.h"
#include <unistd.h>

extern int cache_size;

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

  // char enter_string[18] = {0};
  //
  // while (strcmp(enter_string, "quit")) {
  //   scanf("%s", enter_string);
  //
  //   if (strlen(enter_string) < 2) {
  //     continue;
  //   }
  //
  //   puser = (user_t *) hash_find(&hinit, enter_string);
  //   if (NULL == puser) {
  //     log_msg(log, -2, "userid=%s data is losed", enter_string);
  //   } else {
  //     log_msg(log, 0, "userid= %s, custid=%s, acct_id=%s, name=%s, status=%s, areaid=%s, countyid=%s",
  //             puser->user_id, puser->cust_id, puser->acct_id, puser->user_name, puser->user_status,
  //             puser->area_id, puser->county_id);
  //   }
  // }

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


  return 0;
}
