#include "sqlite_db.h"

int init_db(char* file){
  DBO *db;
  int sql_res;
  sql_res = sqlite3_open(file, &db);
  if(sql_res == SQLITE_OK){
    db_file = (char*) calloc(sizeof(file), sizeof(char));
    strncpy(db_file, file, sizeof(file));
  }
  
  char str_stmt[28];
  char sensnr;
  sqlite3_stmt *crt_stmt;
  for(sensnr = 0; sensnr < 5; sensnr++){
    char *tablename = get_tablename(sensnr);
    memset(str_stmt, '\0', 28);
    strncat(str_stmt, "SELECT max(mnr) FROM ", 21);
    strncat(str_stmt, tablename, 5);
    strncat(str_stmt, ";\0", 2);
    
    free(tablename);
    
    // "compile" the SQL statement
    sql_res = sqlite3_prepare_v2(db, str_stmt, sizeof(str_stmt), &crt_stmt, 0);
    
    if(sql_res != SQLITE_OK){
      logcn("ERROR parsing SQL statement! : ", str_stmt, sql_res);
      return 1;
    }
  
    // run the SQL statement
    sql_res = sqlite3_step(crt_stmt);
  
    if(sql_res != SQLITE_ROW || SQLITE_DONE){
      logcn("ERROR SQL statement was unable to complete! : ", str_stmt, sql_res);
      sqlite3_finalize(crt_stmt);
      return 1;
    }  
    mnr_cnts[sensnr] = sqlite3_column_int64(crt_stmt, 0);  
  }
  
  sqlite3_close(db);
  return sql_res;
}

int init_sql(char* file, DBO* db){
  return sqlite3_open(file, &db);
}

void exit_db(void){
  free(db_file);
}

void exit_sql(DBO* db){
  sqlite3_close(db);
}

int insert_db(char sensnr, int* value){
  DBO *db;
  STMT *crt_stmt;
  int sql_res;
  
  // get the SQL code in string form 
  crt_stmt = build_insert_stmt(sensnr, value);
  
  // init sqlite3
  sql_res = init_sql(db_file, db);
  if(sql_res != SQLITE_OK){
    logn("ERROR opening SQL database file!", sql_res);
    return sql_res;
  }
  
  // execute the SQL statement
  sql_res = exec_sql(crt_stmt, db);
  if(sql_res){
    logcn("ERROR executing SQL INSERT statement! : ", crt_stmt->stmnt, sql_res);
    exit_sql(db);
    return sql_res;
  }
  
  // free memory
  free(crt_stmt);
  
  // shut down the database
  exit_sql(db);
  return 0;
}

TABLE* query_db(char sensnr, char* s_time, char* e_time){
  DBO *db;
  STMT *crt_stmt;
  int sql_res;
  TABLE *result;
  
  // get the SQL code in string form 
  crt_stmt = build_query_time_stmt(sensnr, s_time, e_time);
  
  // init sqlite3
  sql_res = init_sql(db_file, db);
  if(sql_res != SQLITE_OK){
    logn("ERROR opening SQL database file!", sql_res);
    return NULL;
  }
  
  // execute the SQL statement
  result = exec_sql_ret(crt_stmt, db);
  if(result == NULL){
    logc("ERROR executing SQL query! : ", crt_stmt->stmnt);
    exit_sql(db);
    return NULL;
  }
    
  free(crt_stmt);
  
  // shut down the database
  exit_sql(db);
  return result;
}

TABLE* tail_db(char sensnr, int nr){
  DBO *db;
  STMT *crt_stmt;
  int sql_res;
  TABLE *result;
  
  // get the SQL code in string form 
  crt_stmt = build_query_tail_stmt(sensnr, nr);
  
  // init sqlite3
  sql_res = init_sql(db_file, db);
  if(sql_res != SQLITE_OK){
    logn("ERROR opening SQL database file!", sql_res);
    return NULL;
  }
  
  // execute the SQL statement
  result = exec_sql_ret(crt_stmt, db);
  if(result == NULL){
    logc("ERROR executing SQL query! : ", crt_stmt->stmnt);
    exit_sql(db);
    return NULL;
  }
    
  free(crt_stmt);
    
  // shut down the database
  exit_sql(db);
  return result;
}

