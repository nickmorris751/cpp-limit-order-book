#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include "PriceLevel.h"

#define ORDERBOOK_BIDS_START_CAPACITY 1024
#define ORDERBOOK_ASKS_START_CAPACITY 1024
#define ORDERBOOK_BIDS_MAX_CAPACITY 1048576
#define ORDERBOOK_ASKS_MAX_CAPACITY 1048576

struct OrderBook{
    PriceLevel* bids;
    PriceLevel* asks;
    uint32_t bidsCount;
    uint32_t asksCount;
    uint32_t bidsCapacity;
    uint32_t asksCapacity;
};

bool OrderBookInitialize(OrderBook* orderBook);
bool OrderBookResize(OrderBook* orderBook, uint8_t side);
int32_t OrderBookFindBidPriceLevelInsertPosition(const OrderBook* orderBook, uint32_t price);
int32_t OrderBookFindAskPriceLevelInsertPosition(const OrderBook* orderBook, uint32_t price);
int32_t OrderBookFindBidPriceLevel(const OrderBook* orderBook, uint32_t price);
int32_t OrderBookFindAskPriceLevel(const OrderBook* orderBook, uint32_t price);
bool OrderBookInsertBidPriceLevel(OrderBook* orderBook, uint32_t price);
bool OrderBookInsertAskPriceLevel(OrderBook* orderBook, uint32_t price);
bool OrderBookAddBuyOrder(OrderBook* orderBook, Order order);
bool OrderBookAddSellOrder(OrderBook* orderBook, Order order);
bool OrderBookAddOrder(OrderBook* orderBook, Order order);
bool OrderBookRemovePriceLevel(PriceLevel* priceLevels, uint32_t* count, uint32_t index);
PriceLevel* OrderBookBestBid(OrderBook* orderBook);
PriceLevel* OrderBookBestAsk(OrderBook* orderBook);
void OrderBookPrint(const OrderBook* orderBook);
bool OrderBookDestroy(OrderBook* orderBook);

#endif