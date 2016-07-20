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
#ifndef SW_SQLITE3_H
#define SW_SQLITE3_H

#include "log.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

/* Sensor numbers and table names:
 * Light level: 0, lvals
 * Noise level: 1, mvals
 * Temperature: 2, tvals
 * Humidity:    3, hvals
 * Air quality: 4, gvals
 */

#define LSENS 0
#define NSENS 1
#define TSENS 2
#define HSENS 3
#define GSENS 4

#define LTAB lvals
#define NTAB mvals
#define TTAB tvals
#define HTAB hvals
#define GTAB gvals

// can hold a line from the sqlite3 database
typedef struct db_line {
  long mnr;
  char mtimestamp[20];
  int value;
} LINE;

// struct for multiple lines from the database
typedef struct db_table {
  long linecount;
  LINE *lines;
} TABLE;

typedef struct str_stmnt {
  int length;
  char *stmnt;
} STMT;

typedef sqlite3 DBO;

// global database interface pointer
DBO *db;

// "API" FUNCTIONS

// holds highest mnr for each table (primary key)
long mnr_cnts[5];

// initialize database interface, create tables and/or init mnrcnts
int init_db(char *file, char newdb);

// executes the necessary create table statements
int build_new_db(void);

// adds value to the database
int insert_db(char sensnr, int *value);

// executes a select statement to grab values between s_time and e_time
TABLE *query_db(char sensnr, char *s_time, char *e_time);

// executes a select statement to grab the last nr of lines written to the
// database
TABLE *tail_db(char sensnr, int nr);

// releases all memory held by a TABLE variable
void destroy_table(TABLE *t);

// releases the database interface
void close_db(void);

// INTERNAL FUNCTIONS

// frees all memory held by a STMT variable
void destroy_stmt(STMT *cstmt);

// executes a SQL statement without a return value
int exec_sql(STMT *cstmt);

// executes a SQL statement with a return value in the form (LONG NUMBER,
// VARCHAR, NUMBER);
TABLE *exec_sql_ret(STMT *cstmt);

// returns an INSERT SQL statement for sensor sensnr and value
STMT *build_insert_stmt(char sensnr, int *value);

// builds a SELECT SQL statement to return the last nr of lines
STMT *build_query_tail_stmt(char sensnr, int nr);

// builds a SELECT SQL statement to return all lines between from and to
// from and to need to be sqlite "datetime()" statements
STMT *build_query_time_stmt(char sensnr, char *from, char *to);

// returns the name of the table for sensor sensnr
char *get_tablename(char sensnr);

#endif // SW_SQLITE3_H
