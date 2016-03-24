


/***

History:
2015-12-22: Ted: Create

*/

#ifndef _MUSICMONITOR_H
#define _MUSICMONITOR_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "Object_int.h"
#include "stdbool.h"
#include "mico.h"


OSStatus MusicInit();
OSStatus PlayingSong(u16 tracknum);


   
#ifdef __cplusplus
}
#endif

#endif // _MUSICMONITOR_H

// end of file


