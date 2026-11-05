#include <iostream>
#include <string>
#include <stdint.h>

#include "FIFOQueue.h"
#include "PriceLevel.h"
#include "OrderBook.h"
#include "MatchingEngine.h"

int testsPassed = 0;
int testsFailed = 0;

// Report the outcome of one test
void Check(bool condition, const std::string& testName){
    if (condition){
        std::cout << "[PASS] " << testName << "\n";
        testsPassed++;
    }
    else{
        std::cout << "[FAIL] " << testName << "\n";
        testsFailed++;
    }
}

// Compare expected and actual integer values
void CheckEqual(int expected, int actual, const std::string& testName){
    Check(expected == actual, testName);
    if (expected != actual){
        std::cout << " Expected: " << expected << ", Actual: " << actual << "\n";
    }
}

void CheckPointer(const void* expected, const void* actual, const std::string& name){
    Check(expected == actual, name);
}

// Print the final summary
void PrintTestSummary(){
    std::cout << "\n------------------TEST SUMMARY------------------\n";
    std::cout << "Passed : " << testsPassed << "\n";
    std::cout << "Failed : " << testsFailed << "\n";
    std::cout << "Total : " << testsPassed + testsFailed << "\n";
}

// Test helpers
Order MakeOrder(uint64_t id, uint32_t price, uint32_t quantity, uint8_t side){
    Order order{};
    order.id = id;
    order.time = id;
    order.price = price;
    order.quantity = quantity;
    order.side = side;
    return order;
}

void CheckOrderId(const Order* order, uint64_t expectedId, const std::string& name){
    Check(order != nullptr, name + " - order exists");
    if (order != nullptr){
        CheckEqual(expectedId, order->id, name + " - Order Id");
    }
}

// FIFOQueue Tests
void TestFIFOQueueInitialization(){
    FIFOQueue fifoQueue{};
    Check(FIFOQueueInitialize(&fifoQueue), "Queue initialized successfully");
    CheckEqual(0, fifoQueue.count, "New fifoQueue has zero orders");
    CheckEqual(FIFOQUEUE_START_CAPACITY, fifoQueue.capacity, "fifoQueue starts with configured capacity");
    Check(FIFOQueueGetFrontOrder(&fifoQueue) == nullptr, "Empty queue has no front order");
    Check(FIFOQueuePeekOrder(&fifoQueue, 0) == nullptr, "Peaking from empty queue returns nullptr");
    FIFOQueueDestroy(&fifoQueue);
    Check(FIFOQueueInitialize(nullptr) == false, "Queue initialization rejects nullptr");
    Check(FIFOQueueAddOrder(nullptr, MakeOrder(1, 100, 1, BUY)) == false, "Queue insertion rejects nullptr queue");
}

void TestFIFOQueueInsertionAndPeek(){
    FIFOQueue fifoQueue{};
    if (!FIFOQueueInitialize(&fifoQueue)){
        Check(false, "fifoQueue setup for insertion test");
        return;
    }
    Check(FIFOQueueAddOrder(&fifoQueue, MakeOrder(1, 100, 10, BUY)), "Insert first order");
    Check(FIFOQueueAddOrder(&fifoQueue, MakeOrder(2, 101, 20, BUY)), "Insert second order");
    Check(FIFOQueueAddOrder(&fifoQueue, MakeOrder(3, 102, 30, BUY)), "Insert third order");
    CheckEqual(3, fifoQueue.count, "fifoQueue count increases after insertions");
    CheckOrderId(FIFOQueueGetFrontOrder(&fifoQueue), 1, "Front is the oldest order");
    CheckOrderId(FIFOQueuePeekOrder(&fifoQueue, 0), 1, "Peek index 0 returns first order");
    CheckOrderId(FIFOQueuePeekOrder(&fifoQueue, 1), 2, "Peek index 1 returns second order");
    CheckOrderId(FIFOQueuePeekOrder(&fifoQueue, 2), 3, "Peek index 2 returns third order");
    Check(FIFOQueuePeekOrder(&fifoQueue, 3) == nullptr, "Peek beyond valid queue count returns nullptr");
    Check(FIFOQueuePeekOrder(nullptr, 0) == nullptr, "Peek rejects null queue");
    FIFOQueueDestroy(&fifoQueue);
}

