#ifndef SW_WATCH_H
#define SW_WATCH_H

#include "dht/BBB/bbb_dht_read.h"
#include "sqlite_db.h"
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include "dht/common_dht_read.h"

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

typedef struct scheduled_watch_thread{
  pthread_t t_id;
  unsigned int delay;
  char sensnr;
  volatile int running;
  pthread_mutex_t *spinlock;
} WTHR;

WTHR *threads;
WTHR *thread_db;

// ringbuffer to temporarily hold sensor data
typedef struct data_buffer {
  int bufsize;
  int *r_ptr;
  int *w_ptr;
  int *s_ptr;
  pthread_mutex_t r_mutex;
  pthread_mutex_t w_mutex;  
} BUF;

BUF *bufarr;

// initialize buffer, enable adc
void init_watch(void);

// gather datapoint from sensor sensnr
void watch_sensor(char sensnr);

// start a thread to gather a datapoint from sensor sensnr every delay_ms milliseconds
int start_watch_thread(WTHR* thread, char sensnr, unsigned int delay_ms);

// start a thread to flush the buffers to the database every delay_ms milliseconds
int start_db_thread(WTHR* thread, unsigned int delay_ms);

// stop thread for sensor sensnr
int stop_watch_thread(char sensnr);

// stops the db thread
int stop_db_thread(void);

// thread function to watch a sensor
static void * watch_thread(WTHR* inf);

// thread function to periodically empty the buffers in bufarr
static void * db_thread(WTHR* inf);

// grab a datapoint from the buffer
int grab_value(char sensnr);

// add datapoint to the buffer
void add_to_buf(char sensnr, int *val);

// read from the adc file 
int read_adc(char *adcfile);

// read the AM2302 sensor
int read_dht(float *hum, float *temp);

// advance the pointer - thread safe
void advance_r_pointer(char* sensnr);
void advance_w_pointer(char* sensnr);

// check if buffer for sensor sensnr is empty
int buffer_empty(char* sensnr);

// read all datapoints in the buffer and store the average in the database
void flush_buffer_to_db(void);

// free the buffer
void close_watch(void);

#endif // SW_WATCH_H
