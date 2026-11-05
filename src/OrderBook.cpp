#include "OrderBook.h"
#include <stdlib.h>
#include "Debug.h"


bool OrderBookInitialize(OrderBook* orderBook){
    if (!orderBook){return false;}
    orderBook->bidsCount = 0;
    orderBook->asksCount = 0;
    orderBook->bidsCapacity = ORDERBOOK_BIDS_START_CAPACITY;
    orderBook->asksCapacity = ORDERBOOK_ASKS_START_CAPACITY;
    orderBook->bids = (PriceLevel*)malloc(sizeof(PriceLevel) * orderBook->bidsCapacity);
    if (!orderBook->bids){return false;}
    orderBook->asks = (PriceLevel*)malloc(sizeof(PriceLevel) * orderBook->asksCapacity);
    if (!orderBook->asks){
        free(orderBook->bids);
        return false;
    }
    return true;
}

bool OrderBookResize(OrderBook* orderBook, uint8_t side){
    if (!orderBook || side > 1){return false;}
    // check if newCapacity is within valid range
    uint32_t newCapacity = 0;
    if (side == BUY){
        newCapacity = orderBook->bidsCapacity * 2;
        if (newCapacity > ORDERBOOK_BIDS_MAX_CAPACITY){return false;}
    }
    else{
        newCapacity = orderBook->asksCapacity * 2;
        if (newCapacity > ORDERBOOK_ASKS_MAX_CAPACITY){return false;}
    }
    // create new array of PriceLevels
    PriceLevel* newPriceLevels = (PriceLevel*)malloc(sizeof(PriceLevel) * newCapacity);
    if (!newPriceLevels){return false;}
    // copy elements from old PriceLevels
    if (side == BUY){
        for (uint32_t i = 0; i < orderBook->bidsCount; i++){
            newPriceLevels[i] = orderBook->bids[i];
        }
        free(orderBook->bids);
        orderBook->bids = newPriceLevels;
        orderBook->bidsCapacity = newCapacity;
        return true;
    }
    if (side == SELL){
        for (uint32_t i = 0; i < orderBook->asksCount; i++){
            newPriceLevels[i] = orderBook->asks[i];
        }
        free(orderBook->asks);
        orderBook->asks = newPriceLevels;
        orderBook->asksCapacity = newCapacity;
        return true;
    }
    free(newPriceLevels);
    return false;
}


// int32_t OrderBookFindBidPriceLevelInsertPosition(const OrderBook* orderBook, uint32_t price){
//     if (!orderBook || price <= 0){return -1;}
//     for (uint32_t i = 0; i < orderBook->bidsCount; i++){
//         if (price > orderBook->bids[i].price){return i;}
//     }
//     return orderBook->bidsCount;
// }

int32_t OrderBookFindBidPriceLevelInsertPosition(const OrderBook* orderBook, uint32_t price){
    if (!orderBook || price == 0){return -1;}
    int32_t left = 0;
    int32_t right = (int32_t)orderBook->bidsCount - 1;
    while (left <= right){
        int32_t mid = (left + right) / 2;
        uint32_t currentPrice = orderBook->bids[mid].price;
        if (currentPrice == price){return mid;}
        if (currentPrice > price){
            left = mid + 1;
        }
        else{
            right = mid - 1;
        }
    }
    // priceLevel not found, left now points to correct index
    return left;
}

// int32_t OrderBookFindAskPriceLevelInsertPosition(const OrderBook* orderBook, uint32_t price){
//     if (!orderBook || price == 0){return -1;}
//     for (uint32_t i = 0; i < orderBook->asksCount; i++){
//         if (price < orderBook->asks[i].price){return i;}
//     }
//     return orderBook->asksCount;
// }

int32_t OrderBookFindAskPriceLevelInsertPosition(const OrderBook* orderBook, uint32_t price){
    if (!orderBook || price == 0){return -1;}
    int32_t left = 0;
    int32_t right = (int32_t)orderBook->asksCount - 1;
    while (left <= right){
        int32_t mid = (left + right) / 2;
        uint32_t currentPrice = orderBook->asks[mid].price;
        if (currentPrice == price){return mid;}
        if (currentPrice < price){
            left = mid + 1;
        }
        else{
            right = mid - 1;
        }
    }
    // priceLevel not found, left now points to correct index
    return left;
}

