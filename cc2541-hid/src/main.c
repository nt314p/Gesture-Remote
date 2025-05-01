#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_Memory.h"
#include "osal_snv.h"
#include "OnBoard.h"
#include "hal_board.h"
#include "hal_drivers.h"
#include "hal_types.h"

/* HAL */
#include "hal_drivers.h"

/* LL */
#include "ll.h"

/* HCI */
#include "hci_tl.h"

/* L2CAP */
#include "l2cap.h"

/* gap */
#include "gap.h"
#include "gapgattserver.h"
#include "gapbondmgr.h"

/* GATT */
#include "gatt.h"
#include "gattservapp.h"

/* Profiles */
#include "peripheral.h"
#include "hiddev.h"

/* Application */
#include "gestureRemote.h"

// The order in this table must be identical to the task initialization
// calls below in osalInitTask. This variable must be named `tasksArr`
// as it is used internally by OSAL.
const pTaskEventHandlerFn tasksArr[] =
{
    LL_ProcessEvent,
    Hal_ProcessEvent,
    HCI_ProcessEvent,
    L2CAP_ProcessEvent,
    GAP_ProcessEvent,
    GATT_ProcessEvent,
    SM_ProcessEvent,
    GAPRole_ProcessEvent,
    GAPBondMgr_ProcessEvent,
    GATTServApp_ProcessEvent,
    HidDev_ProcessEvent,
    GestureRemote_ProcessEvent
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

    LL_Init(taskID++);

    Hal_Init(taskID++);

    HCI_Init(taskID++);

    L2CAP_Init(taskID++);

    GAP_Init(taskID++);
    GATT_Init(taskID++);
    SM_Init(taskID++);

    GAPRole_Init(taskID++);
    GAPBondMgr_Init(taskID++);

    GATTServApp_Init(taskID++);

    HidDev_Init(taskID++);

    GestureRemote_Init(taskID);
}

int main()
{
    HAL_BOARD_INIT();

    InitBoard(OB_COLD);

    HalDriverInit();
    osal_snv_init();

    osal_init_system();

    HAL_ENABLE_INTERRUPTS();

    InitBoard(OB_READY);

#if defined POWER_SAVING
    osal_pwrmgr_device(PWRMGR_BATTERY);
#endif

    osal_start_system();

    return 0;
}