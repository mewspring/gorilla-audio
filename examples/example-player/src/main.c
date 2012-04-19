#include "gorilla/ga.h"

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

static void exampleOnFinish(ga_Handle* in_handle, void* in_context)
{
  ga_int32 n = (ga_int32)in_context;
  printf("Sound #%d finished\n", n);
  ga_handle_destroy(in_handle);
}

int main(int argc, char** argv)
{
  ga_Format fmt;
  ga_Device* dev;
  ga_int16* buf;
  ga_int32 numSamples;
  ga_int32 sampleSize;
  ga_int32 numToQueue;
  ga_Mixer* mixer;
  ga_Sound* sound;
  ga_Handle* handle;

  ga_int32 i = 0;

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

  mixer = ga_mixer_create(&fmt, numSamples);

  sound = ga_sound_load("test.wav", GA_FILE_FORMAT_WAV, 0);

  while(1)
  {
    numToQueue = ga_device_check(dev);
    while(numToQueue--)
    {
      ga_mixer_mix(mixer, buf);
      ga_device_queue(dev, &fmt, numSamples, (char*)buf);
      if(i % 210 == 0)
      {
        handle = ga_handle_create(mixer, sound);
        ga_handle_setCallback(handle, &exampleOnFinish, 0);
        ga_handle_play(handle);
      }
      ++i;
    }
  }
  ga_mixer_destroy(mixer);
  ga_device_close(dev);
  ga_shutdown();

  free(buf);

  return 0;
}
