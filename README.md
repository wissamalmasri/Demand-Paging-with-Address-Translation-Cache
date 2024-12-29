Author
- Wissam Almasri

Virtual Memory Paging Simulation
This project simulates a virtual memory paging system, incorporating multi-level page tables, Translation Lo
Features
- Multi-level page table implementation with configurable bitmasks.
- TLB implementation with Least Recently Used (LRU) replacement policy.
- Detailed logging of address translation, page table usage, and performance metrics.

Files
- `main.cpp`: Entry point for the program, processes command-line arguments and memory trace files.
- `pageTableLevel.h` and `pageTableLevel.cpp`: Implements the page table and TLB functionality.
- `tracereader.h` and `tracereader.cpp`: Handles reading and decoding of memory trace files.
- `log.h` and `log.cpp`: Provides utilities for logging translations, page mappings, and summaries.
- `Makefile`: Automates the build process.

Usage
Prerequisites
- A C++ compiler supporting C++11 or later.
- A memory trace file in the specified format.

Compilation
Run the following command to compile the program:
```bash
make
```
Running the Program
Execute the program using:
```bash
./pagingwithatc [-n N] [-c N] [-o mode] <tracefile> <levelbits>
```

Options:
- `-n N`: Number of memory accesses to process (default: all).
- `-c N`: TLB capacity (default: 0, no TLB).
- `-o mode`: Output mode (`summary`, `bitmasks`, `offset`, `vpn2pfn`, or `va2pa`).
- `<tracefile>`: Path to the memory trace file.
- `<levelbits>`: Space-separated bit sizes for each level of the page table.
### Example
```bash
./pagingwithatc -n 100 -c 64 -o va2pa trace.bin 10 10 12
```

Output
Depending on the selected output mode, the program logs:
1. Address translations (virtual to physical).
2. Page table entries and mappings.
3. Hit/miss statistics and summary of performance.
Code Overview

Key Components
- **PageTable**: Represents the hierarchical page table structure.
- **TLB**: Implements a cache for frequently accessed page table entries using LRU.
- **Level**: Handles individual levels of the multi-level page table.

Logging
Logs include details such as:
- TLB hits and misses.
- Page table lookups and updates.
- Overall performance metrics.

License
This project is licensed under the MIT License. See the LICENSE file for details.