void TestFIFOQueueRemoval(){
    FIFOQueue fifoQueue{};
    if (!FIFOQueueInitialize(&fifoQueue)){
        Check(false, "fifoQueue setup for removal test");
        return;
    }
    FIFOQueueAddOrder(&fifoQueue, MakeOrder(10, 100, 1, BUY));
    FIFOQueueAddOrder(&fifoQueue, MakeOrder(20, 101, 1, BUY));
    FIFOQueueAddOrder(&fifoQueue, MakeOrder(30, 102, 1, BUY));
    Order* removed = FIFOQueueRemoveFrontOrder(&fifoQueue);
    CheckOrderId(removed, 10, "Removing front returns the removed order");
    CheckEqual(2, fifoQueue.count, "Count decreases after removing one order");
    CheckOrderId(FIFOQueueGetFrontOrder(&fifoQueue), 20, "Second order becomes the new front");
    removed = FIFOQueueRemoveFrontOrder(&fifoQueue);
    CheckOrderId(removed, 20, "Second removal returns the second order");
    removed = FIFOQueueRemoveFrontOrder(&fifoQueue);
    CheckOrderId(removed, 30, "Third removal returns the third order");
    CheckEqual(fifoQueue.count, 0, "Queue count reaches 0 after all removals");
    Check(FIFOQueueGetFrontOrder(&fifoQueue) == nullptr, "Front is null after removing all orders");
    Check(FIFOQueueRemoveFrontOrder(nullptr) == nullptr, "Removal rejects null queue");
    FIFOQueueDestroy(&fifoQueue);
}

void TestFIFOQueueWrapAround(){
    FIFOQueue fifoQueue{};
    if (!FIFOQueueInitialize(&fifoQueue)){
        Check(false, "Queue setup for wrap-around test");
        return;
    }
    bool inserted = true;
    for (uint64_t i = 0; i < 100; ++i)
    {
        if (!FIFOQueueAddOrder(&fifoQueue, MakeOrder(i, 100, 1, BUY))){
            inserted = false;
            break;
        }
    }
    Check(inserted, "Insert orders before wrap-around");
    bool removedCorrectly = true;
    for (uint64_t i = 0; i < 80; ++i)
    {
        Order* front = FIFOQueueGetFrontOrder(&fifoQueue);
        if (front == nullptr || front->id != i){
            removedCorrectly = false;
            break;
        }
        FIFOQueueRemoveFrontOrder(&fifoQueue);
    }
    Check(removedCorrectly, "Front advances correctly before wrap-around");
    inserted = true;
    for (uint64_t i = 100; i < 200; ++i)
    {
        if (!FIFOQueueAddOrder(&fifoQueue, MakeOrder(i, 100, 1, BUY))){
            inserted = false;
            break;
        }
    }
    Check(inserted, "Insert orders after wrap-around");
    CheckEqual(120, fifoQueue.count, "Wrap-around preserves correct queue count");
    bool orderCorrect = true;
    for (uint32_t i = 0; i < fifoQueue.count; ++i)
    {
        Order* order = FIFOQueuePeekOrder(&fifoQueue, i);
        uint64_t expectedId = (i < 20) ? i + 80 : i + 80;
        if (order == nullptr || order->id != expectedId){
            orderCorrect = false;
            break;
        }
    }
    Check(orderCorrect, "FIFO order remains correct after wrap-around");
    FIFOQueueDestroy(&fifoQueue);
}

