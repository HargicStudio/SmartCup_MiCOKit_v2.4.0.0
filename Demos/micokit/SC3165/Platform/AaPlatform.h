


/***

History:
2016-04-23: Ted: Create

*/

#ifndef _AAPLATFORM_H_
#define _AAPLATFORM_H_

#ifdef __cplusplus
 extern "C" {
#endif 


typedef signed char     i8;
typedef unsigned char   u8;
typedef signed int      i16;    // int is 32bit
typedef unsigned int    u16;
typedef signed long     i32;
typedef unsigned long   u32;


// not used yet
typedef unsigned int    EStatus;

enum {
    ENoErr = 0,
    EParamErr,
};


#include "include_for_mico.h"

   
#ifdef __cplusplus
}
#endif

#endif // _AAPLATFORM_H_

// end of file


