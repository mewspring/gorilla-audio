#include "gorilla/ga.h"

#include <memory.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char** argv)
{
  ga_Format fmt;
  ga_Device* dev;
  gc_int16* buf;
  gc_int32 numSamples;
  gc_int32 sampleSize;
  gc_int32 numToQueue;
  gc_int32 i;
  gc_int16 sample;
  gc_float32 pan = 1.0f;
  gc_float32 t = 0.0f;

  gc_initialize(0);
  dev = ga_device_open(GA_DEVICE_TYPE_OPENAL, 2);
  if(!dev)
    return 1;

  memset(&fmt, 0, sizeof(ga_Format));
  fmt.bitsPerSample = 16;
  fmt.numChannels = 2;
  fmt.sampleRate = 44100;
  numSamples = 2048;
  sampleSize = ga_format_sampleSize(&fmt);
  buf = (gc_int16*)malloc(numSamples * sampleSize);

  while(1)
  {
    numToQueue = ga_device_check(dev);
    while(numToQueue--)
    {
      for(i = 0; i < numSamples * 2; i = i + 2)
      {
        sample = (gc_int16)(sin(t) * 32768);
        sample = (sample > -32768 ? (sample < 32767 ? sample : 32767) : -32768);
        pan = (gc_float32)sin(t / 300) / 2.0f + 0.5f;
        buf[i] = (gc_int16)(sample * pan);
        buf[i + 1] = (gc_int16)(sample * (1.0f - pan));
        t = t + 0.03f;
        if(t > 3.14159265f)
          t -= 3.14159265f;
      }
      ga_device_queue(dev, &fmt, numSamples, (char*)buf);
    }
  }
  ga_device_close(dev);
  gc_shutdown();

  free(buf);

  return 0;
}