void TestFIFOQueueResize(){
    FIFOQueue queue{};
    if (!FIFOQueueInitialize(&queue)){
        Check(false, "Queue setup for resize test");
        return;
    }
    const uint32_t orderCount = 300;
    bool inserted = true;
    for (uint32_t i = 0; i < orderCount; ++i){
        if (!FIFOQueueAddOrder(&queue, MakeOrder(i, 100 + i, i + 1, BUY))){
            inserted = false;
            break;
        }
    }
    Check(inserted, "Insert enough orders to resize queue");
    Check(queue.capacity >= orderCount, "Queue capacity grows to fit all inserted orders");
    CheckEqual(orderCount, queue.count, "Queue count survives resize");
    bool preserved = true;
    for (uint32_t i = 0; i < orderCount; ++i){
        Order* order = FIFOQueuePeekOrder(&queue, i);
        if (order == nullptr || order->id != i || order->price != 100 + i || order->quantity != i + 1){
            preserved = false;
            break;
        }
    }
    Check(preserved, "Resize preserves order data and FIFO ordering");
    Check(FIFOQueueResize(&queue),"Explicit queue resize succeeds");
    CheckEqual(orderCount, queue.count, "Explicit resize preserves queue count");
    CheckOrderId(FIFOQueuePeekOrder(&queue, 0), 0,"First order survives explicit resize");
    CheckOrderId(FIFOQueuePeekOrder(&queue, orderCount - 1),orderCount - 1,"Last order survives explicit resize");
    FIFOQueueDestroy(&queue);
}

void TestFIFOQueueDestroy(){
    FIFOQueue queue{};
    FIFOQueueInitialize(&queue);
    FIFOQueueAddOrder(&queue, MakeOrder(1, 100, 1, BUY));
    Check(FIFOQueueDestroy(&queue),"Queue destruction succeeds");
    Check(queue.orders == nullptr,"Destroy resets order storage pointer");
    CheckEqual(0, queue.count,"Destroy resets queue count");
    CheckEqual(0, queue.capacity,"Destroy resets queue capacity");
    Check(FIFOQueueDestroy(&queue),"Destroying an already destroyed queue is safe");
    Check(FIFOQueueDestroy(nullptr),"Destroy accepts null pointer");
}

// PriceLevel tests

void TestPriceLevelInitialization(){
    PriceLevel level{};
    Check(PriceLevelInitialize(&level, 105),"Price level initializes");
    CheckEqual(105, level.price,"Price level stores its price");
    CheckEqual(0, level.totalQuantity,"New price level has zero total quantity");
    Check(PriceLevelEmpty(&level),"New price level is empty");
    Check(PriceLevelInitialize(nullptr, 100) == false,"Price level initialization rejects null");
    Check(PriceLevelEmpty(nullptr),"Null price level is treated as empty");
    PriceLevelDestroy(&level);
}

void TestPriceLevelInsertion(){
    PriceLevel level{};
    if (!PriceLevelInitialize(&level, 100)){
        Check(false, "Price level setup for insertion test");
        return;
    }
    Check(PriceLevelAddOrder(&level, MakeOrder(1, 100, 10, BUY)),"Insert first order at price level");
    Check(PriceLevelAddOrder(&level, MakeOrder(2, 100, 20, BUY)),"Insert second order at price level");
    CheckEqual(30, level.totalQuantity,"Price level total quantity sums orders");
    CheckEqual(2, level.orders.count,"Price level queue count matches inserted orders");
    CheckOrderId(PriceLevelGetFrontOrder(&level), 1,"Price level front follows FIFO");
    Check(PriceLevelAddOrder(nullptr, MakeOrder(3, 100, 5, BUY)) == false,"Adding order rejects null price level");
    PriceLevelDestroy(&level);
}

void TestPriceLevelRemovalAndQuantity(){
    PriceLevel level{};
    if (!PriceLevelInitialize(&level, 100)){
        Check(false, "Price level setup for removal test");
        return;
    }
    PriceLevelAddOrder(&level, MakeOrder(1, 100, 10, BUY));
    PriceLevelAddOrder(&level, MakeOrder(2, 100, 20, BUY));
    Order* removed = PriceLevelRemoveFrontOrder(&level);
    CheckOrderId(removed, 1,"Price level removal returns front order");
    CheckEqual(20, level.totalQuantity,"Removing order subtracts its quantity");
    CheckEqual(1, level.orders.count,"Removing order decrements queue count");
    CheckOrderId(PriceLevelGetFrontOrder(&level), 2,"Next order becomes front after removal");
    PriceLevelRemoveFrontOrder(&level);
    CheckEqual(0, level.totalQuantity,"Removing final order leaves zero quantity");
    Check(PriceLevelEmpty(&level),"Price level is empty after all orders are removed");
    Check(PriceLevelRemoveFrontOrder(nullptr) == nullptr,"Removing from null price level returns null");
    PriceLevelDestroy(&level);
}

