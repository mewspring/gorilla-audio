#include "gorilla/ga.h"

int main(int argc, char** argv)
{
  ga_Device* dev;

  ga_initialize(0);
  dev = ga_device_open(GA_DEVICE_TYPE_OPENAL);
  
  ga_device_close(dev);
  ga_shutdown();

  return 0;
}