// int32_t OrderBookFindBidPriceLevel(const OrderBook* orderBook, uint32_t price){
//     if (!orderBook){return -1;}
//     // traverse all bids to find if priceLevel exists
//     for (uint32_t i = 0; i < orderBook->bidsCount; i++){
//         if (orderBook->bids[i].price == price){return i;}
//     }
//     return -1;
// }

int32_t OrderBookFindBidPriceLevel(const OrderBook* orderBook, uint32_t price){
    if (!orderBook){return -1;}
    int32_t left = 0;
    int32_t right = (int32_t)orderBook->bidsCount - 1;
    while (left <= right){
        int32_t mid = (left + right) / 2;
        uint32_t currentPrice = orderBook->bids[mid].price;
        if (currentPrice == price){return mid;}
        if (currentPrice > price){
            left = mid + 1;
        }
        else{right = mid - 1;}
    }
    return -1;
}

// int32_t OrderBookFindAskPriceLevel(const OrderBook* orderBook, uint32_t price){
//     if (!orderBook || price == 0){return -1;}
//     for (uint32_t i = 0; i < orderBook->asksCount; i++){
//         if (orderBook->asks[i].price == price){return i;}
//     }
//     return -1;
// }

int32_t OrderBookFindAskPriceLevel(const OrderBook* orderBook, uint32_t price){
    if (!orderBook){return -1;}
    int32_t left = 0;
    int32_t right = (int32_t)orderBook->asksCount - 1;
    while (left <= right){
        int32_t mid = (left + right) / 2;
        uint32_t currentPrice = orderBook->asks[mid].price;
        if (currentPrice == price){return mid;}
        if (currentPrice < price){
            left = mid + 1;
        }
        else{right = mid - 1;}
    }
    return -1;
}

bool OrderBookInsertBidPriceLevel(OrderBook* orderBook, uint32_t price){
    if (!orderBook || price == 0){return false;}
    if (OrderBookFindBidPriceLevel(orderBook, price) != -1){return false;}
    if (orderBook->bidsCount >= orderBook->bidsCapacity){
        if (!OrderBookResize(orderBook, BUY)){return false;}
    }
    int32_t insertIndex = OrderBookFindBidPriceLevelInsertPosition(orderBook, price);
    if (insertIndex == -1){return false;}
    // shift all elements from insertIndex to bidsCount to right by 1
    for (int32_t i = orderBook->bidsCount - 1; i >= insertIndex; i--){
        orderBook->bids[i + 1] = orderBook->bids[i];
    }
    if (!PriceLevelInitialize(&orderBook->bids[insertIndex], price)){return false;}
    orderBook->bidsCount++;
    return true;
}

bool OrderBookInsertAskPriceLevel(OrderBook* orderBook, uint32_t price){
    if (!orderBook || price == 0){return false;}
    if (OrderBookFindAskPriceLevel(orderBook, price) != -1){return false;}
    if (orderBook->asksCount >= orderBook->asksCapacity){
        if (!OrderBookResize(orderBook, SELL)){return false;}
    }
    int32_t insertIndex = OrderBookFindAskPriceLevelInsertPosition(orderBook, price);
    if (insertIndex == -1){return false;}
    // shift all elements from insertIndex to asksCount to right by 1
    for (int32_t i = orderBook->asksCount - 1; i >= insertIndex; i--){
        orderBook->asks[i + 1] = orderBook->asks[i];
    }
    if (!PriceLevelInitialize(&orderBook->asks[insertIndex], price)){return false;}
    orderBook->asksCount++;
    return true;
}

