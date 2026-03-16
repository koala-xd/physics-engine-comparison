#include <string>
#include <iostream>
#include <SDL2/SDL.h>
#include "../include/ecs_optimized.h"
#include "../include/ecs_simulation.h"
#include "../include/oop_simulation.h"

void print_help() {
	std::cout << "Usage: simulation [MODE]\n\n";
    std::cout << "Modes:\n";
    std::cout << "  oop            Run OOP simulation\n";
    std::cout << "  ecs            Run ECS simulation\n";
    std::cout << "  ecs_optimized  Run optimized ECS simulation\n\n";
    std::cout << "Options:\n";
    std::cout << "  --help         Show this message\n";
    std::cout << "  --benchmark    Turn on benchmark mode\n";
}

int main(int argc, char** argv)
{
	if(argc < 2)
    {
        std::cout << "No mode specified. Use --help\n";
        return 0;
    }

	std::string arg = argv[1];

    if(arg == "--help")
    {
        print_help();
        return 0;
    }
    else if(arg == "oop")
    {
        run_oop_simulation();
    }
    else if(arg == "ecs")
    {
        run_ecs_simulation();
    }
    else if(arg == "ecs_optimized")
    {
        run_ecs_optimized();
    }
    else
    {
        std::cout << "Unknown mode. Use --help\n";
    }
	return 0;
}
