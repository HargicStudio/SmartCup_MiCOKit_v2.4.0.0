


/***

History:
2016-04-23: Ted: Create

*/

#include "AaThread.h"
#include "AaSysLog.h"


#ifdef DEBUG
  #define user_log(M, ...) custom_log("LightsMonitor", M, ##__VA_ARGS__)
  #define user_log_trace() custom_log_trace("LightsMonitor")
#else
#ifdef USER_DEBUG
  #define user_log(M, ...) user_debug("LightsMonitor", M, ##__VA_ARGS__)
  #define user_log_trace() (void)0
#endif
#endif


typedef struct SAaThread_t {
    struct SAaThread_t*     next;
    const char*             name;
    mico_thread_t*          thread;
    mico_mutex_t            mutex;
    uint8_t                 tid;
} SAaThread;


static SAaThread* _aa_thread_list = NULL;
static u16 _aa_thread_id = 1;



OSStatus AaThreadCreate(mico_thread_t* thread, 
                            uint8_t priority, 
                            const char* name, 
                            mico_thread_function_t function, 
                            uint32_t stack_size, 
                            void* arg)
{
    SAaThread* thread_ptr = (SAaThread*)malloc(sizeof(SAaThread));
    if(thread_ptr == NULL) {
        return kNoMemoryErr;
    }
    
    OSStatus err = mico_rtos_create_thread(thread, priority, name, function, stack_size, arg);
    if(err != kNoErr) {
        if(thread_ptr != NULL) free(thread_ptr);
        return kGeneralErr;
    }

    thread_ptr->thread = thread;
    thread_ptr->name = name;
    thread_ptr->tid = _aa_thread_id++;
    thread_ptr->next = NULL;

    SAaThread* find_ptr = _aa_thread_list;
    if(find_ptr == NULL) {
        _aa_thread_list = thread_ptr;
    }
    else {
        while(find_ptr->next != NULL) {
            find_ptr = find_ptr->next;
        }

        find_ptr->next = thread_ptr;
    }

    return kNoErr;
}

OSStatus AaThreadDelete(mico_thread_t* thread)
{
    OSStatus err = mico_rtos_delete_thread(thread);

    SAaThread* prev_ptr = NULL;
    SAaThread* find_ptr = _aa_thread_list;
    
    if(find_ptr == NULL) return kGeneralErr;
    
    if(*(find_ptr->thread) == *thread) {
        _aa_thread_list = find_ptr->next;
        free(find_ptr);
    }
    else {
        do {
            prev_ptr = find_ptr;
            find_ptr = find_ptr->next;
        } while(*(find_ptr->thread) != *thread && find_ptr != NULL);

        if(find_ptr == NULL) {
            return kGeneralErr;
        }

        if(find_ptr->next == NULL) {
            prev_ptr->next = NULL;
        }
        else {
            prev_ptr->next = find_ptr->next;
        }

        free(find_ptr);
    }

    return kNoErr;
}

u8 AaThreadGetCurrentId()
{
    SAaThread* find_ptr = _aa_thread_list;

    while(find_ptr != NULL) {
        if(mico_rtos_is_current_thread(find_ptr->thread)) {
            break;
        }
        else {
            find_ptr = find_ptr->next;
        }
    };

    if(find_ptr == NULL) {
        return 0;
    }

    return find_ptr->tid;
}

void AaThreadPrintListTbl(void)
{
    SAaThread* find_ptr = _aa_thread_list;

    AaSysLogPrint(LOGLEVEL_DBG, "_aa_thread_list table:");
    while(find_ptr != NULL) {
        AaSysLogPrint(LOGLEVEL_DBG, "ptr %p next %p name %s thread %p tid %d",
                find_ptr, find_ptr->next, find_ptr->name, find_ptr->thread, find_ptr->tid);

        find_ptr = find_ptr->next;
    }
}

// end of file


