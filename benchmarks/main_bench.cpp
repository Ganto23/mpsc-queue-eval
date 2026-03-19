#include <benchmark/benchmark.h>
#include <thread>
#include <vector>
#include <atomic>
#include "core/QueueConcept.hpp"
#include "mutex/BasicMutexQueueAlignas.hpp"

template <typename Q>
void BM_MpscThroughput(benchmark::State& state) {
    const size_t num_producers = state.range(0);
    const size_t items_per_producer = 10000;
    const size_t total_expected = num_producers * items_per_producer;

    for (auto _ : state) {
        // 1. Setup: Everything must be reset inside the loop for every iteration
        Q queue; 
        std::atomic<bool> running{true};
        std::atomic<size_t> total_dequeued{0};

        // 2. Start Consumer
        std::thread consumer([&]() {
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(0, &cpuset);
            pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

            int value;
            // The consumer runs as long as producers are active OR items remain in the queue
            while (running.load(std::memory_order_relaxed) || total_dequeued < total_expected) {
                if (queue.try_dequeue(value)) {
                    benchmark::DoNotOptimize(value);
                    total_dequeued.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });

        // 3. Start Producers
        std::vector<std::thread> producers;
        for (size_t i = 0; i < num_producers; ++i) {
            producers.emplace_back([&, i]() {
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET((i % 3) + 1, &cpuset); 
                pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

                for (size_t j = 0; j < items_per_producer; ++j) {
                    // Spin-wait if the queue is full
                    while (!queue.try_enqueue(static_cast<int>(j))) {
                        std::this_thread::yield(); 
                    }
                }
            });
        }

        // 4. Cleanup: Join producers first
        for (auto& t : producers) {
            t.join();
        }

        // 5. Signal consumer to stop and join it
        running.store(false, std::memory_order_relaxed);
        if (consumer.joinable()) {
            consumer.join();
        }
    }

    // Report total throughput
    state.SetItemsProcessed(state.iterations() * total_expected);
}

// Register for 1, 2, and 4 producers (MPSC)
BENCHMARK_TEMPLATE(BM_MpscThroughput, BasicMutexQueueAlignas<int>)
    ->Arg(1)
    ->Arg(2)
    ->Arg(3) 
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();