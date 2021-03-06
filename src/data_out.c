/*
Sleepwatch, a programm to gather data from sensors and output it in a ordered
fashion
Copyright (C) 2016, Philip Manke

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include "data_out.h"

void init_out(char *file, unsigned int out_delay, unsigned int *i_from,
              unsigned int *i_to, char read_only) {
  out_file = file;
  from = *i_from;
  to = *i_to;
  if (!read_only) {
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
    for (dcnt = 0; dcnt < 30; dcnt++) {
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
  char cnt = 0;

  // reserve memory for the TABLE structs
  TABLE **data = (TABLE **)malloc(5 * sizeof(TABLE *));
  for (; cnt < 5; cnt++) { // for every sensor

    // get data from the database
    data[cnt] = get_data_time(cnt);
    if (data[cnt] == NULL) { // if no data retrieved
      logn("ERROR could not retrieve table for sensnr: ", (int)cnt);
      free(data);
      return cnt;
    }
  }

  // write data to the file
  write_data(data);

  // cleanup
  for (cnt = 0; cnt < 5; cnt++) {
    destroy_table(data[cnt]);
  }
  free(data);

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

void write_data(TABLE **data) {

  // open 5 files, one for each sensor
  FILE **files = (FILE **)malloc(5 * sizeof(FILE *));
  char *filename = (char *)malloc(strlen(out_file) + 10 * sizeof(char));
  char ccnt = 0;
  for (; ccnt < 5; ccnt++) {
    memset(filename, '\0', strlen(out_file) + 6);
    strncpy(filename, out_file, strlen(out_file));
    strncat(filename, get_tablename(ccnt), 5);
    strncat(filename, ".txt", 4);
    *(files + ccnt) = fopen(filename, "w+");
    if (*(files + ccnt) == NULL) {
      logc("ERROR opening output file failed! : ", filename);
    }
  }
  free(filename);

  // reserve memory for buffer strings
  char *out_buf = (char *)malloc(48 * sizeof(char));
  char *num_buf = (char *)malloc(12 * sizeof(char));
  long cnt = 0;
  char cnt_sens;

  // for every sensor
  for (cnt_sens = 0; cnt_sens < 5; cnt_sens++) {

    for (ccnt = 0; ccnt < data[cnt_sens]->linecount; ccnt++) {

      // reset buffers
      memset(out_buf, '\0', 48);
      memset(num_buf, '\0', 12);

      // grab timestamp
      strncpy(out_buf, data[cnt_sens]->lines[cnt].mtimestamp, 20);

      // convert value to string
      snprintf(num_buf, 11, ";%i", data[cnt_sens]->lines[ccnt].value);

      // add datapoint to this lines buffer
      strncat(out_buf, num_buf, strlen(num_buf));

      // write this line to the file
      fwrite(out_buf, sizeof(char), strlen(out_buf), *(files + cnt_sens));
      if (ccnt < data[cnt_sens]->linecount - 1) {
        fwrite(",\n", sizeof(char), 2, *(files + cnt_sens));
      }
    }
  }

  // cleanup
  for (ccnt = 0; ccnt < 5; ccnt++) {
    fclose(*(files + ccnt));
  }
  free(out_buf);
  free(num_buf);
  free(files);
}

void close_out(void) {
  // stop adn free thread
  stop_other_thread(thread_do);
  free(thread_do);
}
