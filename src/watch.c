#include "watch.h"

static void *watch_thread(void *arg) {
  WTHR *inf = arg;
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  inf->spinlock = &mutex;
  // main loop for the thread
  while (1) {

    // lock mutex protecting the variable "running"
    pthread_mutex_lock(&mutex);
    if (!inf->running) { // stop the thread
      pthread_mutex_unlock(&mutex);
      pthread_mutex_destroy(&mutex);
      pthread_exit(NULL);
    }
    pthread_mutex_unlock(&mutex);

    // get datapoint from sensor,
    watch_sensor(inf->sensnr);
    sleep_milliseconds(inf->delay);
  }
}

static void *db_thread(void *arg) {
  OTHR *inf = arg;
  int dcnt;
  // main loop for the thread
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  inf->spinlock = &mutex;
  while (1) {
    for (dcnt = 0; dcnt < 10; dcnt++) {
      // this thread should have a high idle time but can only be stopped when
      // active.
      // therefore wake the thread every seconf during idle time to check if it
      // should stop.
      sleep_milliseconds(inf->delay / 10);
      // lock mutex protecting the variable "running"
      pthread_mutex_lock(&mutex);
      if (!inf->running) { // stop the thread
        pthread_mutex_unlock(&mutex);
        pthread_mutex_destroy(&mutex);
        pthread_exit(NULL);
      }
      pthread_mutex_unlock(&mutex);
    }

    // write averages to db
    flush_buffer_to_db();
  }
}

void init_watch(char en_monitoring) {

  FILE *test = fopen(ADC_FILE0, "r"); // check if adc file can be opened
  if (test == NULL) {
    // if not init adc
    system("echo BB-ADC > /sys/devices/platform/bone_capemgr/slots");
  } else {
    // if yes close the file again
    fclose(test);
  }

  // init buffers
  char bufsizes[5] = {56, 241, 5, 5, 5};
  bufarr = (BUF *)malloc(5 * sizeof(BUF));
  char cntr = 0;
  int response;
  for (; cntr < 5; cntr++) {

    // get memory for the buffer
    (bufarr + cntr)->s_ptr =
        (unsigned int *)malloc(bufsizes[cntr] * sizeof(unsigned int));
    (bufarr + cntr)->r_ptr = (bufarr + cntr)->s_ptr;
    (bufarr + cntr)->w_ptr = (bufarr + cntr)->s_ptr;
    (bufarr + cntr)->bufsize = bufsizes[cntr];

    // initialize the mutexes
    response = pthread_mutex_init(&((bufarr + cntr)->r_mutex), NULL);
    if (response) {
      logn("ERROR could not initialize mutex for bufarr! Error number:",
           response);
      exit(1);
    }
    response = pthread_mutex_init(&((bufarr + cntr)->w_mutex), NULL);
    if (response) {
      logn("ERROR could not initialize mutex for bufarr! Error number:",
           response);
      exit(1);
    }
  }
  if (en_monitoring) {
    run_threads();
  }
}

void run_threads(void) {
  // start watch threads
  char cntr = 0, sensnr = 0;
  int res;
  threads = (WTHR *)malloc(4 * sizeof(WTHR));
  for (; cntr < 4; cntr++, sensnr++) {
    if (cntr == 3) {
      sensnr = 4;
    } // as sensors 2 and 3 are actually one sensor, only one thread is needed.
      // Move to sensor 4 for thread 3
    res = start_watch_thread(
        (threads + cntr), cntr,
        (unsigned int)floor(DB_LOG_INTERVAL /
                            floor((bufarr + cntr)->bufsize * 0.79)),
        &watch_thread);
    if (res) {
      logn("ERROR failed to start watch thread for sensnr: ", (int)cntr);
    } else {
      logn("INFO successfully started watch thread for sensnr: ", (int)cntr);
    }
  }

  // start database flusher thread
  thread_db = (OTHR *)malloc(sizeof(OTHR));
  res = start_other_thread(thread_db, DB_LOG_INTERVAL, &db_thread);
  if (res) {
    logn("ERROR failed to start watch thread for sensnr: ", (int)cntr);
  }
}

