#include "PriceLevel.h"
#include "Debug.h"

bool PriceLevelInitialize(PriceLevel* priceLevel, uint32_t price){
    if (!priceLevel){return false;}
    if (!FIFOQueueInitialize(&priceLevel->orders)){return false;}
    priceLevel->price = price;
    priceLevel->totalQuantity = 0;
    return true;
}

bool PriceLevelAddOrder(PriceLevel* priceLevel, Order order){
    if (!priceLevel){return false;}
    if (!FIFOQueueAddOrder(&priceLevel->orders, order)){return false;}
    priceLevel->totalQuantity += order.quantity;
    return true;
}

Order* PriceLevelRemoveFrontOrder(PriceLevel* priceLevel){
    if (!priceLevel){return nullptr;}
    Order* order = FIFOQueueRemoveFrontOrder(&priceLevel->orders);
    priceLevel->totalQuantity -= order->quantity;
    return order;
}

Order* PriceLevelGetFrontOrder(PriceLevel* priceLevel){
    if (!priceLevel){return nullptr;}
    Order* order = FIFOQueueGetFrontOrder(&priceLevel->orders);
    return order;
}

bool PriceLevelPartialFillFrontOrder(PriceLevel* priceLevel, uint32_t quantity){
    if (!priceLevel || quantity == 0){return false;}
    Order* order = FIFOQueueGetFrontOrder(&priceLevel->orders);
    if (!order){return false;}
    if (quantity >= order->quantity){return false;}
    order->quantity -= quantity;
    priceLevel->totalQuantity -= quantity;
    return true;
}

bool PriceLevelEmpty(const PriceLevel* priceLevel){
    if (!priceLevel){return true;}
    return priceLevel->totalQuantity == 0;
}

void PriceLevelPrint(const PriceLevel* priceLevel){
    if (!priceLevel){return;}
    ENGINE_LOG("PRICELEVEL: %llu TotalQuantity: %llu\n\n", priceLevel->price, priceLevel->totalQuantity);
    Order* currentOrder = nullptr;
    uint32_t currentIndex = 0;
    while (1){
        currentOrder = FIFOQueuePeekOrder(&priceLevel->orders, currentIndex);
        if (!currentOrder){break;}
        ENGINE_LOG("Price: %llu Shares: %llu\n", currentOrder->price, currentOrder->quantity);
        currentIndex++;
    }
    return;
}

bool PriceLevelDestroy(PriceLevel* priceLevel){
    if (!priceLevel){return true;}
    FIFOQueueDestroy(&priceLevel->orders);
    priceLevel->price = 0;
    priceLevel->totalQuantity = 0;
    return true;
}