int exec_sql(STMT* cstmt, DBO* db){
  sqlite3_stmt *crt_stmt;
  
  // "compile" the SQL statement
  int sql_res = sqlite3_prepare_v2(db, cstmt->stmnt, cstmt->length, &crt_stmt, 0);
    
  if(sql_res != SQLITE_OK){
    logcn("ERROR parsing SQL statement! : ", cstmt->stmnt, sql_res);
    return sql_res;
  }
  
  // run the SQL statement
  sql_res = sqlite3_step(crt_stmt);
  
  if(sql_res != SQLITE_DONE){
    logcn("ERROR SQL statement was unable to complete! : ", cstmt->stmnt, sql_res);
    return sql_res;
  }
  
  // clean up the statement
  sqlite3_finalize(crt_stmt);
  
  return 0;
}

TABLE* exec_sql_ret(STMT* cstmt, DBO* db){
  sqlite3_stmt *crt_stmt;
  TABLE *out;
  out = (TABLE*) calloc(1, sizeof(TABLE));
  out->linecount = 0;
  
  // "compile" the SQL statement
  int sql_res = sqlite3_prepare_v2(db, cstmt->stmnt, cstmt->length, &crt_stmt, 0);
    
  if(sql_res != SQLITE_OK){
    logcn("ERROR parsing SQL statement! : ", cstmt->stmnt, sql_res);
    return NULL;
  }
  
  // run the SQL statement
  sql_res = sqlite3_step(crt_stmt);
  
  if(sql_res != SQLITE_ROW || SQLITE_DONE){
    logcn("ERROR SQL statement was unable to complete! : ", cstmt->stmnt, sql_res);
    sqlite3_finalize(crt_stmt);
    return NULL;
  }
  
  // if no data returned
  if(sql_res == SQLITE_DONE){
    logcn("INFO SQL statement that was called with exec_sql_ret did not return any data! : ", cstmt->stmnt, sql_res);
    out->lines = NULL;
  } else {
    int buffsize = 32;
    LINE *lines;
    lines = (LINE*) calloc(buffsize, sizeof(LINE));
    
    // continue statement as long as there is data
    while(sql_res == SQLITE_ROW){
      out->linecount++;
      
      // if necessary resize buffer
      if(out->linecount > buffsize){
        buffsize += buffsize / 2;
        lines = (LINE*) realloc(lines, buffsize * sizeof(LINE));
      }
      
      // collect data
      lines->mnr = sqlite3_column_int64(crt_stmt, 0);
      strncpy(lines->mtimestamp, sqlite3_column_text(crt_stmt, 1), 20);
      lines->value = sqlite3_column_int(crt_stmt, 2);
      
      // next step
      sql_res = sqlite3_step(crt_stmt);
    }
    
    // reallocate so there are no empty lines
    lines = (LINE*) realloc(lines, out->linecount * sizeof(LINE));
    
    out->lines = lines;
    if(sql_res != SQLITE_DONE){
      logcn("ERROR while stepping through SQL statement with return values! : ", cstmt->stmnt, sql_res);
      sqlite3_finalize(crt_stmt);
      return out;
    }
  }
  // clean up the statement
  sqlite3_finalize(crt_stmt);
  
  return out;
}

STMT* build_insert_stmt(char sensnr, int* value){
  STMT *out;
  out = (STMT*) calloc(1, sizeof(STMT)); 
  char str_stmt[100];
  char tempstr[73];
  // get database table name
  char *tablename = get_tablename(sensnr);
  
  // build the statement
  memset(str_stmt, '\0', 100);
  strncpy(str_stmt, "INSERT INTO ", 12);
  strncat(str_stmt, tablename, 5);
  snprintf(tempstr, 73," (mnr, mtimestamp, value) VALUES (%li, datetime('now'), %i);\0", mnr_cnts[sensnr] + 1, *value);
  strncat(str_stmt, tempstr, 73);  
  
  free(tablename);
  
  // set output
  out->length = 100;
  out->stmnt = str_stmt;
  
  return out; 
}

