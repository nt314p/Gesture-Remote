#ifndef GESTURE_REMOTE_H
#define GESTURE_REMOTE_H

#include "hal_types.h"

void GestureRemote_Init(uint8 task_id);
uint16 GestureRemote_ProcessEvent(uint8 task_id, uint16 events);

#endif