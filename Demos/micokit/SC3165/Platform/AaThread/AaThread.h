


/***

History:
2016-04-23: Ted: Create

*/

#ifndef _AATHREAD_H_
#define _AATHREAD_H_

#ifdef __cplusplus
 extern "C" {
#endif 


typedef struct SAaThread_t {
    struct SAaThread_t*     next;
    // mico used
    char*                   name;
    mico_thread_t           thread;    
    mico_thread_function_t  function;
    void*                   arg;
    uint32_t                stack_size;
    uint8_t                 priority;
    //
    uint8_t                 t_id;
} SAaThread;



   
#ifdef __cplusplus
}
#endif

#endif // _AATHREAD_H_

// end of file


