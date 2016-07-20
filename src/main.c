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
#include "dht/common_dht_read.h"
#include "log.h"
#include "sqlite_db.h"
#include "thread_funcs.h"
#include "watch.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stropts.h>

// default number timeframe of data to output
#define DEFAULT_OUTPUT_MINS 60

// default number of seconds between output file refreshes
#define DEFAULT_OUT_DELAY 30

// default file names
#define DEFAULT_OUTPUT "output_"
#define DEFAULT_LOG "sleepwatch_log"
#define DEFAULT_DB "sleepwatch_db"

void print_usage(void);
int test_path(char *fpath);
void cleanup(void);
static void *check_q(void *args);

char *dbfile;
char *logfile;
char *output;

int main(int argc, char *argv[]) {

  char read_only = 0;
  char new_dbfile = 0;
  char autostarted = 0;

  unsigned long timeout = 0;
  unsigned long output_mins = 0;
  unsigned long past = 0;
  unsigned long out_delay = 0;

  char cntr;
  printf("Sleepwatch version 0.1, Copyright (C) 2016 Philip Manke\n"
         "Gnomovision comes with ABSOLUTELY NO WARRANTY; for details\n"
         "type 'sleepwatch -g'.  This is free software, and you are welcome\n"
         "to redistribute it under certain conditions;\n"
         "read the file 'COPYING' for details.\n\n");
  for (cntr = 1; cntr < argc; cntr++) {
    if (argv[cntr][0] == '-') {
      switch (argv[cntr][1]) {
      case 'a': {
        autostarted = 1;
      } break;
      case 'd': {
        if (argv[cntr][2] == '=') {
          dbfile = (char *)calloc(strlen(&argv[cntr][3]) + 1, sizeof(char));
          strncpy(dbfile, &argv[cntr][3], strlen(&argv[cntr][3]));
        } else {
          printf("Unknown argument: %s !\n", argv[cntr]);
          return 1;
        }
      } break;
      case 'f': {
        if (argv[cntr][2] == '=') {
          output = (char *)calloc(strlen(&argv[cntr][3]) + 1, sizeof(char));
          strncpy(output, &argv[cntr][3], strlen(&argv[cntr][3]));
        } else {
          printf("Unknown argument: %s !\n", argv[cntr]);
          return 1;
        }
      } break;
      case 'g': {
        printf("NO WARRANTY\n\n"
               "11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS "
               "NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY "
               "APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE "
               "COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM "
               "\"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR "
               "IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES "
               "OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE "
               "ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM "
               "IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME "
               "THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.\n"
               "12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO "
               "IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO "
               "MAY MODIFY AND/OR REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, "
               "BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY GENERAL, SPECIAL, "
               "INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR "
               "INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO "
               "LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES "
               "SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM "
               "TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR "
               "OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH "
               "DAMAGES.\n");
      } break;
      case 'n': {
        new_dbfile = 1;
        printf("Starting new database.\n");
      } break;
      case 'l': {
        if (argv[cntr][2] == '=') {
          logfile = (char *)calloc(strlen(&argv[cntr][3]) + 1, sizeof(char));
          strncpy(logfile, &argv[cntr][3], strlen(&argv[cntr][3]));
        } else {
          printf("Unknown argument: %s !\n", argv[cntr]);
          return 1;
        }
      } break;
      case 'o': {
        if (argv[cntr][2] == '=') {
          output_mins = strtoul(&argv[cntr][3], NULL, 0);
        } else {
          printf("Unknown argument: %s !\n", argv[cntr]);
          return 1;
        }
      } break;
      case 'p': {
        if (argv[cntr][2] == '=') {
          past = strtoul(&argv[cntr][3], NULL, 0);
        } else {
          printf("Unknown argument: %s !\n", argv[cntr]);
          return 1;
        }
      } break;
      case 'r': {
        read_only = 1;
        printf("Only outputting data. Monitoring disabled.\n");
      } break;
      case 't': {
        if (argv[cntr][2] == '=') {
          timeout = strtoul(&argv[cntr][3], NULL, 0);
          printf("Timeout enabled. Shutting down after %lu minutes...\n",
                 timeout);
        } else {
          printf("Unknown argument: %s !\n", argv[cntr]);
          return 1;
        }
      } break;
      case 'w': {
        if (argv[cntr][2] == '=') {
          out_delay = strtoul(&argv[cntr][3], NULL, 0);
        } else {
          printf("Unknown argument: %s !\n", argv[cntr]);
          return 1;
        }
      } break;
      default: {
        printf("Unknown option: %c !\n", argv[cntr][1]);
        print_usage();
        return 1;
      } break;
      }
    } else {
      printf("Unknown argument: %s !\n", argv[cntr]);
      print_usage();
      return 1;
    }
  }

  // check file paths or set defaults
  // output file
  if (output == NULL) {
    output = (char *)calloc(strlen(DEFAULT_OUTPUT) + 1, sizeof(char));
    strncpy(output, DEFAULT_OUTPUT, strlen(DEFAULT_OUTPUT));
  }
  if (test_path(output) == 1) {
    printf(
        "ERROR! Path for output file not valid or insufficient permissions!");
    cleanup();
    return 1;
  } else {
    remove(output);
  }

  // database file
  if (dbfile == NULL) {
    dbfile = (char *)calloc(strlen(DEFAULT_DB) + 1, sizeof(char));
    strncpy(dbfile, DEFAULT_DB, strlen(DEFAULT_DB));
  }
  int ret = test_path(dbfile);
  if (ret == 1) {
    printf(
        "ERROR! Path for database file not valid or insufficient permissions!");
    cleanup();
    return 1;
  }
  if (ret == 2) {
    new_dbfile = 1;
  }

  // log file
  if (logfile == NULL) {
    logfile = (char *)calloc(strlen(DEFAULT_LOG) + 1, sizeof(char));
    strncpy(logfile, DEFAULT_LOG, strlen(DEFAULT_LOG));
  }
  if (test_path(logfile) == 1) {
    printf("ERROR! Path for log file not valid or insufficient permissions!");
    cleanup();
    return 1;
  }

  // set output fimeframe default
  if (output_mins == 0) {
    output_mins = DEFAULT_OUTPUT_MINS;
  }

  // sets output update delay default
  if (out_delay == 0) {
    out_delay = DEFAULT_OUT_DELAY;
  }
  out_delay *= 1000;

  // convert timeout from minutes to unit of 100 milliseconds if set
  if (timeout) {
    timeout *= 600;
  }

  unsigned int to = (unsigned int)past;
  unsigned int from = (unsigned int)(past + output_mins);

  // initialize the modules
  printf("Initializing...\n");
  init_log(logfile);
  if (new_dbfile) {
    remove(dbfile);
  }
  init_db(dbfile, new_dbfile);
  init_watch(!read_only);
  init_out(output, out_delay, &from, &to, read_only);
  printf("Running... ");
  if (!read_only) {
    char running = 1;

    OTHR *thread = (OTHR *)malloc(sizeof(OTHR));
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    thread->spinlock = &mutex;

    if (!autostarted) {
      // start thread to check for user input
      start_other_thread(thread, 0, &check_q);
    }
    // main loop
    printf("Press 'Q'-Enter to stop!\n");
    while (running) {
      if (!autostarted) {
        pthread_mutex_lock(thread->spinlock);
        running = thread->running;
        pthread_mutex_unlock(thread->spinlock);
      }
      sleep_milliseconds(100);

      if (timeout > 0) {
        timeout--;
        if (timeout == 0) {
          printf("Timeout! Stopping...\n");
          logm("Timeout hit! Stopping...\n");
          if (!autostarted) {
            pthread_cancel(thread->t_id);
          }
          running = 0;
        }
      }
    }

    free(thread);
    pthread_mutex_destroy(&mutex);

  } else {
    printf("\n");
    refresh_out_time();
  }
  cleanup();
  return 0;
}

