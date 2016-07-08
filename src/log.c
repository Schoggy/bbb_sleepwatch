#include "log.h"

int init_log(char* str_logfile){
  FILE* test;
  test = fopen(str_logfile, "r");
  if(test == NULL){
    fprintf(stderr, "ERROR opening logfile failed!");
    return 1;
  } else {
    strncpy(logfile, str_logfile, sizeof(str_logfile));
    fclose(test);
    logm("INFO logging initiated!");
  }
  return 0;
}

void logm(char* message){
  FILE* log = fopen(logfile, "a+");
  if(log != NULL){
    fprintf(log, "%s", message);
  } else {
    fprintf(stderr, "ERROR log file could not be opened!");
    fprintf(stderr, "%s", message);
  }
  fclose(log);
}

void logn(char* message, int nr){
  char *temp;
  char str_nr[13];
  temp = (char*) malloc((sizeof(message) + 13) * sizeof(char));
  strncpy(temp, message, sizeof(message));
  snprintf(str_nr, 13, " : %i", nr);
  strncat(temp, str_nr, 13);
  logm(temp);
  free(temp);
}

void logc(char* message, char* code){
  char *temp;
  temp = (char*) malloc(sizeof(message) + sizeof(code) * sizeof(char));
  strncpy(temp, message, sizeof(message));
  strncat(temp, code, sizeof(code));
  logm(temp);
  free(temp);
}

void logcn(char* message, char* code, int nr){
  char *temp;
  temp = (char*) malloc(sizeof(message) + sizeof(code) * sizeof(char));
  strncpy(temp, message, sizeof(message));
  strncat(temp, code, sizeof(code));
  logn(temp, nr);
  free(temp);
}
