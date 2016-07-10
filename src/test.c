#include <stdlib.h>
#include <stdio.h>
#include "dht/BBB/bbb_dht_read.h"

#define GPIO_BASE 0
#define GPIO_NR 2

int main(){
  float temp = 0;
  float hum = 0;
  int dht_res;
  
  dht_res = bbb_dht_read(AM2302, GPIO_BASE, GPIO_NR, &hum, &temp);
  
  if(dht_res != DHT_SUCCESS){
    fprintf(stderr, "READ FAILED! ERR %i\n", dht_res);
    return dht_res;
  }
  
  printf("READ SUCCESS!\n\nHUM: %f \nTEMP: %f\n----\n", hum,temp);
  
  return 0;
}
