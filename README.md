# Nexus-HFT

**Nexus-HFT** is a modern, ultra-low-latency High-Frequency Trading (HFT) infrastructure platform written in **C++20**. It is designed from the ground up to minimize latency on the critical path by utilizing zero-allocation data structures, lock-free concurrency, memory-mapped persistence, and hardware-aware thread affinity.

This project was built to demonstrate advanced systems programming techniques and quantitative market-making concepts suitable for Tier-1 proprietary trading firms and quantitative hedge funds.

---

## 🚀 Key Features & Architecture

### 1. Limit Order Book & Matching Engine (Zero Allocation)
- **O(1) Memory Pool**: Utilizes a pre-allocated contiguous memory arena (`MemoryPool<T>`) to instantiate new orders. The pool now uses an **intrusive singly-linked freelist** — the next-pointer is stored inside the free slot's own raw bytes, eliminating the secondary bookkeeping allocation entirely. Allocate and deallocate are pure pointer operations (~3–5 ns).
- **SmallVector\<Trade, 16\>**: The matching engine returns trades via a `SmallVector` — a stack-first, heap-fallback container that stores up to 16 trades inline without any heap allocation. For ≥99.99% of real orders this means **zero `malloc()` calls** on the hot path. The container only falls back to the heap if a single order generates more than 16 simultaneous trades.
- **Pool-Backed Order Book**: Price levels in the Limit Order Book are managed by a `PoolAllocator<T>` — an STL-compatible stateful allocator backed by two pre-allocated 128 KB arenas (one per side) embedded directly inside the `OrderBook` object. `std::map` node creation and deletion never touch the OS memory manager during trading hours.
- **Intrusive Linked Lists**: Bids and Asks at each price level are tracked using custom intrusive doubly-linked lists (`IntrusiveList`) to maximize cache locality and minimize pointer chasing.
- **Price-Time Priority**: Fully compliant FIFO matching engine supporting limit orders, market orders, and partial fills.

### 2. Lock-Free Pipeline & Thread Affinity
- **SPSC Ring Buffer**: Inter-thread communication is handled via a custom Single-Producer Single-Consumer queue using `std::atomic`.
- **Cache-Line Padding**: Prevents false sharing between producer and consumer threads by padding atomic indices to the hardware destructive interference size (`alignas(64)`).
- **CPU Core Pinning**: Cross-platform thread affinity utilities ensure hot-path threads (Matching Engine, Networking) are pinned to isolated logical cores, avoiding OS context switching.

### 3. Sub-Microsecond Pre-Trade Risk & OMS
- **Inline Risk Engine**: Validates price collars, max order size (fat-finger prevention), and max notional values with zero allocations and minimal branching.
- **Lock-Free Kill Switch**: A global `std::atomic<bool>` kill switch utilizing relaxed memory semantics to halt the entire system instantly without penalizing standard execution latency.
- **Real-Time Portfolio**: Tracks net position, gross notional, and computes realized/unrealized PnL tick-by-tick.

### 4. Quantitative Signals & Market Making
- **O(1) Rolling Statistics**: Utilizes an adapted Welford's online algorithm to compute rolling VWAP and variance/volatility in constant time, entirely avoiding sliding window loops.
- **Microstructure Signals**: Calculates Volume-Weighted Micro-Price and Order Flow Imbalance (OFI) to predict short-term midpoint drift.
- **Avellaneda-Stoikov Model**: Implements dynamic inventory-based quoting. Automatically skews the reservation price and asymmetric bid/ask spreads to minimize inventory risk while capturing the spread.

### 5. Hardware-Accurate Latency Measurement
- **RDTSC Cycle Counter**: Hot-path latency is measured using the `rdtsc` / `rdtscp` CPU instruction (~7–20 cycles overhead) rather than `std::chrono::high_resolution_clock` (which bottoms out to a syscall costing ~100–400 cycles). A one-time `RdtscCalibration` runs at startup to convert raw cycle counts to nanoseconds using `QueryPerformanceCounter` (Windows) or `CLOCK_MONOTONIC` (Linux).

