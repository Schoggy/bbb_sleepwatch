#include "data_out.h"

void init_out(char *file, unsigned int out_delay, unsigned int *i_from,
              unsigned int *i_to, char read_only) {
  out_file = file;
  from = *i_from;
  to = *i_to;
  if(!read_only){
    thread_do = (OTHR *)malloc(sizeof(OTHR));
    start_other_thread(thread_do, out_delay, &data_out_thread);
  }
}

static void *data_out_thread(void *arg) {
  OTHR *inf = arg;
  int dcnt;
  // main loop for the thread
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  inf->spinlock = &mutex;
  while (1) {
    for (dcnt = 0; dcnt < 10; dcnt++) {
      // this thread should have a high idle time but can only be stopped when
      // active.
      // therefore wake the thread 30 times during idle time to check if it
      // should stop.
      sleep_milliseconds(inf->delay / 30);
      // lock mutex protecting the variable "running"
      pthread_mutex_lock(&mutex);
      if (!inf->running) { // stop the thread
        pthread_mutex_unlock(&mutex);
        pthread_mutex_destroy(&mutex);
        pthread_exit(NULL);
      }
      pthread_mutex_unlock(&mutex);
    }

    refresh_out_time();
  }
}

void set_timeframe(unsigned int *i_from, unsigned int *i_to) {
  pthread_mutex_lock(thread_do->spinlock);
  from = *i_from;
  to = *i_to;
  pthread_mutex_unlock(thread_do->spinlock);
}

int refresh_out_time(void) {

  // overwrite and open the output file
  FILE *file = fopen(out_file, "w+");
  char cnt = 0;

  // reserve memory for the TABLE structs
  TABLE **data = (TABLE **)malloc(5 * sizeof(TABLE *));
  for (; cnt < 5; cnt++) { // for every sensor

    // get data from the database
    data[cnt] = get_data_time(cnt);
    if (data[cnt] == NULL) { // if no data retrieved
      logn("ERROR could not retrieve table for sensnr: ", (int)cnt);
      fclose(file);
      free(data);
      return cnt;
    }
  }

  // write data to the file
  write_data(file, data);

  // free the TABLE structs
  for (cnt = 0; cnt < 5; cnt++) {
    destroy_table(data[cnt]);
  }
  // more cleanup
  free(data);
  fclose(file);
  return 0;
}

TABLE *get_data_time(char sensnr) {
  TABLE *out;
  char *str_from = (char *)calloc(38, sizeof(char));
  char *str_to = (char *)calloc(38, sizeof(char));

  // build parts of the SQL statement
  if (to == 0) {
    strncpy(str_to, "datetime('now')", 15);
  } else {
    snprintf(str_to, 31, "datetime('now','-%u minute')", to);
  }
  snprintf(str_from, 31, "datetime('now','-%u minute')", from);

  // query the database
  out = query_db(sensnr, str_from, str_to);

  // cleanup
  free(str_from);
  free(str_to);

  return out;
}

void write_data(FILE *file, TABLE **data) {
  char *out_buf = (char *)malloc(100 * sizeof(char));
  char *num_buf = (char *)malloc(12 * sizeof(char));
  long cnt = 0;
  char cnt_sens;

  // write header
  fwrite("Timestamp, Light level, Noise level, Temperature, Humidity, Air Quality\n", 72, sizeof(char), file);
  
  // for every line retrieved
  for (; cnt < data[0]->linecount; cnt++) {

    // reset buffer
    memset(out_buf, '\0', 100);

    // grab timestamp
    strncpy(out_buf, data[0]->lines[cnt].mtimestamp, 20);

    // for every sensor
    for (cnt_sens = 0; cnt_sens < 5; cnt_sens++) {

      // reset buffer
      memset(num_buf, '\0', 12);
      if (data[cnt_sens] != NULL) { // if data available
        if (data[cnt_sens]->lines != NULL) {

          // convert the datapoint to a string
          if(data[cnt_sens]->linecount >= cnt){
            snprintf(num_buf, 11, ",%i", data[cnt_sens]->lines[cnt].value);
          } else {
            strncpy(num_buf, ",-", 2);
          }
        } else {

          // add a '-' if no value present
          strncpy(num_buf, ",-", 2);
        }
      } else {
        strncpy(num_buf, ",-", 2);
      }
      // add datapoint to this lines buffer
      strncat(out_buf, num_buf, strlen(num_buf));
    }

    // write line this line to the file
    fwrite(out_buf, sizeof(char), strlen(out_buf), file);

    // start a new line
    fwrite("\n", sizeof(char), 1, file);
  }
  free(out_buf);
  free(num_buf);
}

void close_out(void) {
  // stop adn free thread
  stop_other_thread(thread_do);
  free(thread_do);
}