void TestPriceLevelDestroy(){
    PriceLevel level{};
    PriceLevelInitialize(&level, 100);
    PriceLevelAddOrder(&level, MakeOrder(1, 100, 10, BUY));
    Check(PriceLevelDestroy(&level),"Price level destruction succeeds");
    CheckEqual(0, level.price,"Destroy resets price");
    CheckEqual(0, level.totalQuantity,"Destroy resets total quantity");
    CheckEqual(0, level.orders.capacity,"Destroy releases queue storage");
    Check(PriceLevelDestroy(nullptr),"Price level destroy accepts null pointer");
}

// OrderBook tests

void TestOrderBookInitialization(){
    OrderBook book{};
    Check(OrderBookInitialize(&book),"Order book initializes");
    CheckEqual(0, book.bidsCount,"New order book has no bid levels");
    CheckEqual(0, book.asksCount,"New order book has no ask levels");
    Check(book.bids != nullptr,"Bid price-level storage is allocated");
    Check(book.asks != nullptr,"Ask price-level storage is allocated");
    Check(OrderBookBestBid(&book) == nullptr,"Empty order book has no best bid");
    Check(OrderBookBestAsk(&book) == nullptr,"Empty order book has no best ask");
    Check(OrderBookInitialize(nullptr) == false,"Order book initialization rejects null");
    OrderBookDestroy(&book);
}

void TestOrderBookPriceLevelOrdering(){
    OrderBook book{};
    if (!OrderBookInitialize(&book)){
        Check(false, "Order book setup for ordering test");
        return;
    }
    Check(OrderBookInsertBidPriceLevel(&book, 100),"Insert bid price 100");
    Check(OrderBookInsertBidPriceLevel(&book, 105),"Insert bid price 105");
    Check(OrderBookInsertBidPriceLevel(&book, 102),"Insert bid price 102");
    CheckEqual(3, book.bidsCount,"Bid count matches inserted levels");
    CheckEqual(105, book.bids[0].price,"Best bid is the highest bid");
    CheckEqual(102, book.bids[1].price,"Second bid is in descending order");
    CheckEqual(100, book.bids[2].price,"Lowest bid is last");
    Check(OrderBookInsertAskPriceLevel(&book, 110),"Insert ask price 110");
    Check(OrderBookInsertAskPriceLevel(&book, 103),"Insert ask price 103");
    Check(OrderBookInsertAskPriceLevel(&book, 107),"Insert ask price 107");
    CheckEqual(103, book.asks[0].price,"Best ask is the lowest ask");
    CheckEqual(107, book.asks[1].price,"Second ask is in ascending order");
    CheckEqual(110, book.asks[2].price,"Highest ask is last");
    Check(OrderBookBestBid(&book) == &book.bids[0],"Best bid returns first bid level");
    Check(OrderBookBestAsk(&book) == &book.asks[0],"Best ask returns first ask level");
    OrderBookDestroy(&book);
}

void TestOrderBookFindAndInsertPositions(){
    OrderBook book{};
    if (!OrderBookInitialize(&book)){
        Check(false, "Order book setup for find test");
        return;
    }
    OrderBookInsertBidPriceLevel(&book, 110);
    OrderBookInsertBidPriceLevel(&book, 100);
    OrderBookInsertBidPriceLevel(&book, 105);
    CheckEqual(0, OrderBookFindBidPriceLevel(&book, 110),"Find existing bid level");
    CheckEqual(-1, OrderBookFindBidPriceLevel(&book, 999),"Missing bid level returns -1");
    CheckEqual(1, OrderBookFindBidPriceLevelInsertPosition(&book, 107),"Find insertion position for bid");
    CheckEqual(3, OrderBookFindBidPriceLevelInsertPosition(&book, 90),"Lower bid belongs at end");
    OrderBookInsertAskPriceLevel(&book, 120);
    OrderBookInsertAskPriceLevel(&book, 130);
    OrderBookInsertAskPriceLevel(&book, 125);
    CheckEqual(1, OrderBookFindAskPriceLevel(&book, 125),"Find existing ask level");
    CheckEqual(-1, OrderBookFindAskPriceLevel(&book, 999),"Missing ask level returns -1");
    CheckEqual(1, OrderBookFindAskPriceLevelInsertPosition(&book, 123),"Find insertion position for ask");
    CheckEqual(3, OrderBookFindAskPriceLevelInsertPosition(&book, 140),"Higher ask belongs at end");
    CheckEqual(-1, OrderBookFindBidPriceLevel(nullptr, 100),"Bid lookup rejects null order book");
    CheckEqual(-1, OrderBookFindAskPriceLevel(nullptr, 100),"Ask lookup rejects null order book");
    OrderBookDestroy(&book);
}

