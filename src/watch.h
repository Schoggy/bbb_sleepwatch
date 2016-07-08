#include <stdlib.h>

#define ADC_path

typedef struct data_buffer{
  int bufsize;
  int *r_ptr;
  int *w_ptr;
  int *s_ptr;
} BUF;

BUF *bufarr;

void init_watch();
void watch_sensor(char sensnr);
int grab_value(char sensnr);
void close_buffers();

