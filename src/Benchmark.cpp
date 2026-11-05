#include "MatchingEngine.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <x86intrin.h>


static constexpr uint64_t ORDER_COUNT = 1'000'000;
static constexpr uint64_t LATENCY_SAMPLE_INTERVAL = 1000;
static constexpr uint64_t LATENCY_BATCH_SIZE = 100;

// Deterministic xorshift pseudo-random generator
class Random{
private:
    uint64_t state;
public: 
    explicit Random(uint64_t seed) : state(seed){}
    uint32_t Next(){
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        return static_cast<uint32_t>(state >> 32);
    }
    uint32_t Range(uint32_t min, uint32_t max){
        return min + Next() % (max - min + 1);
    }
};

static std::vector<Order> GenerateOrders(uint64_t count){
    std::vector<Order> orders;
    orders.reserve(count);
    Random random(123456789);
    for (uint64_t i = 0; i < count; i++){
        Order order{};
        order.id = i + 1;
        order.time = i + 1;
        // prices clustered around a midpoint
        order.price = random.Range(9900, 10100);
        // random order quantities
        order.quantity = random.Range(1, 100);
        // approximately equal buy/sell volume by order count
        order.side = (random.Next() % 2 == 0) ? BUY : SELL;
        orders.push_back(order);
    }
    return orders;
}

static double Percentile(std::vector<double>& values, double percentile){
    if (values.empty()){return 0.0;}
    std::sort(values.begin(), values.end());
    size_t index = static_cast<size_t>(percentile * (values.size() - 1));
    return values[index];
}

static inline uint64_t ReadStartCycles(){
    _mm_lfence();
    unsigned int aux;
    return __rdtscp(&aux);
}

static inline uint64_t ReadEndCycles(){
    unsigned int aux;
    uint64_t cycles = __rdtscp(&aux);
    _mm_lfence();
    return cycles;
}

int main(int argc, char** argv){

    printf("Generating %llu synthetic orders...\n", static_cast<unsigned long long>(ORDER_COUNT));
    // generate outside the timed section
    std::vector<Order> orders = GenerateOrders(ORDER_COUNT);

    OrderBook orderBook{};
    if (!OrderBookInitialize(&orderBook)){
        printf("Failed to initialize orderBook\n");
        return 1;
    }

    std::vector<double> latencySamples;
    latencySamples.reserve(ORDER_COUNT / LATENCY_SAMPLE_INTERVAL);
    uint64_t accepted = 0;
    uint64_t rejected = 0;
    using Clock = std::chrono::steady_clock;
    auto benchmarkStart = Clock::now();
    for (uint64_t i = 0; i < ORDER_COUNT; ){
        bool sampleLatency = (i % LATENCY_SAMPLE_INTERVAL == 0) && (i + LATENCY_BATCH_SIZE <= ORDER_COUNT);
        if (sampleLatency){
            uint64_t startCycles = ReadStartCycles();
            for (uint64_t j = 0; j < LATENCY_BATCH_SIZE; j++){
                bool success = MatchingEngineProcessOrder(&orderBook, &orders[i + j]);
                if (success){accepted++;}
                else{rejected++;}
            }
            uint64_t endCycles = ReadEndCycles();
            latencySamples.push_back(static_cast<double>(endCycles - startCycles) / LATENCY_BATCH_SIZE);
            i += LATENCY_BATCH_SIZE;
        }
        else{
            bool success = MatchingEngineProcessOrder(&orderBook, &orders[i]);
            if (success){accepted++;}
            else{rejected++;}
            i++;
        }
    }

    auto benchmarkEnd = Clock::now();
    double elapsedSeconds = std::chrono::duration<double>(benchmarkEnd - benchmarkStart).count();
    double throughput = ORDER_COUNT / elapsedSeconds;
    printf("---------------BENCHMARK RESULTS---------------\n");

    printf("Orders processed: %llu\n", static_cast<unsigned long long>(ORDER_COUNT));

    printf("Accepted: %llu\n", static_cast<unsigned long long>(accepted));

    printf("Rejected: %llu\n", static_cast<unsigned long long>(rejected));

    printf("Elapsed time: %.3f seconds\n", elapsedSeconds);

    printf("Throughput: %.0f orders/second\n", throughput);

    printf("\n---------------LATENCY SAMPLES---------------\n");

    printf("Sample count: %zu\n", latencySamples.size());

    printf("p50: %.2f cycles\n", Percentile(latencySamples, 0.50));

    printf("p95: %.2f cycles\n", Percentile(latencySamples, 0.95));

    printf("p99: %.2f cycles\n", Percentile(latencySamples, 0.99));

    printf("\n---------------ORDER BOOK STATE---------------\n");

    printf("Bid price levels: %u\n", orderBook.bidsCount);
    printf("Ask price levels: %u\n", orderBook.asksCount);

    OrderBookDestroy(&orderBook);

    return rejected == 0 ? 0 : 1;
}

/*
Compile:
g++ -O3 -msse2 -std=c++17 FIFOQueue.cpp PriceLevel.cpp OrderBook.cpp MatchingEngine.cpp Benchmark.cpp -o benchmark
Run:
benchmark.exe
*/


/*
---------------------------------------------------------------------------
---------------RESULTS Linear Search for OrderBook PriceLevels:---------------
---------------------------------------------------------------------------

Generating 1000000 synthetic orders...
---------------BENCHMARK RESULTS---------------
Orders processed: 1000000
Accepted: 1000000
Rejected: 0
Elapsed time: 0.155 seconds
Throughput: 6471403 orders/second

---------------LATENCY SAMPLES---------------
Sample count: 1000
p50: 369.70 cycles
p95: 624.76 cycles
p99: 1135.46 cycles

---------------ORDER BOOK STATE---------------
Bid price levels: 54
Ask price levels: 63


---------------------------------------------------------------------------
---------------RESULTS Binary Search for OrderBook PriceLevels:---------------
---------------------------------------------------------------------------

Generating 1000000 synthetic orders...
---------------BENCHMARK RESULTS---------------
Orders processed: 1000000
Accepted: 1000000
Rejected: 0
Elapsed time: 0.128 seconds
Throughput: 7795691 orders/second

---------------LATENCY SAMPLES---------------
Sample count: 1000
p50: 333.68 cycles
p95: 512.34 cycles
p99: 713.90 cycles

---------------ORDER BOOK STATE---------------
Bid price levels: 54
Ask price levels: 63


*/