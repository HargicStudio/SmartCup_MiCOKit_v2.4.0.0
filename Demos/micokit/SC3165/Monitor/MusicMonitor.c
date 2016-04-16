


/***

History:
2015-12-22: Ted: Create

*/


#include "MusicMonitor.h"
#include "If_MO.h"
#include "user_debug.h"
#include "controllerBus.h"
#include "SendJson.h"


#ifdef DEBUG
  #define user_log(M, ...) custom_log("MusicMonitor", M, ##__VA_ARGS__)
  #define user_log_trace() custom_log_trace("MusicMonitor")
#else
#ifdef USER_DEBUG
  #define user_log(M, ...) user_debug("MusicMonitor", M, ##__VA_ARGS__)
  #define user_log_trace() (void)0
#endif
#endif


#define MAX_WAITING_TRACK_NUM   5

#define STACK_SIZE_MUSIC_THREAD          0x400


static mico_thread_t music_monitor_thread_handle = NULL;
static mico_queue_t handler_queue = NULL;


static void music_thread(void* arg);


OSStatus MusicInit(app_context_t *app_context)
{
    OSStatus err;

    err = mico_rtos_init_queue(&handler_queue, "HandleQueue", sizeof(STrack), MAX_WAITING_TRACK_NUM);
    require_noerr_action(err, exit, user_log("[ERR]MusicInit: handler_queue initialize failed"));
    require(handler_queue, exit);
    user_log("[DBG]MusicInit: music initialize success");
    
#if 1
    // start the music monitor thread
    err = mico_rtos_create_thread(&music_monitor_thread_handle, MICO_APPLICATION_PRIORITY, "music_monitor", 
                                  music_thread, STACK_SIZE_MUSIC_THREAD, 
                                  app_context );
    require_noerr_action( err, exit, user_log("[ERR]MusicInit: create music thread failed") );
    user_log("[DBG]MusicInit: create music thread success");
#endif
    
exit:
    return err;
}

static void music_thread(void* arg)
{
    app_context_t *app_context = (app_context_t *)arg;
    user_log_trace();

    STrack track;
  
    /* thread loop */
    while(1) {
        if(kNoErr == mico_rtos_pop_from_queue(&handler_queue, &track, MICO_WAIT_FOREVER)) {
            SendJsonTrack(app_context, &track);
        }
    }

//    mico_rtos_delete_thread(NULL);  // delete current thread
}

OSStatus UploadTrack(u16 tracknum, char* trackname)
{
    OSStatus err;
    STrack track;

    memset(&track, 0, sizeof(STrack));
    track.trackIdx = tracknum;
    strcpy(track.trackName, trackname);
    err = mico_rtos_push_to_queue(&handler_queue, &track, MICO_WAIT_FOREVER);

    return err;
}
/*
void AddTrack(u16 idx, char *pMp3FileName)
{
    
}*/

// end of file


