#ifndef FIFOQUEUE_H
#define FIFOQUEUE_H

#include <stdint.h>
#include "Order.h"


#define FIFOQUEUE_START_CAPACITY 128
#define FIFOQUEUE_MAX_CAPACITY 1048576


/*
FIFOQueue implemented as contiguous ring buffer.
Avoids pointer chasing and improves cache locality compared to linked lists
*/

struct FIFOQueue{
    Order* orders;
    uint32_t head; // head and tail start at 0, when adding increment tail then add, when removing inrement head and remove
    uint32_t tail; // empty if head == tail, full if (tail + 1)%capacity = head
    uint32_t count;
    uint32_t capacity;
};

bool FIFOQueueInitialize(FIFOQueue* FIFOQueue);

Order* FIFOQueueRemoveFrontOrder(FIFOQueue* FIFOQueue);

Order* FIFOQueueGetFrontOrder(FIFOQueue* FIFOQueue);

bool FIFOQueueAddOrder(FIFOQueue* FIFOQueue, Order order);

bool FIFOQueueResize(FIFOQueue* FIFOQueue);

Order* FIFOQueuePeekOrder(const FIFOQueue* FIFOQueue, uint32_t relativeIndex); // 0 relativeIndex is first item even though it could technically be anywhere in the actual array

bool FIFOQueueDestroy(FIFOQueue* FIFOQueue);

#endif