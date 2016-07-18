#ifndef SW_DATA_OUT_H
#define SW_DATA_OUT_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "dht/common_dht_read.h"
#include "sqlite_db.h"
#include "thread_funcs.h"
#include "log.h"

OTHR *thread_do;
char *out_file;
unsigned int from, to;

void init_out(char* file, unsigned int out_delay, unsigned int *from, unsigned int *to);

void set_timeframe(unsigned int *from, unsigned int *to);

static void *data_out_thread(void *arg);

int refresh_out_time(void);

TABLE* get_data_time(char sensnr);

void write_data(FILE *file, TABLE** data);

void close_out(void);

#endif // SW_DATA_OUT_H
