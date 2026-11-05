#include "FIFOQueue.h"
#include <stdlib.h>


bool FIFOQueueInitialize(FIFOQueue* FIFOQueue){
    if (!FIFOQueue){return false;}
    FIFOQueue->head = 0;
    FIFOQueue->tail = 0;
    FIFOQueue->capacity = FIFOQUEUE_START_CAPACITY;
    FIFOQueue->count = 0;
    FIFOQueue->orders = (Order*)malloc(sizeof(Order) * FIFOQueue->capacity);
    if (!FIFOQueue->orders){return false;}
    return true;
}

Order* FIFOQueueRemoveFrontOrder(FIFOQueue* FIFOQueue){
    if (!FIFOQueue){return nullptr;}
    if (FIFOQueue->head == FIFOQueue->tail){return nullptr;}
    // FIFOQueue->head = (FIFOQueue->head + 1) % FIFOQueue->capacity;
    FIFOQueue->head = (FIFOQueue->head + 1) & (FIFOQueue->capacity - 1);
    FIFOQueue->count--;
    return &(FIFOQueue->orders[FIFOQueue->head]);
}

Order* FIFOQueueGetFrontOrder(FIFOQueue* FIFOQueue){
    if (!FIFOQueue){return nullptr;}
    if (FIFOQueue->head == FIFOQueue->tail){return nullptr;}
    uint32_t headIndex = (FIFOQueue->head + 1) & (FIFOQueue->capacity - 1);
    return &(FIFOQueue->orders[headIndex]);
}

bool FIFOQueueResize(FIFOQueue* FIFOQueue){
    if (!FIFOQueue){return false;}
    // calculate new capacity and ensure it's valid
    uint32_t newCapacity = FIFOQueue->capacity * 2;
    if (newCapacity > FIFOQUEUE_MAX_CAPACITY){return false;}
    Order* newOrders = (Order*)malloc(sizeof(Order) * newCapacity);
    if (!newOrders){return false;}
    // copy old orders into newOrders
    uint32_t count = FIFOQueue->count;
    uint32_t currentHeadIndex = FIFOQueue->head;
    for (uint32_t i = 1; i <= count; i++){
        currentHeadIndex = (currentHeadIndex + 1) & (FIFOQueue->capacity - 1);
        newOrders[i] = FIFOQueue->orders[currentHeadIndex];
    }
    FIFOQueue->tail = count; // since first item is now placed at index 1 and last is at index count (head and tail start at index 0, tail incremented then element inserted so at least one element in array is always empty to preserve condition head == tail means array is empty)
    FIFOQueue->head = 0;
    FIFOQueue->count = count;
    free(FIFOQueue->orders);
    FIFOQueue->capacity = newCapacity;
    FIFOQueue->orders = newOrders;
    return true;
}

bool FIFOQueueAddOrder(FIFOQueue* FIFOQueue, Order order){
    if (!FIFOQueue){return false;}
    if (FIFOQueue->count + 1 >= FIFOQueue->capacity){
        if (!FIFOQueueResize(FIFOQueue)){return false;}
    }
    uint32_t tailIndex = (FIFOQueue->tail + 1) & (FIFOQueue->capacity - 1);
    if (tailIndex == FIFOQueue->head){return false;}
    FIFOQueue->tail = tailIndex;
    FIFOQueue->orders[FIFOQueue->tail] = order;
    FIFOQueue->count++;
    return true;
}

Order* FIFOQueuePeekOrder(const FIFOQueue* FIFOQueue, uint32_t relativeIndex){
    if (!FIFOQueue || relativeIndex >= FIFOQueue->count){return nullptr;}
    uint32_t current = FIFOQueue->head;
    for (uint32_t i = 0; i <= relativeIndex; i++){
        current = (current + 1) & (FIFOQueue->capacity - 1);
    }
    return &(FIFOQueue->orders[current]);
}

bool FIFOQueueDestroy(FIFOQueue* FIFOQueue){
    if (!FIFOQueue){return true;}
    if (FIFOQueue->orders){free(FIFOQueue->orders);}
    FIFOQueue->orders = NULL;
    FIFOQueue->capacity = 0;
    FIFOQueue->head = 0;
    FIFOQueue->tail = 0;
    FIFOQueue->count = 0;
    return true;
}