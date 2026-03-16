cmake_minimum_required(VERSION 3.15)
project(BouncingBall)

add_executable(BouncingBallProd)

#set(CMAKE_PREFIX_PATH /path/to/dependency /another/path/to/dependency)
#list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/sdl2")

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

target_sources(BouncingBallProd
	PRIVATE ../src/bouncing_ball_mt.cc
	PUBLIC ../src/arena.c
)

# Compiler flag optimizaitons
target_compile_options(BouncingBallProd PRIVATE -O3 -Wall -Wextra)

# Linker flags
target_link_options(BouncingBallProd PRIVATE -pthread)


# including SDL2
target_include_directories(BouncingBallProd PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_directories(BouncingBallProd PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(
	BouncingBallProd 
	PRIVATE ${SDL2_LIBRARIES} 
)

