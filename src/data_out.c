#include "data_out.h"

void init_out(char* file, unsigned int out_delay, unsigned int *i_from, unsigned int *i_to){
  out_file = file;
  from = *i_from;
  to = *i_to;
  thread_do = (OTHR*) malloc(sizeof(OTHR));
  start_other_thread(thread_do, out_delay, &data_out_thread);
}

static void * data_out_thread(void *arg){
  OTHR *inf = arg;
  int dcnt;
  // main loop for the thread
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  inf->spinlock = &mutex;
  while(1){
    for(dcnt = 0; dcnt < 10; dcnt++){ 
      // this thread should have a high idle time but can only be stopped when active.
      // therefore wake the thread 10 times during idle time to check if it should stop.
      sleep_milliseconds(inf->delay / 10);
      // lock mutex protecting the variable "running"
      pthread_mutex_lock(&mutex);
      if(!inf->running){ // stop the thread
        pthread_mutex_unlock(&mutex);
        pthread_mutex_destroy(&mutex);
        // OK to call logm here, status of all threads is known, noone will call log functions right now
        logm("INFO data output thread successfully stopped.");
        
        pthread_exit(NULL);
      }
      pthread_mutex_unlock(&mutex);
    }
    
    refresh_out_time();      
  }
}

void set_timeframe(unsigned int* i_from, unsigned int* i_to){
  pthread_mutex_lock(thread_do->spinlock);
  from = *i_from;
  to = *i_to;
  pthread_mutex_unlock(thread_do->spinlock);
}

int refresh_out_time(void){
  FILE *file = fopen(out_file, "w+");
  char cnt = 0;
  TABLE **data = (TABLE**) malloc(5 * sizeof(TABLE*));
  for(; cnt < 5; cnt++){
    data[cnt] = get_data_time(cnt);
    if(data[cnt] == NULL){
      logn("ERROR could not retrieve table for sensnr: ", (int) cnt);
      return cnt;
  }
  write_data(file, data);
  for(cnt = 0; cnt < 5; cnt++){
    destroy_table(data[cnt]);
  }
  free(data);
  fclose(file);
}

TABLE* get_data_time(char sensnr){
  TABLE *out;
  char *str_from = (char*) calloc(38, sizeof(char));
  char *str_to = (char*) calloc(38, sizeof(char));
  
  if(to == 0){
    strncpy(str_to, "datetime('now')", 15);
  } else {
    snprintf(str_to, 31, "datetime('now','-%u minute')", to);
  }
  snprintf(str_from, 31, "datetime('now','-%u minute')", from);
  
  out = query_db(sensnr, str_from, str_to);
  
  free(str_from);
  free(str_to);
  
  return out;
}

void write_data(FILE *file, TABLE** data){
  char *out_buf = (char*) malloc(100 * sizeof(char));
  char *num_buf = (char*) malloc(12 * sizeof(char));
  long cnt = 0;
  char cnt_sens;
  for(;cnt < data[0]->linecount; cnt++){
    memset(out_buf, '\0', 100);
    strncpy(out_buf, data[0]->lines[0].mtimestamp, 20);
    for(cnt_sens = 0; cnt_sens < 5; cnt_sens++){
      memset(num_buf, '\0', 12);
      snprintf(num_buf, 11, ",%i", data[cnt_sens]->lines[cnt].value);
      strncat(out_buf, num_buf, strlen(num_buf));
    }
    fwrite(out_buf, sizeof(char), strlen(out_buf), file);
  }
  free(out_buf);
  free(num_buf); 
}

void close_out(void){
  stop_other_thread(thread_do);
  free(thread_do);
}