STMT* build_query_tail_stmt(char sensnr, int nr){
  STMT *out;
  out = (STMT*) calloc(1, sizeof(STMT));
  char str_stmt[100];
  char tempstr[16];
  // get database table name
  char *tablename = get_tablename(sensnr);
  
  // build the statement
  memset(str_stmt, '\0', 100);
  strncpy(str_stmt, "SELECT * FROM ", 14);
  strncat(str_stmt, tablename, 5);
  strncat(str_stmt, " WHERE mnr > (SELECT max(mnr) FROM ", 35);
  strncat(str_stmt, tablename, 5);
  snprintf(tempstr, 16, ") - %i;\0", nr);
  strncat(str_stmt, tempstr, 12);
  
  free(tablename);
  
  out->length = 100;
  out->stmnt = str_stmt;
  
  return out; 
}

STMT* build_query_time_stmt(char sensnr, char* from, char* to){
  STMT *out;
  out = (STMT*) calloc(1, sizeof(STMT));
  out->length = 52 + sizeof(from) + sizeof(to);
  char str_stmt[out->length];
  char* tablename = get_tablename(sensnr);
  
  // build the statement
  memset(str_stmt, '\0', out->length);
  strncpy(str_stmt, "SELECT * FROM ", 14);
  strncat(str_stmt, tablename, 5);
  strncat(str_stmt, " WHERE mtimestamp BETWEEN ", 26);
  strncat(str_stmt, from, sizeof(from));
  strncat(str_stmt, " AND ", 5);
  strncat(str_stmt, to, sizeof(to));
  strncat(str_stmt, ";\0", 2);
  
  free(tablename);
  
  out->stmnt = str_stmt;
  
  return out; 
}

char* get_tablename(char sensnr){
  char *out;
  out = (char*) calloc(6, sizeof(char));
  // set initial tablename
  strncpy(out, "xvals\0", 6);
  
  // replace char 0 with the appropriate char
  switch(sensnr){
    case 0: {
      out[0] = 'l';
    } break;
    case 1: {
      out[1] = 'm';
    } break;
    case 2: {
      out[2] = 't';
    } break;
    case 3: {
      out[3] = 'h';
    } break;
    case 4: {
      out[4] = 'g';
    } break;
    default: {
      logn("ERROR invalid sensor number in get_tablename!", sensnr);
      return NULL;
    } break;
  }
  return out;  
}


int build_new_db(void){
  char str_stmt[137];
  char sensnr;
  STMT crt_stmt;
  int sql_res;
  DBO *db;
  
  sql_res = init_sql(db_file, db);
  if(sql_res != SQLITE_OK){
    logn("ERROR opening SQL database file!", sql_res);
    return sql_res;
  }
  
  crt_stmt.length = 137;
  crt_stmt.stmnt = str_stmt;
  
  for(sensnr = 0; sensnr < 5; sensnr++){
    char *tablename = get_tablename(sensnr);
    memset(str_stmt, '\0', 137);
    strncpy(str_stmt, "CREATE TABLE ", 13);
    strncat(str_stmt, tablename, 5);
    strncat(str_stmt, " (mnr NUMBER NOT NULL ,mtimestamp VARCHAR NOT NULL ,value NUMBER NOT NULL ,CONSTRAINT ", 86);
    strncat(str_stmt, tablename, 5);
    strncat(str_stmt, "_mnr_pk PRIMARY KEY (mnr));\0", 28);
    
    free(tablename);
    
    sql_res = exec_sql(&crt_stmt, db);
    if(sql_res){
      logcn("ERROR create table statement failed! : ", str_stmt, sql_res);
      exit_sql(db);
      return sql_res;
    }
  }
  
  exit_sql(db);
  logc("INFO successfully initialized database with empty tables in: ", db_file);
  return 0;
}


