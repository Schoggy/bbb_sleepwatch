#include "dht/common_dht_read.h"
#include "log.h"
#include "sqlite_db.h"
#include "watch.h"

int main() {
  // testcode!

  char logfile[8] = "testlog";
  char dbfile[7] = "testdb";

  init_log(logfile);
  init_watch();
  init_db(dbfile, 1);
  
  close_watch();
}
