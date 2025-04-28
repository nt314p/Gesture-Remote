#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_Memory.h"

// The order in this table must be identical to the task initialization
// calls below in osalInitTask. This variable must be named `tasksArr`
// as it is used internally by OSAL.
const pTaskEventHandlerFn tasksArr[] =
{
    // Set task handler function pointers...
};

// The number of tasks configured. This variable must be named
// `tasksCnt` as it is used internally by OSAl;
const uint8 tasksCnt = sizeof(tasksArr) / sizeof(tasksArr[0]);

// An array of task events. The indices correspond to tasks configured
// in `tasksArr`. This variable must be named `tasksEvents` as it is
// used internally by OSAL.
uint16* tasksEvents;

void osalInitTasks()
{
    uint8 taskID = 0;

    tasksEvents = osal_mem_alloc(sizeof(uint16) * tasksCnt);
    osal_memset(tasksEvents, 0, sizeof(uint16) * tasksCnt);

    // Initialize tasks...
}

int main()
{
    osal_init_system();
    osal_start_system();
}