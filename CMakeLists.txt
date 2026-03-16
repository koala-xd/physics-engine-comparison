cmake_minimum_required(VERSION 3.15)
project(physics-engine-simulation)

add_executable(simulation)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

target_sources(simulation
	PRIVATE ../src/main.cc ../src/ecs_optimized.cc ../src/ecs_simulation.cc ../src/oop_simulation.cc
	PUBLIC ../src/arena.c ../src/sparse_set.cc
)

# Compiler flag optimizaitons
target_compile_options(simulation PRIVATE -O3 -Wall -Wextra)

# Linker flags
target_link_options(simulation PRIVATE -pthread)


# including SDL2
target_include_directories(simulation PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_directories(simulation PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(
	simulation 
	PRIVATE ${SDL2_LIBRARIES} 
)

