#!/bin/bash
set -e

echo "Building benchmarks..."
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
cd ..

mkdir -p results

echo "Running Google Benchmark (Throughput & Latency)..."
# Outputs primary throughput metrics to JSON for easy Python parsing
./build/mpsc_bench --benchmark_format=json > results/throughput.json

# Move the generated latency CSVs into the results folder
mv BM_Queue_Throughput*_latencies.csv results/ 2>/dev/null || true

echo "Running Hardware Counters with perf stat (Pinned to Cores 0-3)..."
# We use taskset -c 0-3 to pin the process to the 4 cores. 
# perf stat captures cache misses and branch misses.
perf stat -e L1-dcache-load-misses,L1-dcache-loads,branch-misses,branches -x , -o results/hw_counters.csv taskset -c 2,3 ./build/mpsc_bench --benchmark_filter=".*"
echo "Benchmarking complete. Triggering Python visualizer..."
python3 scripts/plot_results.py