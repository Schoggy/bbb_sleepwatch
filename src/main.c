#include "data_out.h"
#include "dht/common_dht_read.h"
#include "log.h"
#include "sqlite_db.h"
#include "watch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// default number timeframe of data to output
#define DEFAULT_OUTPUT_MINS 60

// default number of seconds between output file refreshes
#define DEFAULT_OUT_DELAY 30

// default file names
#define DEFAULT_OUTPUT "output"
#define DEFAULT_LOG "sleepwatch_log"
#define DEFAULT_DB "sleepwatch_db"

void print_usage(void);
int test_path(char *fpath);
void cleanup(void);

char *dbfile;
char *logfile;
char *output;

int main(int argc, char *argv[]) {

  char read_only = 0;
  char new_dbfile = 0;

  unsigned long timeout = 0;
  unsigned long output_mins = 0;
  unsigned long past = 0;
  unsigned long out_delay = 0;

  char cntr;
  for (cntr = 1; cntr < argc - 1; cntr++) {
    if (argv[cntr][0] == '-') {
      switch (argv[cntr][1]) {
      case 'd': {
        if (argv[cntr][2] == '=') {
          dbfile = (char *)calloc(strlen(&argv[cntr][3]), sizeof(char));
          strncpy(dbfile, &argv[cntr][3], strlen(&argv[cntr][3]));
        } else {
          printf("Unknown argument: %s !\n", argv[cntr]);
        }
      } break;
      case 'f': {
        if (argv[cntr][2] == '=') {
          output = (char *)calloc(strlen(&argv[cntr][3]), sizeof(char));
          strncpy(output, &argv[cntr][3], strlen(&argv[cntr][3]));
        } else {
          printf("Unknown argument: %s !\n", argv[cntr]);
        }
      } break;
      case 'n': {
        new_dbfile = 1;
        printf("Starting new database.\n");
      } break;
      case 'l': {
        if (argv[cntr][2] == '=') {
          logfile = (char *)calloc(strlen(&argv[cntr][3]), sizeof(char));
          strncpy(logfile, &argv[cntr][3], strlen(&argv[cntr][3]));
        } else {
          printf("Unknown argument: %s !\n", argv[cntr]);
        }
      } break;
      case 'o': {
        if (argv[cntr][2] == '=') {
          output_mins = strtoul(&argv[cntr][3], NULL, 0);
        } else {
          printf("Unknown argument: %s !\n", argv[cntr]);
        }
      } break;
      case 'p': {
        if (argv[cntr][2] == '=') {
          past = strtoul(&argv[cntr][3], NULL, 0);
        } else {
          printf("Unknown argument: %s !\n", argv[cntr]);
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
        }
      } break;
      case 'w': {
        if (argv[cntr][2] == '=') {
          out_delay = strtoul(&argv[cntr][3], NULL, 0);
        } else {
          printf("Unknown argument: %s !\n", argv[cntr]);
        }
      } break;
      default: {
        printf("Unknown option: %c !\n", argv[cntr][1]);
        print_usage();
      } break;
      }
    } else {
      printf("Unknown argument: %s !\n", argv[cntr]);
      print_usage();
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

  // convert timeout from minutes to unit of 100 milliseconds if set
  if (timeout) {
    timeout *= 600;
  }

  unsigned int to = (unsigned int)past;
  unsigned int from = (unsigned int)(past + output_mins);

  // initialize the modules
  printf("Initializing..");
  init_log(logfile);
  if (new_dbfile) {
    remove(dbfile);
  }
  init_db(dbfile, new_dbfile);
  init_watch(!read_only);
  init_out(output, out_delay, &from, &to);

  // main loop
  char running = 1;
  int cnt = 0;
  char cin;
  printf("Running... Press 'Q' to stop!");
  while (running) {
    while (!feof(stdin)) {
      cin = getc(stdin);
      if (cin == 'q' || cin == 'Q') {
        running = 0;
      }
    }
    sleep_milliseconds(100);
    cnt = (cnt + 1) % (out_delay * 10);

    if (!timeout) {
      timeout--;
      if (!timeout) {
        running = 0;
      }
    }
  }

  cleanup();
  return 0;
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
         "  -w=x        Wait x seconds between refreshing hte output file. "
         "Default is 30 seconds.\n"
         "\n");
}

void cleanup(void) {
  free(output);
  free(logfile);
  free(dbfile);
  close_watch();
  close_db();
  close_out();
}