### 6. Networking & Persistence
- **Zero-Copy Binary Protocol**: Structs are defined with `#pragma pack(push, 1)` to allow direct casting of TCP socket buffers into C++ objects without parsing overhead.
- **Boost.Asio Gateway**: Asynchronous TCP server for order ingress.
- **Memory-Mapped WAL**: Write-Ahead Log utilizing OS-level `mmap` / `CreateFileMapping` to persist orders to disk at the speed of a RAM `memcpy`, allowing instant crash recovery without blocking the trading thread.

---

## ⚡ Measured Latency

All numbers measured on the live engine using RDTSC over 200 real orders (mix of resting inserts and crossing orders that generate trades).

| Metric | Latency |
|--------|---------|
| **Min** | 33 ns |
| **Median (P50)** | 78 ns |
| **Mean** | 93 ns |
| **P99** | 432 ns |
| **Max spike** | 617 ns |

> Median latency is bounded by L1/L2 cache access speed — the engine generates **zero OS allocator calls** on the normal trading path.

---

## 🛠️ Tech Stack & Requirements

- **Language**: C++20
- **Build System**: CMake (3.20+)
- **Testing Framework**: GoogleTest (gtest)
- **Benchmarking**: Google Benchmark
- **Networking**: Boost.Asio

### Supported Platforms
- Linux (Ubuntu/Debian) via GCC 10+ or Clang 11+
- Windows (MSVC 14.29+ with C++20 enabled)

---

## 🏗️ Build Instructions

1. **Clone the repository:**
   ```bash
   git clone https://github.com/udit-paul/Nexus-HFT.git
   cd Nexus-HFT
   ```

2. **Configure CMake:**
   ```bash
   mkdir build && cd build
   cmake ..
   ```

3. **Build the project:**
   ```bash
   cmake --build . --config Release
   ```

4. **Run Unit Tests:**
   ```bash
   ctest --output-on-failure
   ```

5. **Run Benchmarks:**
   ```bash
   # SPSC Queue throughput
   ./benchmarks/bench_spsc_queue

   # Matching Engine latency (resting insert, single trade, 5-level sweep)
   ./benchmarks/bench_matching_engine
   ```

---

## 🌐 Web Dashboard

The project includes a web dashboard to visualize the trading engine in real time — live order book depth, trade feed, and per-order latency display.

1. **Start the Backend Server (Middleware):**
   ```bash
   cd web/middleware
   node server.js
   ```
   *Runs on port 3001 by default.*

2. **Start the Frontend Server (Vite React App):**
   ```bash
   cd web/frontend
   npm run dev
   ```
   *Runs on port 5173 by default. Accessible at `http://localhost:5173`.*

**Troubleshooting:**
If a port is already in use, you can find the process ID (PID) and kill it (Windows):
```bash
netstat -ano | findstr :<PORT>
taskkill /PID <PID> /F
```

For more detailed instructions, see [STARTING_THE_WEB_SERVER.md](STARTING_THE_WEB_SERVER.md).

---

## 📊 Project Structure

```text
Nexus-HFT/
├── CMakeLists.txt
├── include/nexus/
│   ├── core/         # MemoryPool, SmallVector, PoolAllocator, RDTSC, IntrusiveList, Types
│   ├── engine/       # Limit Order Book, Matching Engine
│   ├── concurrency/  # Lock-Free SPSC Queue, Thread Affinity
│   ├── risk/         # Pre-trade Risk Limits, Kill Switch
│   ├── oms/          # Order Tracker, Portfolio PnL
│   ├── quant/        # Rolling Stats, OFI, Avellaneda-Stoikov
│   ├── network/      # Binary Protocol, TCP Gateway, Multicast
│   └── persistence/  # Memory-Mapped Write-Ahead Log
├── src/              # C++ implementations for the above
├── tests/            # GoogleTest suites covering all modules
└── benchmarks/       # Google Benchmark — SPSC queue & matching engine
```

---

## 💡 Motivation
This project was developed to showcase deep expertise in C++ systems engineering, low-latency architecture, and quantitative finance. It demonstrates a holistic understanding of how a modern HFT firm operates—from socket ingestion and memory-mapped disk I/O, down to CPU cache optimization and inventory risk management.

---
*Disclaimer: This is an educational infrastructure blueprint and does not connect to live financial exchanges.*
