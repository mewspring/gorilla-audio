#include "gorilla/ga.h"
#include "gorilla/gau.h"

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/*#define LOOP*/

static void destroyOnFinish(ga_Handle* in_handle, void* in_context)
{
  ga_handle_destroy(in_handle);
}
int main(int argc, char** argv)
{
  ga_Device* dev;
  gau_Manager* mgr;
  ga_Mixer* mixer;
  ga_StreamManager* streamMgr;
  ga_Handle* stream;
#ifdef LOOP
  gau_SampleSourceLoop* loopSrc;
#endif /* LOOP */
  gc_int32 quit = 0;

  /* Initialize library */
  gc_initialize(0);

  /* Initialize device */
  dev = ga_device_open(GA_DEVICE_TYPE_OPENAL, 4);
  if(!dev)
    return 1;

  /* Initialize manager */
  mgr = gau_manager_create(dev, GAU_THREAD_POLICY_SINGLE, 512);
  mixer = gau_manager_mixer(mgr);
  streamMgr = gau_manager_streamManager(mgr);

  /* Create and play streaming audio */
  stream = gau_helper_stream_file(mixer, streamMgr,
                                  "test.ogg", "ogg",
                                  &destroyOnFinish, 0,
#ifdef LOOP
                                  &loopSrc, 0, -1);
#else
                                  0, 0, 0);
#endif /* LOOP */
  ga_handle_play(stream);

  /* Bounded mix/queue/dispatch loop */
  while(!quit)
  {
    gau_manager_update(mgr);
    if(ga_handle_finished(stream))
      quit = 1;
    else
    {
      printf("%d / %d\n", ga_handle_tell(stream, GA_TELL_PARAM_CURRENT), ga_handle_tell(stream, GA_TELL_PARAM_TOTAL));
      gc_thread_sleep(1);
    }
  }

  /* Clean up manager/device/library */
  gau_manager_destroy(mgr);
  ga_device_close(dev);
  gc_shutdown();

  return 0;
}
