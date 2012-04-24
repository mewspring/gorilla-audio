#include "gorilla/ga.h"
#include "gorilla/gau.h"
#include "gorilla/ga_thread.h"

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

/* Example-specific thread context */
typedef struct AudioThreadContext
{
  ga_Device* device;
  ga_Mixer* mixer;
  ga_Format* format;
  ga_int32 kill;
} AudioThreadContext;

/* Mix thread logic */
static ga_int32 mixThreadFunc(void* in_context)
{
  AudioThreadContext* context = (AudioThreadContext*)in_context;
  ga_Mixer* m = context->mixer;
  ga_int32 sampleSize = ga_format_sampleSize(context->format);
  ga_int16* buf = (ga_int16*)malloc(m->numSamples * sampleSize);
  while(!context->kill)
  {
    ga_int32 numToQueue = ga_device_check(context->device);
    while(numToQueue--)
    {
      ga_mixer_mix(m, buf);
      ga_device_queue(context->device, context->format, m->numSamples, buf);
    }
    ga_thread_sleep(5);
  }
  free(buf);
  printf("Mixer thread terminated.\n");
  return 0;
}

/* Stream thread logic */
static ga_int32 streamThreadFunc(void* in_context)
{
  AudioThreadContext* context = (AudioThreadContext*)in_context;
  ga_Mixer* m = context->mixer;
  while(!context->kill)
  {
    ga_mixer_stream(m);
    ga_thread_sleep(16);
  }
  printf("Stream thread terminated.\n");
  return 0;
}

int main(int argc, char** argv)
{
  ga_Format fmt;
  ga_Device* dev;
  ga_int32 numSamples;
  ga_Mixer* mixer;
  ga_Sound* sound;
  ga_Handle* handle;
  ga_Handle* stream;
  ga_Thread* mixThread;
  ga_Thread* streamThread;
  AudioThreadContext context;

  /* Initialize library */
  ga_initialize(0);

  /* Initialize device */
  dev = ga_device_open(GA_DEVICE_TYPE_OPENAL);
  if(!dev)
    return 1;

  /* Initialize mixer */
  memset(&fmt, 0, sizeof(ga_Format));
  fmt.bitsPerSample = 16;
  fmt.numChannels = 2;
  fmt.sampleRate = 44100;
  numSamples = 2048;
  mixer = ga_mixer_create(&fmt, numSamples);

  /* Initialize thread context */
  context.device = dev;
  context.mixer = mixer;
  context.format = &fmt;
  context.kill = 0;

  /* Create and run mixer and stream threads */
  mixThread = ga_thread_create(mixThreadFunc, &context, GA_THREAD_PRIORITY_HIGH, 64 * 1024);
  ga_thread_run(mixThread);

  streamThread = ga_thread_create(streamThreadFunc, &context, GA_THREAD_PRIORITY_HIGH, 64 * 1024);
  ga_thread_run(streamThread);

  /* Load static sound */
  sound = ga_sound_load("test1.wav", GA_FILE_FORMAT_WAV, 0);

  /* Create and play streaming sound */
  stream = gau_stream_file(mixer, 0, 131072, "test2.wav", GA_FILE_FORMAT_WAV, 0);
  ga_handle_setCallback(stream, &exampleOnFinish, 0);
  ga_handle_setParami(stream, GA_HANDLE_PARAM_LOOP, 0);
  ga_handle_play(stream);

  /* Bounded mix/queue/dispatch loop */
  {
    ga_int32 count = 0;

    while(!context.kill)
    {
      ga_mixer_dispatch(mixer);
      if(count % 200 == 0)
      {
        handle = ga_handle_create(mixer, sound);
        ga_handle_setCallback(handle, &exampleOnFinish, (void*)(count / 200));
        ga_handle_play(handle);
      }
      ++count;

      if(ga_handle_finished(stream))
      {
        context.kill = 1;
        ga_thread_join(mixThread);
        ga_thread_join(streamThread);
        break;
      }

      ga_thread_sleep(1);
    }
  }

  /* Clean up mixer/device/library */
  ga_mixer_destroy(mixer);
  ga_device_close(dev);
  ga_shutdown();

  return 0;
}
