#include "watch.h"

int start_watch_thread(WTHR *thread, char sensnr, unsigned int delay_ms){
  int response;
  
  if(sensnr == 3){sensnr++;}
  
  thread->running = 1;
  thread->sensnr = sensnr;
  thread->delay = delay_ms;
  
  // build pthread attribute valiable
  pthread_attr_t thread_attr;
  pthread_attr_init(&thread_attr);
  
  if(sensnr == 2){
    pthread_attr_setschedpolicy(&thread_attr, SCHED_FIFO);
    struct sched_param par;
    par.sched_priority = 1;
    pthread_attr_setschedparam(&thread_attr, &par);
  }
  
  // create the thread
  printf("Starting thread sensnr: %i\n", sensnr); // debug
  response = pthread_create(&(thread->t_id), &thread_attr, &watch_thread, (void*) thread);
  if(response){
    logn("ERROR failed to create watch thread: ", response);
  } else {
    logn("INFO successfully started watch thread for sensnr: ", sensnr);
  }
  
  // cleanup
  pthread_attr_destroy(&thread_attr);
  return response;
}

int start_db_thread(WTHR *thread, unsigned int delay_ms){
  int response;
  
  // set thread data
  thread->running = 1;
  thread->sensnr = 10;
  thread->delay = delay_ms;
  
  // build pthread attribute valiable
  pthread_attr_t thread_attr;
  pthread_attr_init(&thread_attr);
  
  // create the thread
  response = pthread_create(&(thread->t_id), &thread_attr, &db_thread, (void*) thread);
  if(response){
    logn("ERROR failed to create db thread: ", response);
  } else {
    logm("INFO successfully started DB thread!");
  }
  
  // cleanup
  pthread_attr_destroy(&thread_attr);
  return response;
}

int stop_watch_thread(char sensnr){
  int ret;
  // tell the thread to stop next time it runs through the loop
  pthread_mutex_lock((threads + sensnr)->spinlock);
  (threads + sensnr)->running = 0;
  pthread_mutex_unlock((threads + sensnr)->spinlock);
  void *t_ret;
  
  // join with the thread
  pthread_join((threads + sensnr)->t_id, &t_ret);
  ret = *((int*) t_ret);
  free(t_ret);
  
  return ret;
}

int stop_db_thread(void){
  int ret;
  
  // tell the thread to stop next time it runs through the loop
  pthread_mutex_lock(thread_db->spinlock);
  thread_db->running = 0;
  pthread_mutex_unlock(thread_db->spinlock);
  void *t_ret;
  
  // join with the thread
  pthread_join(thread_db->t_id, &t_ret);
  ret = *((int*) t_ret);
  free(t_ret);
  return ret;
}

static void * watch_thread(void *arg){
  WTHR *inf = arg;
  printf("Thread started! Sensnr: %i\n", inf->sensnr); // debug 
  int *ret = (int*) malloc(sizeof(int));
  *ret = (int) inf->sensnr;
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  inf->spinlock = &mutex;
  // main loop for the thread
  while(1){
    
    // lock mutex protecting the variable "running"
    pthread_mutex_lock(&mutex);
    if(!inf->running){ // stop the thread
      pthread_mutex_unlock(&mutex);
      
      // OK to call logm here, status of all threads is known, noone will call log functions now
      logn("INFO thread successfully stopped. sensnr: ", inf->sensnr);
      pthread_mutex_destroy(&mutex);
      return ret;
    }
    pthread_mutex_unlock(&mutex);
    
    // get datapoint from sensor, 
    watch_sensor(inf->sensnr);
    sleep_milliseconds(inf->delay);
  }
}

static void * db_thread(void *arg){
  WTHR *inf = arg;
  int *ret = (int*) malloc(sizeof(int));
  // main loop for the thread
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  inf->spinlock = &mutex;
  while(1){
    
    // lock mutex protecting the variable "running"
    pthread_mutex_lock(&mutex);
    if(!inf->running){ // stop the thread
      pthread_mutex_unlock(&mutex);
      
      // OK to call logm here, status of all threads is known, noone will call log functions now
      logm("INFO DB thread successfully stopped.");
      *ret = 10;
      pthread_mutex_destroy(&mutex);
      return ret;
    }
    pthread_mutex_unlock(&mutex);
    
    // get datapoint from sensor, 
    flush_buffer_to_db();
    sleep_milliseconds(inf->delay);
  }
}

