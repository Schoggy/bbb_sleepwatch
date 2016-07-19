#ifndef SW_DATA_OUT_H
#define SW_DATA_OUT_H

#include "dht/common_dht_read.h"
#include "log.h"
#include "sqlite_db.h"
#include "thread_funcs.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

OTHR *thread_do;
char *out_file;
unsigned int from, to;

// start the output thread
void init_out(char *file, unsigned int out_delay, unsigned int *from,
              unsigned int *to, char read_only);
// set different timeframe to output
void set_timeframe(unsigned int *from, unsigned int *to);

// thread calling refresh_out_time every delay_ms milliseconds
static void *data_out_thread(void *arg);

// read from the database and output the data in a different format to a file
int refresh_out_time(void);

// reads from the database
TABLE *get_data_time(char sensnr);

// writes data given in "data" to the file
void write_data(FILE *file, TABLE **data);

// stop the output thread
void close_out(void);

#endif // SW_DATA_OUT_H
