#include "hal_types.h"
#include "hal_uart.h"

uint8 gestureRemoteTaskId;

// Returns the length of a null terminated string
uint16 strlen(uint8* str)
{
    if (str == NULL) return 0; // Probably not necessary

    uint16 len = 0;
    while (str[len] != 0)
    {
        len++;
    }

    return len;
}

// Outputs a string to UART
void puts(uint8* str, uint8 length)
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
    //uartConfig.configured = TRUE;
    uartConfig.baudRate = HAL_UART_BR_57600;
    uartConfig.flowControl = HAL_UART_FLOW_OFF;
    uartConfig.flowControlThreshold = 0;
    uartConfig.rx.maxBufSize = 20;
    uartConfig.tx.maxBufSize = 128;
    uartConfig.idleTimeout = 0;
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
    //puts("Hello World!");
}

uint16 GestureRemote_ProcessEvent(uint8 task_id, uint16 events)
{
    return 0;
}