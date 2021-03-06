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
#include "sqlite_db.h"

int init_db(char *file, char newdb) {
  int sql_res;
  sql_res = sqlite3_open(file, &db);
  if (sql_res) {
    logn("ERROR opening SQL database! : ", sql_res);
    return sql_res;
  }
  if (newdb) {
    if (build_new_db() != 0) {
      logc("ERROR building new database in file: ", file);
      return 1;
    }
    logc("INFO successfully initialized database with empty tables in: ", file);
  }

  char *str_stmt = (char *)calloc(28, sizeof(char));
  char sensnr;
  sqlite3_stmt *crt_stmt;
  for (sensnr = 0; sensnr < 5; sensnr++) {
    char *tablename = get_tablename(sensnr);
    strncat(str_stmt, "SELECT max(mnr) FROM ", 21);
    strncat(str_stmt, tablename, 5);
    strncat(str_stmt, ";\0", 2);

    free(tablename);

    // "compile" the SQL statement
    sql_res = sqlite3_prepare_v2(db, str_stmt, -1, &crt_stmt, 0);

    if (sql_res != SQLITE_OK) {
      logcn("ERROR parsing SQL statement! : ", str_stmt, sql_res);
      close_db();
      return 1;
    }

    // run the SQL statement
    sql_res = sqlite3_step(crt_stmt);

    if (sql_res != SQLITE_ROW && sql_res != SQLITE_DONE) {
      logcn("ERROR SQL statement was unable to complete! : ", str_stmt,
            sql_res);
      sqlite3_finalize(crt_stmt);
      close_db();
      free(str_stmt);
      return 1;
    }
    memset(str_stmt, '\0', 28);
    mnr_cnts[sensnr] = sqlite3_column_int64(crt_stmt, 0);
    sqlite3_finalize(crt_stmt);
  }
  free(str_stmt);
  return 0;
}

void close_db(void) { sqlite3_close(db); }

int insert_db(char sensnr, int *value) {
  STMT *crt_stmt;
  int sql_res;

  // get the SQL code in string form
  crt_stmt = build_insert_stmt(sensnr, value);

  // execute the SQL statement
  sql_res = exec_sql(crt_stmt);
  if (sql_res) {
    logcn("ERROR executing SQL INSERT statement! : ", crt_stmt->stmnt, sql_res);
    return sql_res;
  }
  mnr_cnts[sensnr]++;
  // free memory
  destroy_stmt(crt_stmt);
  return 0;
}

TABLE *query_db(char sensnr, char *s_time, char *e_time) {
  STMT *crt_stmt;
  TABLE *result;

  // get the SQL code in string form
  crt_stmt = build_query_time_stmt(sensnr, s_time, e_time);

  // execute the SQL statement
  result = exec_sql_ret(crt_stmt);
  if (result == NULL) {
    logc("ERROR executing SQL query! : ", crt_stmt->stmnt);
    return NULL;
  }

  destroy_stmt(crt_stmt);
  return result;
}

TABLE *tail_db(char sensnr, int nr) {
  STMT *crt_stmt;
  TABLE *result;

  // get the SQL code in string form
  crt_stmt = build_query_tail_stmt(sensnr, nr);

  // execute the SQL statement
  result = exec_sql_ret(crt_stmt);
  if (result == NULL) {
    logc("ERROR executing SQL query! : ", crt_stmt->stmnt);
    return NULL;
  }

  destroy_stmt(crt_stmt);

  return result;
}

int exec_sql(STMT *cstmt) {
  sqlite3_stmt *crt_stmt;
  int sql_res;

  // "compile" the SQL statement
  sql_res = sqlite3_prepare_v2(db, cstmt->stmnt, -1, &crt_stmt, 0);

  if (sql_res != SQLITE_OK) {
    logcn("ERROR parsing SQL statement! : ", cstmt->stmnt, sql_res);
    close_db();
    return sql_res;
  }

  // run the SQL statement
  sql_res = sqlite3_step(crt_stmt);

  if (sql_res != SQLITE_DONE) {
    logcn("ERROR SQL statement was unable to complete! : ", cstmt->stmnt,
          sql_res);
    sqlite3_finalize(crt_stmt);
    close_db();
    return sql_res;
  }

  // clean up the statement
  sqlite3_finalize(crt_stmt);

  return 0;
}

