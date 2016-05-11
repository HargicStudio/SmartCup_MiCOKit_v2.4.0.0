


/***

History:
2016-04-30: Ted: Create

*/

#ifndef _APIINTERNALMSG_H_
#define _APIINTERNALMSG_H_

#ifdef __cplusplus
 extern "C" {
#endif 


#include "AaPlatform.h"
#include <stdbool.h>
#include "If_MO.h"


enum {
    API_MESSAGE_ID_NONE = 0x0000,
    // platform layer
    
    // application layer
    API_MESSAGE_ID_TFSTATUS_REQ = 0x2001,
    API_MESSAGE_ID_TFSTATUS_RESP,
    
    API_MESSAGE_ID_TRACKNUM_REQ,
    API_MESSAGE_ID_TRACKNUM_RESP,
    
    API_MESSAGE_ID_TRACKNAME_REQ,
    API_MESSAGE_ID_TRACKNAME_RESP,

    API_MESSAGE_ID_PLAY_REQ,
    API_MESSAGE_ID_PLAY_RESP,

    API_MESSAGE_ID_DELETE_REQ,
    API_MESSAGE_ID_DELETE_RESP,

    API_MESSAGE_ID_ADD_REQ,
    API_MESSAGE_ID_ADD_RESP,

    API_MESSAGE_ID_QUIT_REQ,
    API_MESSAGE_ID_QUIT_RESP,

    API_MESSAGE_ID_VOLUME_REQ,
    API_MESSAGE_ID_VOLUME_RESP,

    API_MESSAGE_ID_ISPLAYING_REQ,
    API_MESSAGE_ID_ISPLAYING_RESP,

    API_MESSAGE_ID_PAUSE_REQ,
    API_MESSAGE_ID_PAUSE_RESP,

    API_MESSAGE_ID_TRACKLIST_REQ,
    
    API_MESSAGE_ID_MAX = 0xFFFF,
};


// API_MESSAGE_ID_TFSTATUS_REQ
typedef struct ApiTfStatusReq_t {
    u16 reserve;
} ApiTfStatusReq;

// API_MESSAGE_ID_TFSTATUS_RESP
typedef struct ApiTfStatusResp_t {
    bool status;
    u16  capacity;
    u16  free;
} ApiTfStatusResp;

// API_MESSAGE_ID_TRACKNUM_REQ
typedef struct ApiTrackNumReq_t {
    u8  type;
} ApiTrackNumReq;

// API_MESSAGE_ID_TRACKNUM_RESP
typedef struct ApiTrackNumResp_t {
    u8  type;
    u16 track_num;
} ApiTrackNumResp;

// API_MESSAGE_ID_TRACKNAME_REQ
typedef struct ApiTrackNameReq_t {
    u8  type;
    u16 track_index;
} ApiTrackNameReq;

// API_MESSAGE_ID_TRACKNAME_RESP
typedef struct ApiTrackNameResp_t {
    u8      type;
    bool    status;
    u16     track_index;
    char    name[TRACKNAME_MAX_LENGTH];
} ApiTrackNameResp;

// API_MESSAGE_ID_PLAY_REQ
typedef struct ApiPlayReq_t {
    u8  type;
    u16 track_index;
} ApiPlayReq;

// API_MESSAGE_ID_PLAY_RESP
typedef struct ApiPlayResp_t {
    u8   type;
    bool status;
} ApiPlayResp;

// API_MESSAGE_ID_DELETE_REQ
typedef struct ApiDeleteReq_t {
    u8      type;
    u16     track_index;
} ApiDeleteReq;

// API_MESSAGE_ID_DELETE_RESP
typedef struct ApiDeleteResp_t {
    bool status;
} ApiDeleteResp;

// API_MESSAGE_ID_ADD_REQ
typedef struct ApiAddReq_t {
    u8 reserve;
} ApiAddReq;

// API_MESSAGE_ID_ADD_RESP
typedef struct ApiAddResp_t {
    u8 reserve;
} ApiAddResp;

// API_MESSAGE_ID_QUIT_REQ
typedef struct ApiQiutReq_t {
    u8 reserve;
} ApiQuitReq;

// API_MESSAGE_ID_QUIT_RESP
typedef struct ApiQuitResp_t {
    bool status;
} ApiQuitResp;

// API_MESSAGE_ID_VOLUME_REQ
typedef struct ApiVolumeReq_t {
    u8 volume;
} ApiVolumeReq;

// API_MESSAGE_ID_VOLUME_RESP
typedef struct ApiVolumeResp_t {
    bool status;
} ApiVolumeResp;

// API_MESSAGE_ID_ISPLAYING_REQ
typedef struct ApiIsplayingReq_t {
    u8 reserve;
} ApiIsplayingReq;

// API_MESSAGE_ID_ISPLAYING_RESP
typedef struct ApiIsplayingResp_t {
    bool status;
} ApiIsplayingResp;

// API_MESSAGE_ID_PAUSE_REQ
typedef struct ApiPauseReq_t {
    bool pause;
} ApiPauseReq;

// API_MESSAGE_ID_PAUSE_RESP
typedef struct ApiPauseResp_t {
    bool status;
} ApiPauseResp;

// API_MESSAGE_ID_TRACKLIST_REQ
typedef struct {
    u8 reserve;
} ApiTrackListReq;



#ifdef __cplusplus
}
#endif

#endif // _APIINTERNALMSG_H_

// end of file


