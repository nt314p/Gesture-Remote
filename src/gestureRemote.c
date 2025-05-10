#include "gestureRemote.h"
#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_Timers.h"
#include "OnBoard.h"
#include "gap.h"
#include "peripheral.h"
#include "gatt.h"
#include "gatt_profile_uuid.h"
#include "gapgattserver.h"
#include "gattservapp.h"
#include "gapbondmgr.h"
#include "battservice.h"
#include "hiddev.h"
#include "hidkbmservice.h"
#include "hal_types.h"
#include "hal_uart.h"

#define DEVICE_INIT_EVENT 1 << 0
#define PERIODIC_EVENT 1 << 1

#define DEVICE_NAME "Gesture Remote"
#define HID_IDLE_TIMEOUT_MS 60000

static uint8 deviceName[GAP_DEVICE_NAME_LEN] = DEVICE_NAME;

uint8 gestureRemoteTaskId;

// https://jimmywongiot.com/2019/08/13/advertising-payload-format-on-ble/
// https://www.ti.com/lit/ug/swru271i/swru271i.pdf
// https://argenox.com/library/bluetooth-low-energy/ble-advertising-primer

// Central devices can request more information from an advertising device
// by issuing a scan request. The advertising device responds with a scan
// response. In this case, we respond with the device name.
static uint8 scanResponse[] =
{
  15, // length of string (not including terminator) + 1
  GAP_ADTYPE_LOCAL_NAME_COMPLETE,   // AD Type = Complete local name
  'G', 'e', 's', 't', 'u', 'r', 'e', ' ', 'R', 'e', 'm', 'o', 't', 'e'
};

// Advertising devices send out advertising packets to Central devices.
// This packet contains flags, appearance, and service UUID data.
// TODO: move some of this data to scan response?
static uint8 advertisingData[] =
{
    // flags
    0x02,   // length of this data
    GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_LIMITED | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

    // appearance
    0x03,   // length of this data
    GAP_ADTYPE_APPEARANCE,
    LO_UINT16(GAP_APPEARE_HID_KEYBOARD),
    HI_UINT16(GAP_APPEARE_HID_MOUSE),

    // service UUIDs
    0x05,   // length of this data
    GAP_ADTYPE_16BIT_MORE,
    LO_UINT16(HID_SERV_UUID),
    HI_UINT16(HID_SERV_UUID),
    LO_UINT16(BATT_SERV_UUID),
    HI_UINT16(BATT_SERV_UUID)
};

// HID Dev configuration
static hidDevCfg_t hidDevCfg =
{
    HID_IDLE_TIMEOUT_MS,
    HID_KBD_FLAGS
};

static uint8 hidKbdMouseRptCB(uint8 id, uint8 type, uint16 uuid,
    uint8 oper, uint8* pLen, uint8* pData);
static void hidKbdMouseEvtCB(uint8 evt);

static hidDevCB_t hidDevCallbacks =
{
    NULL,//hidKbdMouseRptCB,
    NULL,//hidKbdMouseEvtCB,
    NULL
};

// // Returns the length of a null terminated string
// uint16 strlen(uint8* str)
// {
//     if (str == NULL) return 0; // Probably not necessary

//     uint16 len = 0;
//     while (str[len] != 0)
//     {
//         len++;
//     }

//     return len;
// }

// Outputs a string to UART
void puts(uint8* str, uint16 length)
{
#if (HAL_UART_ISR == 1)
    HalUARTWrite(HAL_UART_PORT_0, str, length);
#else
    HalUARTWrite(HAL_UART_PORT_1, str, length);
#endif
}

void UartCallback(uint8 port, uint8 event)
{
    uint8 buffer[64];
    uint8 readLength = 0;

    switch (event)
    {
    case HAL_UART_RX_FULL:
    case HAL_UART_RX_ABOUT_FULL:
    case HAL_UART_RX_TIMEOUT:
    {
        readLength = Hal_UART_RxBufLen(port);
        HalUARTRead(port, buffer, readLength);
        HalUARTWrite(port, buffer, readLength);
        
        // uint8 newline = '\n';
        // uint8 str[] = "Beep?";
        // uint8 str2[] = "Keep?";

        // HalUARTWrite(port, str, 5);
        // HalUARTWrite(port, &newline, 1);
        // HalUARTWrite(port, "Meep?", 5);
        // HalUARTWrite(port, &newline, 1);
        // HalUARTWrite(port, (uint8*) str2, 5);
        // HalUARTWrite(port, &newline, 1);
        
        break;
    }
    case HAL_UART_TX_FULL:
    {
        break;
    }
    case HAL_UART_TX_EMPTY:
    {
        break;
    }
    }

    //HalUARTWrite(port, buffer, readLength);
    // Do something with buffer
}

