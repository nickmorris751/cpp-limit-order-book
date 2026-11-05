# C++ Limit Order Book & Matching Engine

A high-performance limit order book and matching engine implemented in C++.

This project was developed to explore the design and implementation of a limit order book, focusing on efficient data structures, price-time priority, memory management, and performance optimization.

The matching engine supports buy and sell limit orders, matches compatible orders automatically, and maintains an organized order book using price levels and FIFO queues.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Project Structure](#project-structure)
- [Architecture](#architecture)
- [Data Structures](#data-structures)
  - [Order](#1-order)
  - [FIFOQueue](#2-fifoqueue)
  - [PriceLevel](#3-pricelevel)
  - [OrderBook](#4-orderbook)
- [Matching Engine](#matching-engine)
- [Matching Algorithm](#matching-algorithm)
- [Design Decisions](#design-decisions)
- [Performance Optimizations](#performance-optimizations)
- [Benchmarking](#benchmarking)
- [Benchmark Results](#benchmark-results)
- [Building and Running](#building-and-running)
- [Testing](#testing)
- [Current Limitations](#current-limitations)
- [Potential Improvements](#potential-improvements)
- [Project Goals](#project-goals)

---

## Overview

A limit order book is a data structure used by financial exchanges to organize buy and sell orders for a particular financial instrument.

The order book maintains two sides:

- **Bids:** Buy orders, organized from highest price to lowest price.
- **Asks:** Sell orders, organized from lowest price to highest price.

The matching engine processes incoming orders and attempts to execute trades against existing orders whenever their prices are compatible.

This implementation follows **price-time priority**:

1. Better prices receive priority.
2. At the same price, earlier orders receive priority.

For example, if multiple sell orders exist at the same price, the oldest order is matched first.

The project focuses on implementing these mechanics while maintaining efficient memory usage and minimizing unnecessary allocations.

## Features

- Buy and sell limit orders.
- Automatic order matching.
- Price-time priority.
- Separate bid and ask sides.
- Price levels organized in sorted contiguous arrays.
- Binary search for price-level lookup and insertion-position discovery.
- FIFO queues implemented using contiguous ring buffers.
- Dynamic resizing of queues and order book arrays.
- Aggregate quantity tracking at each price level.
- Partial order fills.
- Automatic removal of empty price levels.
- Compile-time configurable logging.
- Synthetic order generation for benchmarking.
- Throughput and sampled latency measurements.

## Project Structure

```text
MarketLimitOrderBook/
│
├── src/
│   ├── Order.h
│   │
│   ├── FIFOQueue.h
│   ├── FIFOQueue.cpp
│   │
│   ├── PriceLevel.h
│   ├── PriceLevel.cpp
│   │
│   ├── OrderBook.h
│   ├── OrderBook.cpp
│   │
│   ├── MatchingEngine.h
│   ├── MatchingEngine.cpp
│   │
│   ├── Debug.h
│   │
│   ├── Tests.cpp
│   └── Benchmark.cpp
│
├── .gitignore
└── README.md
```

### File Responsibilities

| File | Responsibility |
|---|---|
| `Order.h` | Defines the order structure and buy/sell constants. |
| `FIFOQueue.h/.cpp` | Implements the contiguous FIFO ring buffer. |
| `PriceLevel.h/.cpp` | Manages orders at a specific price and tracks aggregate quantity. |
| `OrderBook.h/.cpp` | Maintains sorted bid and ask price levels. |
| `MatchingEngine.h/.cpp` | Processes incoming orders and executes trades. |
| `Debug.h` | Provides compile-time configurable logging. |
| `Tests.cpp` | Contains tests for the data structures and matching engine. |
| `Benchmark.cpp` | Generates synthetic orders and measures performance. |

## Architecture

The project is divided into four main layers:

```text
                 Matching Engine
                        |
                        v
                    OrderBook
                   /         \
                  v           v
                Bids         Asks
                  |           |
                  v           v
              PriceLevel   PriceLevel
                  |           |
                  v           v
               FIFOQueue   FIFOQueue
                  |           |
                  v           v
                Orders      Orders
```

Each component has a specific responsibility:

- The `MatchingEngine` determines whether incoming orders can execute against existing orders.
- The `OrderBook` organizes price levels and provides access to the best bid and ask.
- Each `PriceLevel` maintains the orders submitted at a particular price.
- Each `FIFOQueue` maintains time priority among orders at the same price.

This separation makes the implementation easier to maintain, test, and optimize.

## Data Structures

### 1. Order

The `Order` structure represents an individual buy or sell order.

```cpp
struct Order {
    uint64_t id;
    uint64_t time;
    uint32_t price;
    uint32_t quantity;
    uint8_t side;
    uint8_t padding[7];
};
```

Fields:

| Field | Description |
|---|---|
| `id` | Unique order identifier. |
| `time` | Order timestamp or sequence value. |
| `price` | Limit price. |
| `quantity` | Remaining order quantity. |
| `side` | BUY or SELL. |
| `padding` | Explicit padding to maintain the intended structure layout. |

The side constants are:

```cpp
#define BUY 0
#define SELL 1
```

The structure uses fixed-width integer types to make field sizes explicit.

The explicit padding is intended to give the structure a predictable 32-byte layout on conventional platforms with the specified field alignment.

### 2. FIFOQueue

The `FIFOQueue` is implemented as a contiguous ring buffer.

```cpp
struct FIFOQueue {
    Order* orders;
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    uint32_t capacity;
};
```

Instead of using a linked list, the queue stores orders in a dynamically allocated contiguous array.

This design provides:

- Better memory locality.
- Fewer individual memory allocations.
- Reduced pointer chasing.
- Efficient insertion and removal.
- More predictable memory access patterns.

#### Ring Buffer Design

The queue uses two indices:

- `head`: Position immediately before the front element.
- `tail`: Position of the last inserted element.

When adding an order, the tail advances and the new order is inserted.

When removing an order, the head advances.

The queue uses the following indexing operation:

```cpp
(index + 1) & (capacity - 1)
```

This replaces modulo arithmetic with a bitwise operation.

This optimization requires the capacity to remain a power of two.

The initial capacity is:

```cpp
#define FIFOQUEUE_START_CAPACITY 128
```

The maximum capacity is:

```cpp
#define FIFOQUEUE_MAX_CAPACITY 1048576
```

When the queue reaches its capacity limit, it doubles its allocation and copies existing orders into the new buffer while preserving FIFO order.

The queue also maintains a `count` field, allowing it to track the number of active orders directly.

### 3. PriceLevel

A `PriceLevel` represents all orders submitted at a particular price.

```cpp
struct PriceLevel {
    FIFOQueue orders;
    uint32_t price;
    uint32_t totalQuantity;
};
```

Each price level contains:

- The price associated with the level.
- A FIFO queue containing all orders at that price.
- The aggregate remaining quantity of all orders at that price.

For example:

```text
Price Level: 10000

Order 1: Quantity 50
Order 2: Quantity 30
Order 3: Quantity 20

Total Quantity: 100
```

#### Aggregate Quantity Tracking

The `PriceLevel` is responsible for maintaining its own aggregate quantity.

When an order is added:

```cpp
priceLevel->totalQuantity += order.quantity;
```

When an order is partially filled:

```cpp
order->quantity -= quantity;
priceLevel->totalQuantity -= quantity;
```

When an order is completely removed:

```cpp
priceLevel->totalQuantity -= order->quantity;
```

This keeps aggregate quantity bookkeeping within the `PriceLevel` abstraction rather than distributing it across the matching engine.

The key invariant is:

```text
totalQuantity == sum of remaining quantities of all orders in the FIFO queue
```

This allows the matching engine to modify orders through the `PriceLevel` interface without directly managing aggregate quantity calculations.

### 4. OrderBook

The `OrderBook` maintains separate arrays for bids and asks.

```cpp
struct OrderBook {
    PriceLevel* bids;
    PriceLevel* asks;

    uint32_t bidsCount;
    uint32_t asksCount;

    uint32_t bidsCapacity;
    uint32_t asksCapacity;
};
```

The two sides are maintained in sorted order.

**Bids:**

```text
Highest price
     |
     v
  10100
  10050
  10000
   9950
   9900
     |
     v
Lowest price
```

**Asks:**

```text
Lowest price
     |
     v
   9900
   9950
  10000
  10050
  10100
     |
     v
Highest price
```

The best bid and best ask are always located at index zero.

This makes retrieving the best available prices an O(1) operation.

#### Binary Search

The order book uses binary search to locate existing price levels and determine insertion positions.

For bids, prices are sorted in descending order.

For asks, prices are sorted in ascending order.

The relevant operations are:

```cpp
OrderBookFindBidPriceLevel()
OrderBookFindAskPriceLevel()

OrderBookFindBidPriceLevelInsertPosition()
OrderBookFindAskPriceLevelInsertPosition()
```

Binary search reduces price-level lookup from O(n) to O(log n).

However, inserting a new price level still requires shifting elements in the contiguous array.

Therefore:

| Operation | Complexity |
|---|---|
| Find bid price level | O(log n) |
| Find ask price level | O(log n) |
| Find bid insertion position | O(log n) |
| Find ask insertion position | O(log n) |
| Retrieve best bid | O(1) |
| Retrieve best ask | O(1) |
| Insert price level | O(n) |
| Remove price level | O(n) |

The contiguous array design favors efficient lookup and cache locality, at the cost of shifting elements when price levels are inserted or removed.

## Matching Engine

The matching engine is responsible for processing incoming orders and executing trades whenever compatible orders exist.

The main entry point is:

```cpp
bool MatchingEngineProcessOrder(OrderBook* orderBook, Order* order);
```

It determines whether the incoming order is a buy or sell order and delegates processing to the appropriate function.

```cpp
MatchingEngineProcessBuyOrder()
MatchingEngineProcessSellOrder()
```

### Buy Order Matching

An incoming buy order can match against the best ask when:

```cpp
bestAskOrder->price <= order->price
```

The matching engine repeatedly examines the best ask until:

- The incoming order is completely filled.
- The best ask price exceeds the incoming buy limit.
- No ask orders remain.

### Sell Order Matching

An incoming sell order can match against the best bid when:

```cpp
bestBidOrder->price >= order->price
```

The matching engine repeatedly examines the best bid until:

- The incoming order is completely filled.
- The best bid price falls below the incoming sell limit.
- No bid orders remain.

### Partial Fills

The matching engine supports partial order execution.

For example:

```text
Incoming Buy Order:
Price: 10000
Quantity: 100

Best Ask:
Price: 9950
Quantity: 40
```

The trade executes for 40 units.

The incoming buy order now has:

```text
Remaining Quantity: 60
```

The remaining 60 units continue matching against the next available ask.

If no more compatible asks exist, the remaining buy order is added to the bid side of the order book.

The existing resting order's remaining quantity and its price level's aggregate quantity are updated through:

```cpp
PriceLevelPartialFillFrontOrder()
```

Completely filled resting orders are removed through:

```cpp
PriceLevelRemoveFrontOrder()
```

If a price level becomes empty, it is removed from the order book.

## Matching Algorithm

The matching engine follows price-time priority.

Consider the following ask side:

```text
ASKS

Price: 10100
  Order A: 20
  Order B: 30

Price: 10050
  Order C: 40
  Order D: 10

Price: 10000
  Order E: 50
```

The best ask is 10000.

An incoming buy order with a limit price of 10050 can match against:

1. Order E at 10000.
2. Order C at 10050.
3. Order D at 10050.

Order C receives priority over Order D because it was submitted first at that price.

The matching engine does not match against orders at prices above the incoming buy limit.

The same principle applies in reverse for incoming sell orders.

This maintains price priority across price levels and FIFO time priority within each level.

## Design Decisions

### 1. Contiguous Arrays Instead of Linked Lists

The order book uses contiguous arrays of `PriceLevel` objects rather than linked lists.

Advantages:

- Improved cache locality.
- Less pointer chasing.
- Efficient binary search.
- Low allocation overhead for the outer price-level structure.

Disadvantages:

- Inserting and removing price levels requires shifting elements.
- Resizing requires allocating and copying the outer array.

This design was selected to prioritize lookup performance and memory locality.

### 2. Ring Buffers Instead of Linked-List Queues

Each price level uses a ring buffer rather than a linked list.

A linked-list queue requires separate node allocations and pointer traversal.

The ring buffer stores orders contiguously and uses head and tail indices to manage FIFO ordering.

This reduces allocation overhead and improves memory locality.

### 3. Power-of-Two Capacities

The FIFO queue starts at a capacity of 128 and doubles whenever it needs to grow.

Because capacities remain powers of two, circular indexing can use:

```cpp
(index + 1) & (capacity - 1)
```

This avoids the more expensive modulo operation.

### 4. Binary Search for Price-Level Lookup

The bid and ask arrays remain sorted at all times.

This allows binary search to locate existing price levels in O(log n) time.

The best bid and ask are also immediately accessible at index zero.

### 5. Aggregate Quantity Tracking

Each price level tracks its total remaining quantity.

The `PriceLevel` abstraction owns this bookkeeping, ensuring that additions, partial fills, and removals update the aggregate quantity consistently.

### 6. Separation of Responsibilities

Each component has a clearly defined responsibility.

- `FIFOQueue` manages order storage and FIFO behavior.
- `PriceLevel` manages orders at a specific price and aggregate quantity.
- `OrderBook` manages price-level organization.
- `MatchingEngine` manages matching decisions and trade execution.

This separation reduces coupling and makes individual components easier to test.

## Performance Optimizations

Several design decisions were made specifically to improve performance.

| Optimization | Purpose |
|---|---|
| Contiguous order storage | Improve cache locality. |
| Ring buffer | Avoid linked-list traversal and individual node allocations. |
| Power-of-two capacities | Enable bitwise circular indexing. |
| Binary search | Reduce price-level lookup complexity. |
| Best price at index zero | Provide O(1) best-price access. |
| Pre-generated benchmark orders | Exclude synthetic order generation from timed execution. |
| Compile-time logging | Avoid unnecessary logging overhead when disabled. |
| Batched latency sampling | Reduce the overhead of measuring individual operations. |

These optimizations are intended to improve both computational efficiency and memory access patterns.

## Benchmarking

The project includes a benchmark program that generates synthetic orders and processes them through the matching engine.

The benchmark is designed to compare different implementations of the order book, particularly linear search versus binary search for price-level lookup.

### Synthetic Order Generation

The benchmark generates:

```cpp
static constexpr uint64_t ORDER_COUNT = 1'000'000;
```

Each order contains:

- A unique order ID.
- A sequential timestamp.
- A randomly generated price.
- A randomly generated quantity.
- A randomly selected buy or sell side.

The price range is:

```cpp
9900 - 10100
```

The quantity range is:

```cpp
1 - 100
```

A deterministic xorshift pseudo-random generator is used so that benchmark runs use the same generated order sequence.

Orders are generated before the timed benchmark begins.

This prevents order generation from affecting the measured matching-engine execution time.

### Throughput Measurement

The benchmark uses:

```cpp
std::chrono::steady_clock
```

to measure total execution time.

Throughput is calculated as:

```cpp
throughput = ORDER_COUNT / elapsedSeconds;
```

The result represents the number of orders processed per second.

### Latency Measurement

Latency is measured using the x86 timestamp counter through:

```cpp
__rdtscp()
```

The benchmark uses the following configuration:

```cpp
static constexpr uint64_t ORDER_COUNT = 1'000'000;
static constexpr uint64_t LATENCY_SAMPLE_INTERVAL = 1000;
static constexpr uint64_t LATENCY_BATCH_SIZE = 100;
```

Every 1,000 orders, the benchmark measures a batch of 100 matching-engine operations.

The total number of latency samples is:

```text
1,000,000 / 1,000 = 1,000 samples
```

Each sample represents the average number of timestamp-counter ticks per order across its 100-order batch.

Therefore:

- Total orders processed: 1,000,000.
- Number of latency samples: 1,000.
- Orders included in sampled batches: 100,000.
- Orders outside sampled batches: 900,000.

The benchmark reports:

- p50 latency.
- p95 latency.
- p99 latency.

These percentiles are calculated across the 1,000 batch-average measurements.

**Important:** These are batch-average timestamp-counter ticks per order, not percentiles of individually timed orders. The timestamp counter is not necessarily equivalent to actual core clock cycles, because modern processors can dynamically adjust their operating frequency.

The batch approach reduces measurement overhead compared with timing every individual order.

## Benchmark Results

The following results were obtained when comparing the linear-search and binary-search implementations.

### Linear Search

```text
Generating 1000000 synthetic orders...

--------------- BENCHMARK RESULTS ---------------

Orders processed: 1000000
Accepted: 1000000
Rejected: 0

Elapsed time: 0.155 seconds
Throughput: 6471403 orders/second

--------------- LATENCY SAMPLES ---------------

Sample count: 1000

p50: 369.70 cycles
p95: 624.76 cycles
p99: 1135.46 cycles

--------------- ORDER BOOK STATE ---------------

Bid price levels: 54
Ask price levels: 63
```

### Binary Search

```text
Generating 1000000 synthetic orders...

--------------- BENCHMARK RESULTS ---------------

Orders processed: 1000000
Accepted: 1000000
Rejected: 0

Elapsed time: 0.128 seconds
Throughput: 7795691 orders/second

--------------- LATENCY SAMPLES ---------------

Sample count: 1000

p50: 333.68 cycles
p95: 512.34 cycles
p99: 713.90 cycles

--------------- ORDER BOOK STATE ---------------

Bid price levels: 54
Ask price levels: 63
```

### Performance Comparison

| Metric | Linear Search | Binary Search |
|---|---:|---:|
| Orders processed | 1,000,000 | 1,000,000 |
| Accepted | 1,000,000 | 1,000,000 |
| Rejected | 0 | 0 |
| Elapsed time | 0.155 s | 0.128 s |
| Throughput | 6,471,403 orders/s | 7,795,691 orders/s |
| p50 | 369.70 | 333.68 |
| p95 | 624.76 | 512.34 |
| p99 | 1,135.46 | 713.90 |
| Bid price levels | 54 | 54 |
| Ask price levels | 63 | 63 |

### Observations

The binary-search implementation achieved approximately **20.45% higher throughput** in these benchmark runs.

The measured latency percentiles also decreased:

- p50 decreased by approximately 9.74%.
- p95 decreased by approximately 18.00%.
- p99 decreased by approximately 37.12%.

These results suggest that binary search improves price-level lookup performance for this workload.

Both implementations processed the same one million synthetic orders, accepted all orders, rejected none, and finished with the same number of bid and ask price levels.

This provides a useful initial comparison between the two lookup strategies.

However, these measurements represent individual benchmark runs rather than a statistically controlled performance study. Multiple runs, alternating implementation order, and comparing medians would provide stronger evidence of the performance difference.

The benchmark also reflects a particular synthetic workload. Performance may differ with other order distributions, price-level counts, queue depths, and matching patterns.

## Building and Running

The project uses standard C++ and does not require third-party libraries.

### Build the Benchmark

Using GCC or MinGW:

```bash
g++ -O3 -msse2 -std=c++17 \
    FIFOQueue.cpp \
    PriceLevel.cpp \
    OrderBook.cpp \
    MatchingEngine.cpp \
    Benchmark.cpp \
    -o benchmark
```

Run:

```bash
./benchmark
```

On Windows:

```bash
benchmark.exe
```

The `-O3` flag enables compiler optimizations.

The `-msse2` flag enables SSE2 support for the x86 memory-fence intrinsic used by the benchmark.

### Build the Tests

```bash
g++ -O3 -std=c++17 \
    FIFOQueue.cpp \
    PriceLevel.cpp \
    OrderBook.cpp \
    MatchingEngine.cpp \
    Tests.cpp \
    -o tests
```

Run:

```bash
./tests
```

On Windows:

```bash
tests.exe
```

## Testing

The project includes a custom test suite that validates the behavior of the individual data structures and matching engine.

Tests cover areas such as:

- FIFO queue initialization.
- FIFO queue insertion and removal.
- FIFO ordering.
- FIFO queue resizing.
- Price-level initialization.
- Price-level aggregate quantity tracking.
- Partial order fills.
- Complete order removal.
- Empty price-level detection.
- Bid ordering.
- Ask ordering.
- Price-level lookup.
- Price-level insertion and removal.
- Best bid and best ask retrieval.
- Buy order matching.
- Sell order matching.
- Partial matching.
- Complete matching.
- Orders remaining in the book after matching.

The test suite is intended to verify both individual component behavior and interactions between components.

## Current Limitations

The current implementation focuses on the core mechanics of a single-instrument limit order book.

It does not currently include:

- Order cancellation by order ID.
- Market orders.
- Stop orders.
- Trade history or execution reports.
- Persistent storage.
- Network communication.
- Exchange connectivity.
- Multiple financial instruments.
- Multithreaded order processing.
- Risk management checks.
- Advanced order types.

The current order book also uses contiguous arrays for price levels, meaning insertion and removal can require O(n) element shifting.

The implementation is a learning and performance-engineering project rather than a production-ready exchange system.

## Potential Improvements

Several improvements could be explored in future versions.

### Data Structure Improvements

- Implement order cancellation using an order ID lookup structure.
- Explore hash maps for order ID lookup.
- Compare sorted arrays against balanced trees.
- Investigate alternative price-level storage strategies.
- Improve memory allocation using custom allocators or memory pools.
- Explore cache alignment and cache-aware layouts.

### Matching Engine Improvements

- Add market orders.
- Add order cancellation and modification.
- Implement trade execution records.
- Add support for multiple instruments.
- Improve error handling and transaction consistency.

### Performance Improvements

- Benchmark different order distributions.
- Run repeated benchmark trials and report medians.
- Compare additional data structures.
- Profile cache misses and branch prediction.
- Investigate memory allocation overhead.
- Explore batching and other performance optimizations.
- Investigate multithreaded architectures while preserving order-book correctness.

### Testing Improvements

- Add randomized stress tests.
- Add invariant checks after every operation.
- Run AddressSanitizer and UndefinedBehaviorSanitizer where supported.
- Test maximum queue and price-level capacities.
- Test edge cases involving partial fills and empty price levels.

## Project Goals

The main goals of this project are:

1. Understand how a limit order book operates.
2. Implement a matching engine that follows price-time priority.
3. Explore efficient data structures for order storage and price-level organization.
4. Understand the trade-offs between contiguous arrays, linked lists, and other data structures.
5. Apply binary search and other algorithmic optimizations.
6. Measure throughput and latency using a repeatable benchmark.
7. Develop good C++ practices around memory management, modular design, and testing.

The project demonstrates how fundamental data structure and algorithm choices can affect the performance of a matching engine.

---

**Built with C++**

A limit order book and matching engine focused on data structures, algorithmic efficiency, and performance optimization.