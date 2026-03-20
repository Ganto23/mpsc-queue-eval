#include <benchmark/benchmark.h>
#include <thread>
#include <vector>
#include <fstream>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <filesystem>

// FAA Family
#include "../include/faa/FaaTicketQueue.hpp"
#include "../include/faa/FaaTicketQueueCompilerHints.hpp"
#include "../include/faa/FaaTicketQueueAlignas.hpp"
// Basic CAS Family
#include "../include/cas/BasicAtomicQueue.hpp"
#include "../include/cas/BasicAtomicQueueCompilerHints.hpp"
#include "../include/cas/BasicAtomicQueueAlignas.hpp"
// Vyukov CAS Family
#include "../include/cas/VyukovStateMachine.hpp"
#include "../include/cas/VyukovStateMachineCompilerHints.hpp"
#include "../include/cas/VyukovStateMachineAlignas.hpp"
// Mutex Family
#include "../include/mutex/BasicMutexQueue.hpp"
#include "../include/mutex/BasicMutexQueueCompilerHints.hpp"
#include "../include/mutex/BasicMutexQueueAlignas.hpp"
// Spinlock Family
#include "../include/mutex/SpinlockQueue.hpp"
#include "../include/mutex/SpinlockQueueCompilerHints.hpp"
#include "../include/mutex/SpinlockQueueAlignas.hpp"

// Benchmark constants - 500k is a good "fast" run size
const int NUM_ITEMS = 500000;
const size_t QUEUE_CAPACITY = 65536; 
using DataT = int;

inline uint64_t get_time_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

template <typename Q>
void BM_MPSC_Performance(benchmark::State& state) {
    int num_producers = state.range(0);
    // Ensure total items is divisible by num_producers for clean math
    int items_per_producer = NUM_ITEMS / num_producers;
    int total_to_consume = items_per_producer * num_producers;
    
    std::vector<uint64_t> latencies(items_per_producer, 0);

    for (auto _ : state) {
        Q queue;
        std::atomic<bool> start_flag{false};
        std::atomic<int> items_consumed{0};
        
        std::vector<std::thread> producers;
        for (int i = 0; i < num_producers; ++i) {
            producers.emplace_back([&, i]() {
                // Wait for the start signal
                while (!start_flag.load(std::memory_order_acquire)) {
                    std::this_thread::yield();
                }

                for (int j = 0; j < items_per_producer; ++j) {
                    uint64_t s = (i == 0) ? get_time_ns() : 0;
                    
                    // RETRY LOOP: Keep trying until enqueue succeeds
                    while (!queue.try_enqueue(static_cast<DataT>(j))) {
                        asm volatile("yield" ::: "memory");
                    }
                    
                    if (i == 0) latencies[j] = get_time_ns() - s;
                }
            });
        }

        std::thread consumer([&]() {
            DataT dummy;
            // Wait for the start signal
            while (!start_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            while (items_consumed < total_to_consume) {
                // RETRY LOOP: Keep trying until dequeue succeeds
                if (queue.try_dequeue(dummy)) {
                    items_consumed++;
                } else {
                    asm volatile("yield" ::: "memory");
                }
            }
        });

        // Fire the starting pistol
        start_flag.store(true, std::memory_order_release);

        for (auto& t : producers) t.join();
        consumer.join();
    }
    
    state.SetItemsProcessed(state.iterations() * total_to_consume);

    // File writing logic
    std::filesystem::create_directories("results");
    std::string q_name = state.name();
    std::string illegal_chars = "<>:, /\"|?*";
    for (char c : illegal_chars) {
        std::replace(q_name.begin(), q_name.end(), c, '_');
    }

    std::ofstream out("results/" + q_name + "_latencies.csv");
    if (out.is_open()) {
        for (const auto& lat : latencies) out << lat << "\n";
    }
}

template<typename ConcreteQueueType>
void do_reg(const std::string& name) {
    benchmark::RegisterBenchmark(name, BM_MPSC_Performance<ConcreteQueueType>)
        ->Args({1})->Args({2})->Args({4})->UseRealTime();
}

int dummy_registration = []() {
    // FAA
    do_reg<FaaTicketQueue<DataT, QUEUE_CAPACITY>>("FaaTicketQueue");
    do_reg<FaaTicketQueueCompilerHints<DataT, QUEUE_CAPACITY>>("FaaTicketQueueCompilerHints");
    do_reg<FaaTicketQueueAlignas<DataT, QUEUE_CAPACITY>>("FaaTicketQueueAlignas");

    // Basic Atomic
    do_reg<BasicAtomicQueue<DataT, QUEUE_CAPACITY>>("BasicAtomicQueue");
    do_reg<BasicAtomicQueueCompilerHints<DataT, QUEUE_CAPACITY>>("BasicAtomicQueueCompilerHints");
    do_reg<BasicAtomicQueueAlignas<DataT, QUEUE_CAPACITY>>("BasicAtomicQueueAlignas");

    // Vyukov
    do_reg<VyukovStateMachine<DataT, QUEUE_CAPACITY>>("VyukovStateMachine");
    do_reg<VyukovStateMachineCompilerHints<DataT, QUEUE_CAPACITY>>("VyukovStateMachineCompilerHints");
    do_reg<VyukovStateMachineAlignas<DataT, QUEUE_CAPACITY>>("VyukovStateMachineAlignas");

    // Mutex
    do_reg<BasicMutexQueue<DataT, QUEUE_CAPACITY>>("BasicMutexQueue");
    do_reg<BasicMutexQueueCompilerHints<DataT, QUEUE_CAPACITY>>("BasicMutexQueueCompilerHints");
    do_reg<BasicMutexQueueAlignas<DataT, QUEUE_CAPACITY>>("BasicMutexQueueAlignas");

    // Spinlock
    do_reg<SpinlockQueue<DataT, QUEUE_CAPACITY>>("SpinlockQueue");
    do_reg<SpinlockQueueCompilerHints<DataT, QUEUE_CAPACITY>>("SpinlockQueueCompilerHints");
    do_reg<SpinlockQueueAlignas<DataT, QUEUE_CAPACITY>>("SpinlockQueueAlignas");

    return 0;
}();

int main(int argc, char** argv) {
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}