void init_watch(void) {
  
  // init adc
  FILE* test = fopen(ADC_FILE0, "r");
  if(test == NULL){
    system("echo BB-ADC > /sys/devices/platform/bone_capemgr/slots");
  } else {
    fclose(test);
  }
  
  // init buffers
  char bufsizes[5] = {56, 241, 5, 5, 5};
  bufarr = (BUF *)malloc(5 * sizeof(BUF));
  char cntr = 0;
  int response;
  for (; cntr < 5; cntr++) {
    
    // get memory for the buffer
    (bufarr + cntr)->s_ptr = (int *)malloc(bufsizes[cntr] * sizeof(int));
    (bufarr + cntr)->r_ptr = (bufarr + cntr)->s_ptr;
    (bufarr + cntr)->w_ptr = (bufarr + cntr)->s_ptr;
    
    (bufarr + cntr)->bufsize = bufsizes[cntr];
    
    response = pthread_mutex_init(&((bufarr + cntr)->r_mutex), NULL);
    if(response){
      logn("ERROR could not initialize mutex for bufarr! Error number:", response);
      exit(1);
    }
    
    response = pthread_mutex_init(&((bufarr + cntr)->w_mutex), NULL);
    if(response){
      logn("ERROR could not initialize mutex for bufarr! Error number:", response);
    }
  }
  
  // start watch threads 
  int res;
  unsigned int delay_ms;
  threads = (WTHR*) malloc(4 * sizeof(WTHR));
  for(cntr = 0; cntr < 4; cntr++){
    if(cntr == 3){ cntr++;} // as sensors 2 and 3 are actually one sensor, only one thread is needed. 
    res = start_watch_thread((threads + cntr), cntr,
      (unsigned int) floor(DB_LOG_INTERVAL / floor((bufarr + cntr)->bufsize * 0.79)));
    if(res){
      logn("ERROR failed to start watch thread for sensnr: ", (int) cntr);
    }
  }
  
  // start database flusher thread
  thread_db = (WTHR*) malloc(sizeof(WTHR));
  res = start_db_thread((threads + cntr), DB_LOG_INTERVAL);
  if(res){
    logn("ERROR failed to start watch thread for sensnr: ", (int) cntr);
  }
}

void watch_sensor(char sensnr) {
  int val;
  switch (sensnr) {
  case 0: {
    // light level
    val = read_adc(ADC_FILE0);
    if (val < 1) {
      logm("WARNING light sensor could not be read!");
    } else {
      add_to_buf(LSENS, &val);
    }
  } break;
  case 1: {
    // noise level
    val = read_adc(ADC_FILE1);
    if (val < 1) {
      logm("WARNING microphone could not be read!");
    } else {
      add_to_buf(NSENS, &val);
    }
  } break;
  case 2:
  case 3: {
    // temperature & humidity
    float hum, temp;
    int dht_res = read_dht(&hum, &temp);
    if (dht_res == DHT_SUCCESS) {
      val = temp * 100;
      add_to_buf(TSENS, &val);
      val = hum * 100;
      add_to_buf(HSENS, &val);
    }
  } break;
  case 4: {
    // air quality
    val = read_adc(ADC_FILE2);
    if (val < 1) {
      logm("WARNING gas sensor could not be read!");
    } else {
      add_to_buf(GSENS, &val);
    }
  } break;
  default: {
    logn("ERROR watch_sensor called with wrong sensnr!", sensnr);
  } break;
  }
}

int read_adc(char *adcfile) {
  FILE *file = fopen(adcfile, "r");

  if (file == NULL) {
    logc("ERROR opening ADC file failed! : ", adcfile);
    return -1;
  }
  
  char *cval = (char *)calloc(5, sizeof(char));
  int out;

  fread(cval, 1, 4, file);
  out = atoi(cval);
  
  free(cval);
  fclose(file);
  
  return out;
}