void TestOrderBookDuplicateAndInvalidLevels(){
    OrderBook book{};
    if (!OrderBookInitialize(&book)){
        Check(false, "Order book setup for invalid-level test");
        return;
    }
    Check(OrderBookInsertBidPriceLevel(&book, 100),"Insert initial bid level");
    Check(OrderBookInsertBidPriceLevel(&book, 100) == false,"Reject duplicate bid level");
    Check(OrderBookInsertAskPriceLevel(&book, 100),"Insert initial ask level");
    Check(OrderBookInsertAskPriceLevel(&book, 100) == false,"Reject duplicate ask level");
    Check(OrderBookInsertBidPriceLevel(&book, 0) == false,"Reject zero bid price");
    Check(OrderBookInsertAskPriceLevel(&book, 0) == false,"Reject zero ask price");
    Check(OrderBookInsertBidPriceLevel(nullptr, 100) == false,"Bid insertion rejects null order book");
    Check(OrderBookInsertAskPriceLevel(nullptr, 100) == false,"Ask insertion rejects null order book");
    OrderBookDestroy(&book);
}

void TestOrderBookAddingOrders(){
    OrderBook book{};
    if (!OrderBookInitialize(&book)){
        Check(false, "Order book setup for order insertion test");
        return;
    }
    Check(OrderBookAddOrder(&book, MakeOrder(1, 100, 10, BUY)),"Add buy order");
    Check(OrderBookAddOrder(&book, MakeOrder(2, 105, 20, BUY)),"Add second buy order");
    Check(OrderBookAddOrder(&book, MakeOrder(3, 110, 15, SELL)),"Add sell order");
    CheckEqual(2, book.bidsCount,"Buy orders create expected bid levels");
    CheckEqual(1, book.asksCount,"Sell order creates expected ask level");
    CheckEqual(105, OrderBookBestBid(&book)->price,"Best bid reflects highest price");
    CheckEqual(110, OrderBookBestAsk(&book)->price,"Best ask reflects lowest ask");
    CheckEqual(10, book.bids[1].totalQuantity,"First bid level tracks its quantity");
    CheckEqual(20, book.bids[0].totalQuantity,"Second bid level tracks its quantity");
    CheckEqual(15, book.asks[0].totalQuantity,"Ask level tracks its quantity");
    Check(OrderBookAddOrder(&book, MakeOrder(4, 100, 1, 99)) == false,"Reject order with invalid side");
    Check(OrderBookAddOrder(nullptr, MakeOrder(5, 100, 1, BUY)) == false,"Order insertion rejects null order book");
    OrderBookDestroy(&book);
}

void TestOrderBookPriceLevelRemoval(){
    OrderBook book{};
    if (!OrderBookInitialize(&book)){
        Check(false, "Order book setup for level removal test");
        return;
    }
    OrderBookInsertBidPriceLevel(&book, 110);
    OrderBookInsertBidPriceLevel(&book, 100);
    OrderBookInsertBidPriceLevel(&book, 105);
    Check(OrderBookRemovePriceLevel(book.bids, &book.bidsCount, 1),"Remove middle bid price level");
    CheckEqual(2, book.bidsCount,"Bid count decreases after removal");
    CheckEqual(110, book.bids[0].price,"First bid remains correct after shifting");
    CheckEqual(100, book.bids[1].price,"Remaining bid shifts into removed position");
    Check(OrderBookRemovePriceLevel(book.bids, &book.bidsCount, 10) == false,"Reject removal at out-of-range index");
    Check(OrderBookRemovePriceLevel(nullptr, &book.bidsCount, 0) == false,"Reject null price-level array");
    Check(OrderBookRemovePriceLevel(book.bids, nullptr, 0) == false,"Reject null count pointer");
    OrderBookDestroy(&book);
}

