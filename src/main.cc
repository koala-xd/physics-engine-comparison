#include <string>
#include <iostream>
#include <SDL2/SDL.h>
#include "../include/ecs_optimized.h"
#include "../include/ecs_simulation.h"
#include "../include/oop_simulation.h"

const static int FRAMES_COUNT = 1500;

void print_help() {
	std::cout << "Usage: simulation [MODE] [OBJECTS AMOUNT] [BENCHMARK MODE]\n\n";
    std::cout << "Modes:\n";
    std::cout << "  oop            Run OOP simulation\n";
    std::cout << "  ecs            Run ECS simulation\n";
    std::cout << "  ecs_optimized  Run optimized ECS simulation\n\n";
    std::cout << "Options:\n";
    std::cout << "  --help         Show this message\n";
    std::cout << "  --benchmark    Turn on benchmark mode\n";
}

void print_comment(std::string mode, int amount_of_objects) {
	std::cout << "Mode: " << mode << "\n";
	std::cout << "Objects: " << amount_of_objects << "\n";;
	std::cout << "Frames: " << FRAMES_COUNT << "\n";
}


int main(int argc, char** argv)
{
	if(argc < 2)
    {
        std::cout << "No mode specified. Use --help\n";
        return 0;
    }
	std::string arg = argv[1];
	int amount_of_objects = 1e5;
	if(argc >= 3) {
		if(atoi(argv[2]) != 0) {
			amount_of_objects = atoi(argv[2]);
		}	
	}

	int benchmark_mode = 0; 
	if(strcmp(argv[argc-1], "--benchmark") == 0) {
		benchmark_mode = 1;
	}

    if(arg == "--help")
    {
        print_help();
        return 0;
    }
    else if(arg == "oop")
    {
		print_comment(arg, amount_of_objects);
        run_oop_simulation(amount_of_objects, benchmark_mode, FRAMES_COUNT);
    }
    else if(arg == "ecs")
    {
		print_comment(arg, amount_of_objects);
        run_ecs_simulation(amount_of_objects, benchmark_mode, FRAMES_COUNT);
    }
    else if(arg == "ecs_optimized")
    {
		print_comment(arg, amount_of_objects);
        run_ecs_optimized(amount_of_objects, benchmark_mode, FRAMES_COUNT);
    }
    else
    {
        std::cout << "Unknown mode. Use --help\n";
    }
	return 0;
}
