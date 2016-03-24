
/***

History:
2015-09-30: Ted: Create

*/


#include "user_debug.h"


mico_mutex_t user_debug_mutex;


void UserDebug_Init()
{
    mico_rtos_init_mutex(&user_debug_mutex);
    mico_rtos_unlock_mutex(&user_debug_mutex);
}


// end of file