void TestOrderBookResize(){
    OrderBook book{};
    if (!OrderBookInitialize(&book)){
        Check(false, "Order book setup for resize test");
        return;
    }
    OrderBookAddOrder(&book, MakeOrder(1, 100, 10, BUY));
    OrderBookAddOrder(&book, MakeOrder(2, 110, 20, SELL));
    uint32_t oldBidCapacity = book.bidsCapacity;
    uint32_t oldAskCapacity = book.asksCapacity;
    Check(OrderBookResize(&book, BUY),"Resize bid storage");
    CheckEqual(oldBidCapacity * 2, book.bidsCapacity,"Bid capacity doubles");
    CheckEqual(1, book.bidsCount,"Bid count survives resize");
    CheckEqual(100, book.bids[0].price,"Bid price survives resize");
    CheckEqual(10, book.bids[0].totalQuantity,"Bid quantity survives resize");
    CheckOrderId(FIFOQueueGetFrontOrder(&book.bids[0].orders),1, "Bid order survives resize");
    Check(OrderBookResize(&book, SELL),"Resize ask storage");
    CheckEqual(oldAskCapacity * 2, book.asksCapacity,"Ask capacity doubles");
    CheckEqual(1, book.asksCount,"Ask count survives resize");
    CheckEqual(110, book.asks[0].price,"Ask price survives resize");
    CheckOrderId(FIFOQueueGetFrontOrder(&book.asks[0].orders),2, "Ask order survives resize");
    Check(OrderBookResize(&book, 99) == false,"Reject invalid resize side");
    Check(OrderBookResize(nullptr, BUY) == false,"Resize rejects null order book");
    OrderBookDestroy(&book);
}

void TestOrderBookDestroy(){
    OrderBook book{};
    OrderBookInitialize(&book);
    OrderBookAddOrder(&book, MakeOrder(1, 100, 10, BUY));
    OrderBookAddOrder(&book, MakeOrder(2, 110, 10, SELL));
    Check(OrderBookDestroy(&book),"Order book destruction succeeds");
    Check(book.bids == nullptr,"Destroy resets bid storage pointer");
    Check(book.asks == nullptr,"Destroy resets ask storage pointer");
    CheckEqual(0, book.bidsCount,"Destroy resets bid count");
    CheckEqual(0, book.asksCount,"Destroy resets ask count");
    Check(OrderBookDestroy(nullptr) == false,"Order book destroy rejects null pointer");
}

// MatchingEngine tests

void TestMatchingInvalidInputs(){
    OrderBook book{};
    if (!OrderBookInitialize(&book)){
        Check(false, "Order book setup for matching validation");
        return;
    }
    Order order = MakeOrder(1, 100, 10, BUY);
    Check(MatchingEngineProcessOrder(nullptr, &order) == false,"Matching rejects null order book");
    Check(MatchingEngineProcessOrder(&book, nullptr) == false,"Matching rejects null order");
    order.quantity = 0;
    Check(MatchingEngineProcessOrder(&book, &order) == false,"Matching rejects zero quantity");
    order = MakeOrder(2, 100, 10, 99);
    Check(MatchingEngineProcessOrder(&book, &order) == false,"Matching rejects invalid side");
    OrderBookDestroy(&book);
}

