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
#include "log.h"

void init_log(char *str_logfile) { l_logfile = str_logfile; }

void logm(char *message) {
  FILE *log = fopen(l_logfile, "a+");
  if (log != NULL) {
    fwrite(message, sizeof(char), strlen(message), log);
    fputc('\n', log);
  } else {
    fprintf(stderr, "ERROR log file could not be opened!");
    fprintf(stderr, "%s", message);
  }
  fflush(log);
  fclose(log);
}

void logn(char *message, int nr) {
  char *temp;
  char str_nr[13];
  temp = (char *)calloc((strlen(message) + 13), sizeof(char));
  strncpy(temp, message, strlen(message));
  snprintf(str_nr, 13, " : %i", nr);
  strncat(temp, str_nr, 13);
  logm(temp);
  free(temp);
}

void logc(char *message, char *code) {
  char *temp;
  temp = (char *)calloc(strlen(message) + strlen(code), sizeof(char));
  strncpy(temp, message, strlen(message));
  strncat(temp, code, strlen(code));
  logm(temp);
  free(temp);
}

void logcn(char *message, char *code, int nr) {
  char *temp;
  temp = (char *)calloc(strlen(message) + strlen(code), sizeof(char));
  strncpy(temp, message, strlen(message));
  strncat(temp, code, strlen(code));
  logn(temp, nr);
  free(temp);
}
