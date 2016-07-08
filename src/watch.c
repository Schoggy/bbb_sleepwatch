#include "watch.h"

void init_watch(){
  // init adc
  system("export SLOTS=/sys/devices/platform/bone_capemgr/slots");
  system("sh -c \"echo BB-ADC > $SLOTS\"");
  
  // init buffers
  char bufsizes[5] = {20, 250, 2, 2, 2};
  buffarr = (BUF*) malloc(5 * sizeof(BUF));
  char cntr = 0;
  for(;cntr < 5; cntr++){
    (buffarr + cntr)->s_ptr = (int*) malloc(bufsizes[cntr] * sizeof(int));
    (buffarr + cntr)->r_ptr = (buffarr + cntr)->s_ptr;
    (buffarr + cntr)->w_ptr = (buffarr + cntr)->s_ptr;
    (buffarr + cntr)->bufsize = bufsizes[cntr];
  }
}

void watch_sensor(char sensnr){
  
  
  switch(sensnr){
    case 0: {
    
    } break;
    case 0: {
    
    } break;
    case 0: {
    
    } break;
    case 0: {
    
    } break;
    case 0: {
    
    } break;
    default: {
    
    } break;
  }
  
}

int grab_value(char sensnr);
void close_buffers();