void TestMatchingNoCross(){
    OrderBook book{};
    if (!OrderBookInitialize(&book)){
        Check(false, "Order book setup for no-cross test");
        return;
    }
    Order sell = MakeOrder(1, 110, 5, SELL);
    Check(MatchingEngineProcessOrder(&book, &sell),"Resting sell order is accepted");
    Order buy = MakeOrder(2, 100, 3, BUY);
    Check(MatchingEngineProcessOrder(&book, &buy),"Non-crossing buy order is accepted");
    CheckEqual(1, book.asksCount,"Non-crossing sell remains on book");
    CheckEqual(1, book.bidsCount,"Non-crossing buy rests on book");
    CheckEqual(110, OrderBookBestAsk(&book)->price,"Sell price remains unchanged");
    CheckEqual(100, OrderBookBestBid(&book)->price,"Buy price remains unchanged");
    CheckEqual(5, OrderBookBestAsk(&book)->totalQuantity,"Resting sell quantity remains unchanged");
    CheckEqual(3, OrderBookBestBid(&book)->totalQuantity,"Resting buy quantity remains unchanged");
    OrderBookDestroy(&book);
}

void TestMatchingExactFill(){
    OrderBook book{};
    if (!OrderBookInitialize(&book)){
        Check(false, "Order book setup for exact-fill test");
        return;
    }
    Order sell = MakeOrder(1, 100, 5, SELL);
    Check(MatchingEngineProcessOrder(&book, &sell),"Insert sell order before exact fill");
    Order buy = MakeOrder(2, 100, 5, BUY);
    Check(MatchingEngineProcessOrder(&book, &buy),"Matching buy order fills sell exactly");
    CheckEqual(0, buy.quantity,"Buy quantity becomes zero after full fill");
    CheckEqual(0, book.asksCount,"Fully filled ask level is removed");
    CheckEqual(0, book.bidsCount,"Fully filled buy order does not rest");
    Check(OrderBookBestAsk(&book) == nullptr,"No best ask remains after exact fill");
    OrderBookDestroy(&book);
}

void TestMatchingPartialFillAgainstLargerRestingOrder(){
    OrderBook book{};
    if (!OrderBookInitialize(&book)){
        Check(false, "Order book setup for partial-fill test");
        return;
    }
    Order sell = MakeOrder(1, 100, 10, SELL);
    MatchingEngineProcessOrder(&book, &sell);
    Order buy = MakeOrder(2, 100, 4, BUY);
    Check(MatchingEngineProcessOrder(&book, &buy),"Buy partially fills resting sell");
    CheckEqual(0, buy.quantity,"Incoming buy is fully filled");
    CheckEqual(1, book.asksCount,"Partially filled ask level remains");
    if (book.asksCount > 0){
        CheckEqual(6, book.asks[0].totalQuantity,"Ask total quantity decreases after partial fill");
        CheckOrderId(FIFOQueueGetFrontOrder(&book.asks[0].orders),1, "Original sell order remains");
        Order* remaining = FIFOQueueGetFrontOrder(&book.asks[0].orders);
        if (remaining != nullptr){
            CheckEqual(6, remaining->quantity,"Resting sell has correct remaining quantity");
        }
    }
    OrderBookDestroy(&book);
}

void TestMatchingIncomingOrderLargerThanRestingOrder(){
    OrderBook book{};
    if (!OrderBookInitialize(&book)){
        Check(false, "Order book setup for incoming-larger test");
        return;
    }
    Order sell = MakeOrder(1, 100, 4, SELL);
    MatchingEngineProcessOrder(&book, &sell);
    Order buy = MakeOrder(2, 100, 10, BUY);
    Check(MatchingEngineProcessOrder(&book, &buy),"Larger buy consumes resting sell");
    CheckEqual(6, buy.quantity,"Incoming buy retains unfilled quantity");
    CheckEqual(0, book.asksCount,"Fully consumed ask level is removed");
    CheckEqual(1, book.bidsCount,"Remaining buy order rests on bid side");
    if (book.bidsCount > 0){
        CheckEqual(100, book.bids[0].price,"Remaining buy uses original limit price");
        CheckEqual(6, book.bids[0].totalQuantity,"Remaining bid level has correct total quantity");
        Order* remaining = FIFOQueueGetFrontOrder(&book.bids[0].orders);
        Check(remaining != nullptr,"Remaining buy order exists");
        if (remaining != nullptr){
            CheckEqual(2, remaining->id,"Correct incoming buy order rests");
            CheckEqual(6, remaining->quantity,"Resting buy order has correct remaining quantity");
        }
    }
    OrderBookDestroy(&book);
}

