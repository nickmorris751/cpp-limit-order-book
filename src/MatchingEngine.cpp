#include "MatchingEngine.h"
#include "Debug.h"


bool MatchingEngineProcessBuyOrder(OrderBook* orderBook, Order* order){
    if (!orderBook || !order){return false;}
    if (order->quantity == 0){return false;}
    // check if trade is possible
    while (1){
        PriceLevel* bestAskPriceLevel = OrderBookBestAsk(orderBook);
        if (!bestAskPriceLevel){break;}
        Order* bestAskOrder = PriceLevelGetFrontOrder(bestAskPriceLevel);
        if (bestAskOrder){
            // check if trade is possible
            if (bestAskOrder->price > order->price){break;}
            uint32_t sellShares = bestAskOrder->quantity;
            uint32_t remainingShares = 0;
            /*
            Case 1: sell shares > buy shares Result: adjust number of sell shares (trade complete)
            Case 2: sell shares == buy shares Result: remove sell order (trade complete)
            Case 3: sell shares < buy shares Result: remove sell order and move onto next sell order if exists (trade not completed)
            */
            if (sellShares > order->quantity){
                remainingShares = sellShares - order->quantity;
                // update sellShares and PriceLevel totalQuantity (since partial fill)
                PriceLevelPartialFillFrontOrder(bestAskPriceLevel, order->quantity); 
                ENGINE_LOG("**TRADE** BuyId: %llu -- SellId: %llu\nShares Bought: %u Sell Shares Remaining: %u\n", order->id, bestAskOrder->id, order->quantity, bestAskOrder->quantity);
                order->quantity = 0;
                ENGINE_LOG("TRADE COMPLETE\n");
                return true;
            }
            if (sellShares == order->quantity){
                ENGINE_LOG("**TRADE** BuyId: %llu -- SellId: %llu\nShares Bought: %u Sell Shares Remaining: 0\n", order->id, bestAskOrder->id, order->quantity);
                order->quantity = 0;
                PriceLevelRemoveFrontOrder(bestAskPriceLevel);
                if (PriceLevelEmpty(bestAskPriceLevel)){
                    OrderBookRemovePriceLevel(orderBook->asks, &orderBook->asksCount, 0);
                }
                ENGINE_LOG("TRADE COMPLETE\n");
                return true;
            }
            else{
                remainingShares = order->quantity - sellShares;
                uint32_t sharesBought = order->quantity - remainingShares;
                ENGINE_LOG("**TRADE** BuyId: %llu -- SellId: %llu\nShares Bought: %u Sell Shares Remaining: 0\n", order->id, bestAskOrder->id, sharesBought);
                order->quantity = remainingShares;
                PriceLevelRemoveFrontOrder(bestAskPriceLevel);
                if (PriceLevelEmpty(bestAskPriceLevel)){
                    OrderBookRemovePriceLevel(orderBook->asks, &orderBook->asksCount, 0);
                }
                ENGINE_LOG("TRADE PARTIALLY COMPLETE\n");
                continue;
            }
        }
        else{
            break;
        }
    }
    // add buy order to appropriate priceLevel in orderBook if all shares weren't traded
    if (!OrderBookAddBuyOrder(orderBook, *order)){return false;}
    return true;
}

bool MatchingEngineProcessSellOrder(OrderBook* orderBook, Order* order){
    if (!orderBook || !order){return false;}
    if (order->quantity == 0){return false;}
    // check if trade is possible
    while (1){
        PriceLevel* bestBidPriceLevel = OrderBookBestBid(orderBook);
        if (!bestBidPriceLevel){break;}
        Order* bestBidOrder = PriceLevelGetFrontOrder(bestBidPriceLevel);
        if (bestBidOrder){
            // check if trade is possible
            if (bestBidOrder->price < order->price){break;}
            uint32_t buyShares = bestBidOrder->quantity;
            uint32_t remainingShares = 0;
            /*
            Case 1: buy shares > sell shares Result: adjust number of buy shares (trade complete)
            Case 2: buy shares == sell shares Result: remove buy order (trade complete)
            Case 3: buy shares < sell shares Result: remove buy order and move onto next buy order if exists (trade not completed)
            */
            if (buyShares > order->quantity){
                remainingShares = buyShares - order->quantity;
                // update buyShares and PriceLevel totalQuantity (since partial fill)
                PriceLevelPartialFillFrontOrder(bestBidPriceLevel, order->quantity);
                ENGINE_LOG("**TRADE** SellId: %llu -- BuyId: %llu\nShares Sold: %u Buy Shares Remaining: %u\n", order->id, bestBidOrder->id, order->quantity, bestBidOrder->quantity);
                order->quantity = 0;
                ENGINE_LOG("TRADE COMPLETE\n");
                return true;
            }
            if (buyShares == order->quantity){
                ENGINE_LOG("**TRADE** SellId: %llu -- BuyId: %llu\nShares Sold: %u Buy Shares Remaining: 0\n", order->id, bestBidOrder->id, order->quantity);
                order->quantity = 0;
                PriceLevelRemoveFrontOrder(bestBidPriceLevel);
                if (PriceLevelEmpty(bestBidPriceLevel)){
                    OrderBookRemovePriceLevel(orderBook->bids, &orderBook->bidsCount, 0);
                }
                ENGINE_LOG("TRADE COMPLETE\n");
                return true;
            }
            else{
                remainingShares = order->quantity - buyShares;
                uint32_t sharesSold = order->quantity - remainingShares;
                ENGINE_LOG("**TRADE** SellId: %llu -- BuyId: %llu\nShares Sold: %u Buy Shares Remaining: 0\n", order->id, bestBidOrder->id, sharesSold);
                order->quantity = remainingShares;
                PriceLevelRemoveFrontOrder(bestBidPriceLevel);
                if (PriceLevelEmpty(bestBidPriceLevel)){
                    OrderBookRemovePriceLevel(orderBook->bids, &orderBook->bidsCount, 0);
                }
                ENGINE_LOG("TRADE PARTIALLY COMPLETE\n");
                continue;
            }
        }
        else{
            break;
        }
    }
    // add sell order to appropriate priceLevel in orderBook if all shares weren't traded
    if (!OrderBookAddSellOrder(orderBook, *order)){return false;}
    return true;
}

bool MatchingEngineProcessOrder(OrderBook* orderBook, Order* order){
    if (!orderBook || !order){return false;}
    if (order->side == BUY){return MatchingEngineProcessBuyOrder(orderBook, order);}
    if (order->side == SELL){return MatchingEngineProcessSellOrder(orderBook, order);}
    return false;
}