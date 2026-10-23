# C Virtual Memory Simulator

A UTSA Computer Science coursework project in C. The program translates logical addresses through a page table, loads 256-byte pages from a backing store, tracks 128 physical frames, applies least-recently-used replacement, and reports page faults.

## Student contribution and starter materials

I wrote most of the substantive implementation. The recovered project directory contains the implementation and CMake file, but no separate instructor source or starter file. The retained records do not support a line-by-line attribution claim. Some coursework used instructor-provided starter materials or templates; no such standalone material is included here.

## Concepts and structure

- main.c implements address translation, page-fault handling, LRU selection, and page-table snapshots.
- CMakeLists.txt describes the C11 executable target.
- The page number and offset are derived from each logical address; page misses load a 256-byte page into a free or least-recently-used frame.

## Build and limitations

CMake 4.2 or later is specified by the project file. From this directory:

    cmake -S . -B build
    cmake --build build

These commands have not been run for this release review. Runtime requires addresses.txt and BACKING_STORE.bin; both course-associated fixtures are excluded. Supply independently authorized fixtures before running the program.

The code assumes input addresses are within the 0–65535 range and does not validate that bound. Empty address input also makes the reported fault-rate division invalid. This is an educational simulator, not an operating-system kernel. No open-source license is included.

## Exclusions

Course input fixtures, output snapshots, generated files, and IDE build output are not included.
