#ifndef SW_WATCH_H
#define SW_WATCH_H

#include <stdlib.h>
#include "dht/BBB/bbb_dht_read.h"
#include "sqlite_db.h"

// light sensor
#define ADC_FILE0 "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
// microphone
#define ADC_FILE1 "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"
// gas sensor
#define ADC_FILE2 "/sys/bus/iio/devices/iio:device0/in_voltage2_raw"

#define DHT_GPIO_BASE 0
#define DHT_GPIO_NR 2

typedef struct data_buffer{
  int bufsize;
  int *r_ptr;
  int *w_ptr;
  int *s_ptr;
} BUF;

BUF *bufarr;

void init_watch(void);
void watch_sensor(char sensnr);
int grab_value(char sensnr);
void add_to_buf(char sensnr, int* val);
int read_adc(char* adcfile);
int read_dht(float* hum, float* temp);
int flush_buffer_to_db(void);
void close_buffers(void);

#endif // SW_WATCH_H