void TestMatchingSellAgainstBid(){
    OrderBook book{};
    if (!OrderBookInitialize(&book)){
        Check(false, "Order book setup for sell matching test");
        return;
    }
    Order buy = MakeOrder(1, 105, 8, BUY);
    MatchingEngineProcessOrder(&book, &buy);
    Order sell = MakeOrder(2, 100, 3, SELL);
    Check(MatchingEngineProcessOrder(&book, &sell),"Sell order matches a resting bid");
    CheckEqual(0, sell.quantity,"Incoming sell is fully filled");
    CheckEqual(1, book.bidsCount,"Partially filled bid level remains");
    if (book.bidsCount > 0){
        CheckEqual(5, book.bids[0].totalQuantity,"Bid total quantity decreases after fill");
        Order* remaining = FIFOQueueGetFrontOrder(&book.bids[0].orders);
        Check(remaining != nullptr,"Remaining bid order exists");
        if (remaining != nullptr){
            CheckEqual(5, remaining->quantity,"Bid order retains correct remaining quantity");
        }
    }
    OrderBookDestroy(&book);
}

void TestMatchingPricePriority(){
    OrderBook book{};
    if (!OrderBookInitialize(&book)){
        Check(false, "Order book setup for price-priority test");
        return;
    }
    Order sell105 = MakeOrder(1, 105, 2, SELL);
    Order sell100 = MakeOrder(2, 100, 3, SELL);
    Check(MatchingEngineProcessOrder(&book, &sell105),"Insert sell order at price 105");
    Check(MatchingEngineProcessOrder(&book, &sell100),"Insert sell order at price 100");
    // The best ask should be 100, since asks are sorted ascending.
    PriceLevel* bestAsk = OrderBookBestAsk(&book);
    Check(bestAsk != nullptr, "Best ask exists before matching");
    if (bestAsk != nullptr){
        CheckEqual(100, bestAsk->price,"Lowest ask has price priority");
    }
    // This buy should fill the 3-share ask at 100, not the ask at 105.
    Order buy = MakeOrder(3, 100, 3, BUY);
    Check(MatchingEngineProcessOrder(&book, &buy),"Buy matches the best-priced ask first");
    CheckEqual(0, buy.quantity,"Buy is fully filled");
    CheckEqual(1, book.asksCount,"Higher-priced ask remains");
    if (book.asksCount > 0){
        CheckEqual(105, book.asks[0].price,"Higher-priced ask remains after matching");
        CheckEqual(2, book.asks[0].totalQuantity,"Remaining ask quantity is unchanged");}
    OrderBookDestroy(&book);
}

// Run all tests

int main()
{
    std::cout << "\n-----------FIFO QUEUE TESTS-----------\n";
    TestFIFOQueueInitialization();
    TestFIFOQueueInsertionAndPeek();
    TestFIFOQueueRemoval();
    TestFIFOQueueWrapAround();
    TestFIFOQueueResize();
    TestFIFOQueueDestroy();

    std::cout << "\n-----------PRICE LEVEL TESTS-----------\n";
    TestPriceLevelInitialization();
    TestPriceLevelInsertion();
    TestPriceLevelRemovalAndQuantity();
    TestPriceLevelDestroy();

    std::cout << "\n-----------ORDER BOOK TESTS-----------\n";
    TestOrderBookInitialization();
    TestOrderBookPriceLevelOrdering();
    TestOrderBookFindAndInsertPositions();
    TestOrderBookDuplicateAndInvalidLevels();
    TestOrderBookAddingOrders();
    TestOrderBookPriceLevelRemoval();
    TestOrderBookResize();
    TestOrderBookDestroy();

    std::cout << "\n-----------MATCHING ENGINE TESTS-----------\n";
    TestMatchingInvalidInputs();
    TestMatchingNoCross();
    TestMatchingExactFill();
    TestMatchingPartialFillAgainstLargerRestingOrder();
    TestMatchingIncomingOrderLargerThanRestingOrder();
    TestMatchingSellAgainstBid();
    TestMatchingPricePriority();

    std::cout << "Tests Passed: " << testsPassed << "\n";
    std::cout << "Tests Failed: " << testsFailed << "\n";

    return testsFailed == 0 ? 0 : 1;
}