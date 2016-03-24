


/***

History:
2015-12-22: Ted: Create

*/


#include "MusicMonitor.h"
#include "If_MO.h"
#include "user_debug.h"


#ifdef DEBUG
  #define user_log(M, ...) custom_log("MusicMonitor", M, ##__VA_ARGS__)
  #define user_log_trace() custom_log_trace("MusicMonitor")
#else
#ifdef USER_DEBUG
  #define user_log(M, ...) user_debug("MusicMonitor", M, ##__VA_ARGS__)
  #define user_log_trace() (void)0
#endif
#endif


typedef struct SPlaylTrack_t {
    u16 trackNum;
} SPlayTrack;

#define MAX_WAITING_TRACK_NUM   5

#define STACK_SIZE_MUSIC_THREAD          0x400


static mico_thread_t music_monitor_thread_handle = NULL;
static mico_queue_t track_queue = NULL;


static void music_thread(void* arg);
static char* SongIndexMap(u16 trackindex);


OSStatus MusicInit()
{
    OSStatus err;

    err = mico_rtos_init_queue(&track_queue, "TrackQueue", sizeof(u16), MAX_WAITING_TRACK_NUM);
    require_noerr_action(err, exit, user_log("[ERR]MusicInit: track_queue initialize failed"));
    require(track_queue, exit);
    user_log("[DBG]MusicInit: track_queue initialize success");
    
#if 1
    // start the music monitor thread
    err = mico_rtos_create_thread(&music_monitor_thread_handle, MICO_APPLICATION_PRIORITY, "music_monitor", 
                                  music_thread, STACK_SIZE_MUSIC_THREAD, 
                                  NULL );
    require_noerr_action( err, exit, user_log("[ERR]MusicInit: create music thread failed") );
    user_log("[DBG]MusicInit: create music thread success");
#endif
    
exit:
    return err;
}

static void music_thread(void* arg)
{
    // avoid compiling warning
    arg = arg;
    user_log_trace();

    u8 i;
    SPlayTrack track = {0};
    char name[64];
    u8 music_num;

    music_num = MP3_getMp3FileNum("0:/default/");
    user_log("[DBG]music_thread: path 0:/default/ have mp3 file number %d", music_num);
    music_num = MP3_getMp3FileNum("0:/");
    user_log("[DBG]music_thread: path 0:/ have mp3 file number %d", music_num);

    for(i=0; i<music_num; i++) {
        //MP3_getMp3FileName("0:/", i, name);
        //AddTrack(i, name);
    }
  
    /* thread loop */
    while(1) {
        if(kNoErr == mico_rtos_pop_from_queue(&track_queue, &track, MICO_WAIT_FOREVER)) {
            // TODO: playing the song in while, get the track number by " *track "

            user_log("[DBG]music_thread: play song index(%d)", track.trackNum);

            sprintf(name, "0:/%s\0", SongIndexMap(track.trackNum));
            MP3_playSong(name);  //播放指定歌曲
            user_log("[DBG]music_thread: play song finished");
        }
    }

//    mico_rtos_delete_thread(NULL);  // delete current thread
}

static char* SongIndexMap(u16 trackindex)
{
    char* str = NULL;
    switch(trackindex) {
        case 1: str = "american_goldfinch.wav"; break;
        case 2: str = "american_robin.wav"; break;
        case 3: str = "baltimore_oriole.wav"; break;
        case 4: str = "blue_jay.wav"; break;
        case 5: str = "eastern_bluebird.wav"; break;
        case 6: str = "northern_cardinal.wav"; break;
        case 7: str = "red-winged blackbird.wav"; break;
        case 8: str = "wood_thrush_mono.wav"; break;
        case 9: str = "电池没电了.mp3"; break;
        case 10: str = "提醒及时喝水.mp3"; break;
        default: str = NULL; break;
    }

    return str;
}

OSStatus PlayingSong(u16 tracknum)
{
    OSStatus err;
    SPlayTrack track = {0};

    track.trackNum = tracknum;
    err = mico_rtos_push_to_queue(&track_queue, &track, MICO_WAIT_FOREVER);

    return err;
}
/*
void AddTrack(u16 idx, char *pMp3FileName)
{
    
}*/

// end of file


