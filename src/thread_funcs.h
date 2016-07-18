#ifndef SW_THREAD_FUNCS_H
#define SW_THREAD_FUNCS_H

#include <pthread.h>
#include <stdlib.h>

typedef struct scheduled_watch_thread {
  pthread_t t_id;
  unsigned int delay;
  char sensnr;
  volatile int running;
  pthread_mutex_t *spinlock;
} WTHR;

typedef struct scheduled_other_thread {
  pthread_t t_id;
  unsigned int delay;
  volatile int running;
  pthread_mutex_t *spinlock;
} OTHR;

// start a thread to gather a datapoint from sensor sensnr every delay_ms
// milliseconds
int start_watch_thread(WTHR *thread, char sensnr, unsigned int delay_ms,
                       void *(*func)(void *));

// start a thread that executes func every delay_ms milliseconds
int start_other_thread(OTHR *thread, unsigned int delay_ms,
                       void *(*func)(void *));

// stop thread for sensor sensnr
int stop_watch_thread(WTHR *thread);

// stops other thread "thread"
int stop_other_thread(OTHR *thread);

#endif // SW_THREAD_FUNCS_H