static void *check_q(void *args) {
  OTHR *inf = args;
  char cin;
  int old_cancel;
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old_cancel);
  while (1) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old_cancel);
    cin = getchar();
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old_cancel);
    if (cin == 'q' || cin == 'Q') {
      printf("Stopping...\n");
      pthread_mutex_lock(inf->spinlock);
      inf->running = 0;
      pthread_mutex_unlock(inf->spinlock);
      pthread_exit(NULL);
    }
  }
}

int test_path(char *fpath) {
  FILE *test = fopen(fpath, "r");
  if (test == NULL) {
    test = fopen(fpath, "w+");
    if (test == NULL) {
      return 1;
    }
    fclose(test);
    return 2;
  }
  fclose(test);
  return 0;
}

void print_usage(void) {
  printf("Usage: sleepwatch [OPTIONS]\n\n"
         "Options:\n"
         "  -a          Autostarted mode: Will not start a thread to check for "
         "user input.\n"
         "  -d=foo/bar  Path to and name of database file. Default is current "
         "directory.\n"
         "  -f=foo/bar  Path to and name of output file. Default is current "
         "directory.\n"
         "  -l=foo/bar  Path to and name of log file. Default is current "
         "directory.\n"
         "  -n          Start new database. WARNING: ALL data will be ERASED!\n"
         "  -o=x        Output data of a timeframe of x minutes.\n"
         "  -p=x        Output data from x minutes in the past.\n"
         "  -r          Only read from database, no monitoring.\n"
         "  -t=x        Run for x minutes. Default is infinite.\n"
         "  -w=x        Wait x seconds between refreshing the output file. "
         "Default is 30 seconds.\n"
         "\n");
}

void cleanup(void) {
  close_out();
  close_watch();
  close_db();
  printf("Manually stopped. Wrote data to %s<x>vals!\n", output);
  logm("INFO successfully shut down!");
  free(output);
  free(logfile);
  free(dbfile);
}
