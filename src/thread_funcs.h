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