int read_dht(float *hum, float *temp) {
  int dht_res = bbb_dht_read(AM2302, DHT_GPIO_BASE, DHT_GPIO_NR, hum, temp);
  if (dht_res != DHT_SUCCESS) {
    switch (dht_res) {
    case DHT_ERROR_TIMEOUT: {
      logn("WARNING DHT did not return in give timeframe: DHT_ERROR_TIMEOUT!",
           dht_res);
    } break;
    case DHT_ERROR_CHECKSUM: {
      logn("INFO DHT checksum failed to validate: DHT_ERROR_CHECKSUM!",
           dht_res);
    } break;
    case DHT_ERROR_ARGUMENT: {
      logn("ERROR DHT read function failed! Wrong input to read function: "
           "DHT_ERROR_ARGUMENT!",
           dht_res);
    } break;
    case DHT_ERROR_GPIO: {
      logn("ERROR DHT is not connected correctly or wrong pin numbers are "
           "used: DHT_ERROR_GPIO!",
           dht_res);
    } break;
    default: {
      logm("ERROR unknown return value value from DHT!");
      return -5;
    } break;
    }
    return dht_res;
  }
  return dht_res;
}

void add_to_buf(char sensnr, int *val) {
  *((bufarr + sensnr)->w_ptr) = *val;
  advance_w_pointer(&sensnr);
}

int grab_value(char sensnr) {
  int out = *((bufarr + sensnr)->r_ptr);
  advance_r_pointer(&sensnr);
  return out;
}

void advance_r_pointer(char* sensnr){
  pthread_mutex_lock(&((bufarr + *sensnr)->r_mutex));
  
  // advance the pointer by 1 mod buffersize
  (bufarr + *sensnr)->r_ptr =
      (bufarr + *sensnr)->s_ptr +
      (((bufarr + *sensnr)->r_ptr - (bufarr + *sensnr)->s_ptr + 1) %
       (bufarr + *sensnr)->bufsize);
  pthread_mutex_unlock(&((bufarr + *sensnr)->r_mutex));
}

void advance_w_pointer(char* sensnr){
  pthread_mutex_lock(&((bufarr + *sensnr)->w_mutex));
  
  // advance the pointer by 1 mod buffersize
  (bufarr + *sensnr)->w_ptr =
      (bufarr + *sensnr)->s_ptr +
      (((bufarr + *sensnr)->w_ptr - (bufarr + *sensnr)->s_ptr + 1) %
       (bufarr + *sensnr)->bufsize);
  pthread_mutex_unlock(&((bufarr + *sensnr)->w_mutex));
}

int buffer_empty(char* sensnr){
  int out = 0;
  int *r;
  int *w;
  
  // grab write pointer
  pthread_mutex_lock(&((bufarr + *sensnr)->w_mutex));
  w = (bufarr + *sensnr)->w_ptr;
  pthread_mutex_unlock(&((bufarr + *sensnr)->w_mutex));
  
  // grab read pointer
  pthread_mutex_lock(&((bufarr + *sensnr)->r_mutex));
  r = (bufarr + *sensnr)->r_ptr;
  pthread_mutex_unlock(&((bufarr + *sensnr)->r_mutex));
  
  // compare
  if(r == w){out = 1;}
  return out;
}

void close_watch(void) {
  char cntr = 0;
  for (; cntr < 5; cntr++) {
    
    // free actual buffers
    free((bufarr + cntr)->s_ptr);
    
    // cleanup the mutexes
    pthread_mutex_destroy(&((bufarr + cntr)->r_mutex));
    pthread_mutex_destroy(&((bufarr + cntr)->w_mutex));
    if(cntr < 4){
      
      // stop watch threads
      stop_watch_thread(cntr);
    }
  }
  
  stop_db_thread();
  
  // free buffer and thread info structs
  free(bufarr);
  free(threads);
  free(thread_db);
}

void flush_buffer_to_db(void) {
  char cntr_o, cntr_i;
  int akk;
  for (cntr_o = 0; cntr_o < 5; cntr_o++) {
    akk = 0;
    
    // add up all datapoints in the buffer
    for (cntr_i = 0; !buffer_empty(&cntr_o); cntr_i++) {
      akk += grab_value(cntr_o);
    }
    if(cntr_i){
    
      // store the average in the database
      akk /= cntr_i;
      insert_db(cntr_o, &akk);
    }
  }
}
