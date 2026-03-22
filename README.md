# physics-engine-comparison
This project compares Object-Oriented Programming (OOP) and Entity Component System (ECS) approaches for simulating large numbers of objects, with a focus on performance optimizations such as SIMD, multithreading, and memory layout.

# Simulation Overview

- 2D particle simulation (circles)
- Gravity and air resistance
- Elastic collisions with window boundaries
- No inter-object collisions (independent objects)

# Key Optimizations:

- AoS -> SoA transformation (cache-friendly memory layout)
- SIMD vectorization (ARM NEON)
- Multithreading (pthread)
- Memory alignment and prefetching
- Rendering batching (single draw call for all objects)

# Results:

## With rendering (100k objects, 1500 Frames):
- OOP: ~61s   
- ECS: ~60s
- ECS optimized: ~36s

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

# Conclusion:
The most impactful optimization was reducing rendering overhead through batching. While ECS, SIMD, and multithreading are powerful techniques, their benefits depend heavily on workload size, memory access patterns, and system bottlenecks. For smaller workloads, simpler OOP implementations can outperform more complex architectures due to lower overhead.
