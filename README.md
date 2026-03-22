# physics-engine-comparison
This project compares Object-Oriented Programming (OOP) and Entity Component System (ECS) approaches for simulating large numbers of objects, with a focus on performance optimizations such as SIMD, multithreading, and memory layout.

# Installation:
To run the program it is necessary to have:
- CMake
- pkg_config 
- SDL2 

These can be easily installed via brew:
```bash
brew install cmake
brew install pkg-config
brew install sdl2
```

After you cloned the repository to your device.

```bash
git clone <repo-url>
```

You just need to execute following commands:

```bash
cd physics-engine-comparison

mkdir build
cd build

cmake ..
make
```

Now you're ready to run the simulation.

# Usage:

To get additional data:

```bash
./simulation --help
```

To run program with OOP architecture:
```bash
./simulation oop 
```

# Simulation Overview

The simulation models independent circular particles with:

- Gravity
- Air resistance
- Elastic collisions with window boundaries
- No inter-object collisions

# Key Optimizations:

- AoS -> SoA transformation (cache-friendly memory layout)
- SIMD vectorization (ARM NEON)
- Multithreading (pthread)
- Memory alignment and prefetching
- Rendering batching (single draw call for all objects)

# Results:

## With rendering (100k objects, 1500 Frames):
- OOP: ~34s   
- ECS: ~33s
- ECS optimized: ~19s

## Without rendering (100k objects, 1500 Frames):
- OOP: ~0.37s 
- ECS: ~0.95s
- ECS optimized: ~1.60s

All results are averaged over 5 runs. Full benchmark data is available in the /benchmarks directory.

# Findings:

- Rendering dominates total runtime when enabled. The primary performance improvement (~2×) was achieved by batching draw calls (reducing ~100k draw calls per frame to a single call).
- ECS introduces additional overhead (indirection, memory access patterns, and abstraction), which makes it slower than a simple OOP loop for relatively small workloads.
- Multithreading does not improve performance in this setup because the work per frame is too small, and synchronization overhead outweighs the benefits.
- SIMD and memory optimizations provide limited gains because the simulation is memory-bandwidth bound rather than compute-bound.
- High-level optimizations (such as reducing draw calls) had a significantly greater impact than low-level optimizations like SIMD or multithreading.

# Conclusion:
The most impactful optimization was reducing rendering overhead through batching. While ECS, SIMD, and multithreading are powerful techniques, their benefits depend heavily on workload size, memory access patterns, and system bottlenecks. For smaller workloads, simpler OOP implementations can outperform more complex architectures due to lower overhead.