void UartInit()
{
    halUARTCfg_t uartConfig; // TODO: does this need to be global?
    uartConfig.configured = TRUE;
    uartConfig.baudRate = HAL_UART_BR_57600;
    uartConfig.flowControl = HAL_UART_FLOW_OFF;
    uartConfig.flowControlThreshold = 0;
    uartConfig.rx.maxBufSize = 20;
    uartConfig.tx.maxBufSize = 128;
    uartConfig.idleTimeout = 25; // Timeout in ms?? to callback with the idle timeout event
    uartConfig.intEnable = TRUE;
    uartConfig.callBackFunc = (halUARTCBack_t)UartCallback;

#if (HAL_UART_ISR == 1)
    HalUARTOpen(HAL_UART_PORT_0, &uartConfig);
#else
    HalUARTOpen(HAL_UART_PORT_1, &uartConfig);
#endif
}

void GestureRemote_Init(uint8 task_id)
{
    HalUARTInit();
    UartInit();

    gestureRemoteTaskId = task_id;

    uint8 message[] = {'P', 'e', 'e', 'p', '\n'};
    HalUARTWrite(HAL_UART_PORT_1, message, 5);

    /*
    // Setup GAP ()
    GAP_SetParamValue(TGAP_CONN_PAUSE_PERIPHERAL, 6); // Pause 6 seconds (?)

    // Setup GAP role peripheral broadcaster
    uint8 enableAdvertising = TRUE;
    uint16 advertOffTime = 0;

    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &enableAdvertising);
    GAPRole_SetParameter(GAPROLE_ADVERT_OFF_TIME, sizeof(uint16), &advertOffTime);

    GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advertisingData), advertisingData);
    GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scanResponse), scanResponse);

    uint8 enableUpdateRequest = TRUE;

    // The range of the connection interval
    uint16 minConnInterval = 8; // 10 ms
    uint16 maxConnInterval = 8; // 10 ms
    uint16 peripheralLatency = 50; // The number of connection events we can skip
    uint16 connTimeoutMultiplier = 500; // Connection timeout in units of 10 ms

    GAPRole_SetParameter(GAPROLE_PARAM_UPDATE_ENABLE, sizeof(uint8), &enableUpdateRequest);
    GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(uint16), &minConnInterval);
    GAPRole_SetParameter(GAPROLE_MAX_CONN_INTERVAL, sizeof(uint16), &maxConnInterval);
    GAPRole_SetParameter(GAPROLE_SLAVE_LATENCY, sizeof(uint16), &peripheralLatency);
    GAPRole_SetParameter(GAPROLE_TIMEOUT_MULTIPLIER, sizeof(uint16), &connTimeoutMultiplier);

    // Set GATT server name attribute
    GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, deviceName);

    uint16 advInt = 160; // In units of 0.625 ms, 160 = 100 ms

    GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MIN, advInt);
    GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MAX, advInt);
    GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MIN, advInt);
    GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MAX, advInt);

    // Setup the GAP Bond Manager
    uint32 passkey = 802048;
    uint8 pairMode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;
    uint8 mitmProtection = TRUE;
    uint8 ioCapabilities = GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8 enableBonding = TRUE;
    GAPBondMgr_SetParameter(GAPBOND_DEFAULT_PASSCODE, sizeof(uint32), &passkey);
    GAPBondMgr_SetParameter(GAPBOND_PAIRING_MODE, sizeof(uint8), &pairMode);
    GAPBondMgr_SetParameter(GAPBOND_MITM_PROTECTION, sizeof(uint8), &mitmProtection);
    GAPBondMgr_SetParameter(GAPBOND_IO_CAPABILITIES, sizeof(uint8), &ioCapabilities);
    GAPBondMgr_SetParameter(GAPBOND_BONDING_ENABLED, sizeof(uint8), &enableBonding);

    uint8 critical = 10; // Critical level at 10%
    Batt_SetParameter(BATT_PARAM_CRITICAL_LEVEL, sizeof(uint8), &critical);

    GGS_AddService(GATT_ALL_SERVICES);
    GATTServApp_AddService(GATT_ALL_SERVICES);

    HidKbM_AddService();
    HidDev_Register(&hidDevCfg, &hidDevCallbacks);

    RegisterForKeys(gestureRemoteTaskId); // TODO: is this necessary?
    */

    osal_set_event(gestureRemoteTaskId, DEVICE_INIT_EVENT);
}

uint16 GestureRemote_ProcessEvent(uint8 task_id, uint16 events)
{
    if (events & SYS_EVENT_MSG)
    {
        // We don't expect any messages

        return (events ^ SYS_EVENT_MSG);
    }

    if (events & DEVICE_INIT_EVENT)
    {

        // Called when device starts
       // puts("Device initialized\n", 19);

        osal_start_reload_timer(gestureRemoteTaskId, PERIODIC_EVENT, 100);

        return (events ^ DEVICE_INIT_EVENT);
    }

    if (events & PERIODIC_EVENT)
    {
        uint8 message[] = "Beep\n";
        HalUARTWrite(HAL_UART_PORT_1, message, 5);

        return (events ^ PERIODIC_EVENT);
    }

    return 0;
}