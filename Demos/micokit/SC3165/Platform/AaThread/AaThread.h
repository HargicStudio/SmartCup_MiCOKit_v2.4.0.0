


/***

History:
2016-04-23: Ted: Create

*/

#ifndef _AATHREAD_H_
#define _AATHREAD_H_

#ifdef __cplusplus
 extern "C" {
#endif 


#include "AaPlatform.h"


OSStatus AaThreadCreate(mico_thread_t* thread, uint8_t priority, const char* name, mico_thread_function_t function, uint32_t stack_size, void* arg);
OSStatus AaThreadDelete(mico_thread_t* thread);
u8 AaThreadGetCurrentId();
void AaThreadPrintListTbl(void);



   
#ifdef __cplusplus
}
#endif

#endif // _AATHREAD_H_

// end of file


