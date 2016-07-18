#ifndef SW_LOG_H
#define SW_LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *l_logfile;

void init_log(char *str_logfile);

// write string message to log
void logm(char *message);

// write string message and integer nr to log
void logn(char *message, int nr);

// write string message and string code to log
void logc(char *message, char *code);

// write string message, code and integer nr to log
void logcn(char *message, char *code, int nr);

#endif // SW_LOG_H