void watch_sensor(char sensnr) {
  unsigned int val;
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
      val = (unsigned int)(temp * 100);
      add_to_buf(TSENS, &val);
      val = (unsigned int)(hum * 100);
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

unsigned int read_adc(char *adcfile) {
  FILE *file = fopen(adcfile, "r"); // open adc file

  if (file == NULL) { // file not opened
    logc("ERROR opening ADC file failed! : ", adcfile);
    return -1;
  }

  char *cval = (char *)calloc(5, sizeof(char));
  unsigned long out;
  // read four characters from the file;
  fread(cval, 1, 4, file);
  out = strtoul(cval, NULL, 0); // convert them to unsigned long

  // cleanup
  free(cval);
  fclose(file);

  return (unsigned int)out; // return as unsigned int. Values from the adc
                            // should only range from 0 to 4096
}

int read_dht(float *hum, float *temp) {
  // read from the AM2302
  int dht_res = bbb_dht_read(AM2302, DHT_GPIO_BASE, DHT_GPIO_NR, hum, temp);
  if (dht_res != DHT_SUCCESS) { // print appropriate error message
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

void add_to_buf(char sensnr, unsigned int *val) {
  *((bufarr + sensnr)->w_ptr) = *val;
  advance_w_pointer(&sensnr);
}

unsigned int grab_value(char sensnr) {
  unsigned int out = *((bufarr + sensnr)->r_ptr);
  advance_r_pointer(&sensnr);
  return out;
}

void advance_r_pointer(char *sensnr) {
  pthread_mutex_lock(&((bufarr + *sensnr)->r_mutex));

  // advance the pointer by 1 mod buffersize
  (bufarr + *sensnr)->r_ptr =
      (bufarr + *sensnr)->s_ptr +
      (((bufarr + *sensnr)->r_ptr - (bufarr + *sensnr)->s_ptr + 1) %
       (bufarr + *sensnr)->bufsize);
  pthread_mutex_unlock(&((bufarr + *sensnr)->r_mutex));
}

void advance_w_pointer(char *sensnr) {
  pthread_mutex_lock(&((bufarr + *sensnr)->w_mutex));

  // advance the pointer by 1 mod buffersize
  (bufarr + *sensnr)->w_ptr =
      (bufarr + *sensnr)->s_ptr +
      (((bufarr + *sensnr)->w_ptr - (bufarr + *sensnr)->s_ptr + 1) %
       (bufarr + *sensnr)->bufsize);
  pthread_mutex_unlock(&((bufarr + *sensnr)->w_mutex));
}

int buffer_empty(char *sensnr) {
  int out = 0;
  unsigned int *r;
  unsigned int *w;

  // grab write pointer
  pthread_mutex_lock(&((bufarr + *sensnr)->w_mutex));
  w = (bufarr + *sensnr)->w_ptr;
  pthread_mutex_unlock(&((bufarr + *sensnr)->w_mutex));

  // grab read pointer
  pthread_mutex_lock(&((bufarr + *sensnr)->r_mutex));
  r = (bufarr + *sensnr)->r_ptr;
  pthread_mutex_unlock(&((bufarr + *sensnr)->r_mutex));

  // compare
  if (r == w) {
    out = 1;
  }
  return out;
}

void close_watch(void) {
  char cntr = 0;
  for (; cntr < 5; cntr++) {
    if (cntr < 4) {
      // stop watch threads
      stop_watch_thread(threads + cntr);
    }

    // free actual buffers
    free((bufarr + cntr)->s_ptr);

    // cleanup the mutexes
    pthread_mutex_destroy(&((bufarr + cntr)->r_mutex));
    pthread_mutex_destroy(&((bufarr + cntr)->w_mutex));
  }

  stop_other_thread(thread_db);

  // free buffer and thread info structs
  free(bufarr);
  free(threads);
  free(thread_db);
}

void flush_buffer_to_db(void) {
  char cntr_o, cntr_i;
  unsigned long akk;
  int avg;
  for (cntr_o = 0; cntr_o < 5; cntr_o++) {
    akk = 0;

    // add up all datapoints in the buffer
    for (cntr_i = 0; !buffer_empty(&cntr_o); cntr_i++) {
      akk += (unsigned long)grab_value(cntr_o);
    }
    if (cntr_i) {
      // store the average in the database
      avg = (int)floor(akk / cntr_i);
      if (cntr_o < 2) {
        avg = 4096 - avg;
      }
      if (avg < 1) {
        avg = 0;
      }
      insert_db(cntr_o, &avg);
    }
  }
}
