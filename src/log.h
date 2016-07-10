#ifndef SW_LOG_H
#define SW_LOG_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char logfile[16];

int init_log(char* str_logfile);

void logm(char* message);
void logn(char* message, int nr);
void logc(char* message, char* code);
void logcn(char* message, char* code, int nr);

#endif // SW_LOG_H
