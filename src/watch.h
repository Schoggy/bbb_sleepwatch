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
#ifndef SW_WATCH_H
#define SW_WATCH_H

#include "dht/BBB/bbb_dht_read.h"
#include "dht/common_dht_read.h"
#include "log.h"
#include "sqlite_db.h"
#include "thread_funcs.h"
#include <math.h>
#include <pthread.h>
#include <stdlib.h>

// light sensor
#define ADC_FILE0 "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
// microphone
#define ADC_FILE1 "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"
// gas sensor
#define ADC_FILE2 "/sys/bus/iio/devices/iio:device0/in_voltage2_raw"

// gpio numbers for the one wire communication with the AM2302 sensor
#define DHT_GPIO_BASE 0
#define DHT_GPIO_NR 2

// intervall between database entries in milliseconds
#define DB_LOG_INTERVAL 10000

WTHR *threads;
OTHR *thread_db;

// ringbuffer to temporarily hold sensor data
typedef struct data_buffer {
  int bufsize;
  unsigned int *r_ptr;
  unsigned int *w_ptr;
  unsigned int *s_ptr;
  pthread_mutex_t r_mutex;
  pthread_mutex_t w_mutex;
} BUF;

BUF *bufarr;

// initialize buffer, enable adc
void init_watch(char en_monitoring);

// start the threads gathering and saving data
void run_threads(void);

// gather datapoint from sensor sensnr
void watch_sensor(char sensnr);

// thread function to watch a sensor
static void *watch_thread(void *arg);

// thread function to periodically empty the buffers in bufarr
static void *db_thread(void *arg);

// grab a datapoint from the buffer
unsigned int grab_value(char sensnr);

// add datapoint to the buffer
void add_to_buf(char sensnr, unsigned int *val);

// read from the adc file
unsigned int read_adc(char *adcfile);

// read the AM2302 sensor
int read_dht(float *hum, float *temp);

// advance the pointer - thread safe
void advance_r_pointer(char *sensnr);
void advance_w_pointer(char *sensnr);

// check if buffer for sensor sensnr is empty
int buffer_empty(char *sensnr);

// read all datapoints in the buffer and store the average in the database
void flush_buffer_to_db(void);

// free the buffer
void close_watch(void);

#endif // SW_WATCH_H