TABLE *exec_sql_ret(STMT *cstmt) {
  sqlite3_stmt *crt_stmt;
  TABLE *out;
  int sql_res;
  out = (TABLE *)calloc(1, sizeof(TABLE));
  out->linecount = 0;

  // "compile" the SQL statement
  sql_res = sqlite3_prepare_v2(db, cstmt->stmnt, -1, &crt_stmt, 0);

  if (sql_res != SQLITE_OK) {
    logcn("ERROR parsing SQL statement! : ", cstmt->stmnt, sql_res);
    close_db();
    free(out);
    return NULL;
  }

  // run the SQL statement
  sql_res = sqlite3_step(crt_stmt);

  if (sql_res != SQLITE_ROW && sql_res != SQLITE_DONE) {
    logcn("ERROR SQL statement was unable to complete! : ", cstmt->stmnt,
          sql_res);
    sqlite3_finalize(crt_stmt);
    close_db();
    free(out);
    return NULL;
  }

  // if no data returned
  if (sql_res == SQLITE_DONE) {
    logcn("INFO SQL statement that was called with exec_sql_ret did not return "
          "any data! : ",
          cstmt->stmnt, sql_res);
    out->lines = NULL;
  } else {
    int buffsize = 32;
    LINE *lines;
    lines = (LINE *)calloc(buffsize, sizeof(LINE));
    LINE *lines_new;

    // continue statement as long as there is data
    while (sql_res == SQLITE_ROW) {
      out->linecount++;

      // if necessary resize buffer
      if (out->linecount > buffsize) {
        buffsize += buffsize / 2;
        lines_new = (LINE *)realloc(lines, buffsize * sizeof(LINE));
        if (lines_new == NULL) {
          logn("ERROR not enough memory! realloc failed for size of:",
               buffsize * sizeof(LINE));
          free(lines);
          return NULL;
        } else {
          lines = lines_new;
        }
      }

      // collect data
      (lines + out->linecount - 1)->mnr = sqlite3_column_int64(crt_stmt, 0);
      strcpy((lines + out->linecount - 1)->mtimestamp,
             sqlite3_column_text(crt_stmt, 1));
      (lines + out->linecount - 1)->value = sqlite3_column_int(crt_stmt, 2);

      // next step
      sql_res = sqlite3_step(crt_stmt);
    }

    // reallocate so there are no empty lines
    lines_new = (LINE *)realloc(lines, out->linecount * sizeof(LINE));
    if (lines_new == NULL) {
      logn("ERROR not enough memory! realloc failed for size of:",
           buffsize * sizeof(LINE));
      free(lines);
      return NULL;
    } else {
      lines = lines_new;
    }
    out->lines = lines;
    if (sql_res != SQLITE_DONE) {
      logcn("ERROR while stepping through SQL statement with return values! : ",
            cstmt->stmnt, sql_res);
      sqlite3_finalize(crt_stmt);
      close_db();
      return out;
    }
  }
  // clean up the statement
  sqlite3_finalize(crt_stmt);
  return out;
}

STMT *build_insert_stmt(char sensnr, int *value) {
  STMT *out = (STMT *)malloc(sizeof(STMT));
  char *str_stmt = (char *)calloc(100, sizeof(char));
  char *tempstr = (char *)calloc(73, sizeof(char));
  // get database table name
  char *tablename = get_tablename(sensnr);

  // build the statement
  strncpy(str_stmt, "INSERT INTO ", 12);
  strncat(str_stmt, tablename, 5);
  sprintf(tempstr,
          " (mnr, mtimestamp, value) VALUES (%li, datetime('now'), %i);",
          mnr_cnts[sensnr] + 1, *value);
  strncat(str_stmt, tempstr, strlen(tempstr));

  free(tablename);
  free(tempstr);

  // set output
  out->length = 100;
  out->stmnt = str_stmt;

  return out;
}

