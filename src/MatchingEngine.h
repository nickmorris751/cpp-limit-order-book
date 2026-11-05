#ifndef MATCHINGENGINE_H
#define MATCHINGENGINE_H

#include "OrderBook.h"

bool MatchingEngineProcessBuyOrder(OrderBook* orderBook, Order* order);
bool MatchingEngineProcessSellOrder(OrderBook* orderBook, Order* order);
bool MatchingEngineProcessOrder(OrderBook* orderBook, Order* order);

#endif