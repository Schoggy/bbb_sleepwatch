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