STMT *build_query_tail_stmt(char sensnr, int nr) {
  STMT *out = (STMT *)malloc(sizeof(STMT));
  char *str_stmt = (char *)calloc(100, sizeof(char));
  char *tempstr = (char *)calloc(16, sizeof(char));
  // get database table name
  char *tablename = get_tablename(sensnr);

  // build the statement
  strncpy(str_stmt, "SELECT * FROM ", 14);
  strncat(str_stmt, tablename, 5);
  strncat(str_stmt, " WHERE mnr > (SELECT max(mnr) FROM ", 35);
  strncat(str_stmt, tablename, 5);
  snprintf(tempstr, 16, ") - %i;", nr);
  strncat(str_stmt, tempstr, 11);

  free(tablename);
  free(tempstr);

  out->length = 100;
  out->stmnt = str_stmt;

  return out;
}

STMT *build_query_time_stmt(char sensnr, char *from, char *to) {
  STMT *out = (STMT *)malloc(sizeof(STMT));
  out->length = 52 + strlen(from) + strlen(to);
  char *str_stmt = (char *)calloc(out->length, sizeof(char));
  char *tablename = get_tablename(sensnr);

  // build the statement
  strncpy(str_stmt, "SELECT * FROM ", 14);
  strncat(str_stmt, tablename, 5);
  strncat(str_stmt, " WHERE mtimestamp BETWEEN ", 26);
  strncat(str_stmt, from, strlen(from));
  strncat(str_stmt, " AND ", 5);
  strncat(str_stmt, to, strlen(to));
  strncat(str_stmt, ";", 1);

  free(tablename);

  out->stmnt = str_stmt;

  return out;
}

char *get_tablename(char sensnr) {
  char *out;
  out = (char *)calloc(6, sizeof(char));
  // set initial tablename
  strncpy(out, "xvals", 5);

  // replace char 0 with the appropriate char
  switch (sensnr) {
  case 0: {
    out[0] = 'l';
  } break;
  case 1: {
    out[0] = 'm';
  } break;
  case 2: {
    out[0] = 't';
  } break;
  case 3: {
    out[0] = 'h';
  } break;
  case 4: {
    out[0] = 'g';
  } break;
  default: {
    logn("ERROR invalid sensor number in get_tablename!", sensnr);
    free(out);
    return NULL;
  } break;
  }
  return out;
}

int build_new_db(void) {
  char *str_stmt = (char *)malloc(137 * sizeof(char));
  char sensnr;
  STMT *crt_stmt = (STMT *)malloc(sizeof(STMT));
  int sql_res;

  crt_stmt->length = 137;
  crt_stmt->stmnt = str_stmt;

  for (sensnr = 0; sensnr < 5; sensnr++) {
    char *tablename = get_tablename(sensnr);
    memset(str_stmt, '\0', 137);
    strncpy(str_stmt, "CREATE TABLE ", 13);
    strncat(str_stmt, tablename, 5);
    strncat(str_stmt, " (mnr NUMBER NOT NULL ,mtimestamp VARCHAR NOT NULL "
                      ",value NUMBER NOT NULL ,CONSTRAINT ",
            86);
    strncat(str_stmt, tablename, 5);
    strncat(str_stmt, "_mnr_pk PRIMARY KEY (mnr));", 27);

    free(tablename);

    sql_res = exec_sql(crt_stmt);
    if (sql_res) {
      logcn("ERROR create table statement failed! : ", str_stmt, sql_res);
      return sql_res;
    }
  }
  destroy_stmt(crt_stmt);

  return 0;
}

void destroy_stmt(STMT *cstmt) {
  free(cstmt->stmnt);
  free(cstmt);
}

void destroy_table(TABLE *t) {
  free(t->lines);
  free(t);
}