bool OrderBookAddBuyOrder(OrderBook* orderBook, Order order){
    if (!orderBook){return false;}
    int32_t priceLevelIndex = OrderBookFindBidPriceLevel(orderBook, order.price);
    if (priceLevelIndex == -1){
        // PriceLevel doesn't exist so create one
        if (!OrderBookInsertBidPriceLevel(orderBook, order.price)){return false;}
        // update priceLevelIndex
        priceLevelIndex = OrderBookFindBidPriceLevel(orderBook, order.price);
    }
    // add order to priceLevel's FIFOQueue
    return PriceLevelAddOrder(&orderBook->bids[priceLevelIndex], order);
}

bool OrderBookAddSellOrder(OrderBook* orderBook, Order order){
    if (!orderBook){return false;}
    int32_t priceLevelIndex = OrderBookFindAskPriceLevel(orderBook, order.price);
    if (priceLevelIndex == -1){
        // PriceLevel doesn't exist so create one
        if (!OrderBookInsertAskPriceLevel(orderBook, order.price)){return false;}
        // update priceLevelIndex
        priceLevelIndex = OrderBookFindAskPriceLevel(orderBook, order.price);
    }
    // add order to priceLevel's FIFOQueue
    return PriceLevelAddOrder(&orderBook->asks[priceLevelIndex], order);
}

bool OrderBookAddOrder(OrderBook* orderBook, Order order){
    if (!orderBook){return false;}
    if (order.side == BUY){return OrderBookAddBuyOrder(orderBook, order);}
    if (order.side == SELL){return OrderBookAddSellOrder(orderBook, order);}
    return false;
}
        
bool OrderBookRemovePriceLevel(PriceLevel* priceLevels, uint32_t* count, uint32_t index){
    if (!priceLevels || !count){return false;}
    if (index >= *count){return false;}
    // destroy priceLevel
    if (!PriceLevelDestroy(&priceLevels[index])){return false;}
    // shift all elements from index to count left
    for (uint32_t i = index; i < (*count) - 1; i++){
        priceLevels[i] = priceLevels[i + 1];
    }
    (*count)--;
    return true;
}

PriceLevel* OrderBookBestBid(OrderBook* orderBook){
    if (!orderBook){return nullptr;}
    if (orderBook->bidsCount == 0){return nullptr;}
    return &orderBook->bids[0];
}

PriceLevel* OrderBookBestAsk(OrderBook* orderBook){
    if (!orderBook){return nullptr;}
    if (orderBook->asksCount == 0){return nullptr;}
    return &orderBook->asks[0];
}

void OrderBookPrint(const OrderBook* orderBook){
    if (!orderBook){ENGINE_LOG("OrderBook EMPTY\n"); return;}
    ENGINE_LOG("------ASKS------\n\n");
    if (orderBook->asksCount == 0){ENGINE_LOG("None\n");}
    for (uint32_t i = 0; i < orderBook->asksCount; i++){
        PriceLevel* currentPriceLevel = &(orderBook->asks[i]);
        PriceLevelPrint(currentPriceLevel);
    }
    ENGINE_LOG("------BIDS------\n\n");
    if (orderBook->bidsCount == 0){ENGINE_LOG("None\n");}
    for (uint32_t i = 0; i < orderBook->bidsCount; i++){
        PriceLevel* currentPriceLevel = &(orderBook->bids[i]);
        PriceLevelPrint(currentPriceLevel);
    }
}


bool OrderBookDestroy(OrderBook* orderBook){
    if (!orderBook){return false;}
    // traverse bids and asks to delete priceLevels
    for (uint32_t i = 0; i < orderBook->bidsCount; i++){
        PriceLevelDestroy(&orderBook->bids[i]);
    }
    for (uint32_t i = 0; i < orderBook->asksCount; i++){
        PriceLevelDestroy(&orderBook->asks[i]);
    }
    free(orderBook->bids);
    free(orderBook->asks);
    orderBook->bids = nullptr;
    orderBook->asks = nullptr;
    orderBook->bidsCount = 0;
    orderBook->asksCount = 0;
    orderBook->bidsCapacity = 0;
    orderBook->asksCapacity = 0;
    return true;
}