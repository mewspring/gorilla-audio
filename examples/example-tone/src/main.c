#include "gorilla/ga.h"

#include <memory.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char** argv)
{
  ga_Format fmt;
  ga_Device* dev;
  ga_int16* buf;
  ga_int32 numSamples;
  ga_int32 sampleSize;
  ga_int32 numToQueue;
  ga_int32 i;
  ga_int16 sample;
  ga_float32 pan = 1.0f;
  ga_float32 t = 0.0f;

  ga_initialize(0);
  dev = ga_device_open(GA_DEVICE_TYPE_OPENAL);

  if(!dev)
    return 1;

  memset(&fmt, 0, sizeof(ga_Format));
  fmt.bitsPerSample = 16;
  fmt.numChannels = 2;
  fmt.sampleRate = 44100;

  numSamples = 2048;
  sampleSize = ga_format_sampleSize(&fmt);
  buf = (ga_int16*)malloc(numSamples * sampleSize);

  while(1)
  {
    numToQueue = ga_device_check(dev);
    while(numToQueue--)
    {
      for(i = 0; i < numSamples * 2; i = i + 2)
      {
        sample = (ga_int16)(sin(t) * 32768);
        sample = (sample > -32768 ? (sample < 32767 ? sample : 32767) : -32768);
        pan = sin(t / 300) / 2.0f + 0.5f;
        buf[i] = sample * pan;
        buf[i + 1] = sample * (1.0f - pan);
        t = t + 0.05f;
        if(t > 3.14159265f)
          t -= 3.14159265f;
      }
      ga_device_queue(dev, &fmt, numSamples, (char*)buf);
    }
  }
  ga_device_close(dev);
  ga_shutdown();

  free(buf);

  return 0;
}
