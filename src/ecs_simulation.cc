#include <iostream>
#include <SDL2/SDL.h>
#include "../include/arena.h"
#include "../include/sparse_set.h"

using namespace std;

/////////////////////////////////////////////////////////////////////////////////////////////
// const 

const static uint32_t WINDOW_WIDTH = 1000;
const static uint32_t WINDOW_HEIGHT = 800;

const uint32_t points_size = (10 * 8 * 35 / 49 + (8 - 1)) & -8;
SDL_Point points[points_size]; 

/////////////////////////////////////////////////////////////////////////////////////////////
// Functions

void compute_circle(SDL_Point* points, int32_t centreX, int32_t centreY, int32_t radius, size_t points_size) {

    int32_t x = (radius - 1);
    int32_t y = 0;
    int32_t tx = 1;
    int32_t ty = 1;
    int32_t error = (tx - (radius << 1));

	int counter = 0;
    while (x >= y) {
        // Each of the following renders an octant of the circle
		
		points[counter] = {centreX + x, centreY - y};	
		points[counter + 1] = {centreX + x, centreY + y};	

		points[counter + 2] = {centreX - x, centreY - y};	
		points[counter + 3] = {centreX - x, centreY + y};	

		points[counter + 4] = {centreX + y, centreY - x};	
		points[counter + 5] = {centreX + y, centreY + x};	

		points[counter + 6] = {centreX - y, centreY - x};	
		points[counter + 7] = {centreX - y, centreY + x};	

        if (error <= 0) {
            ++y;
            error += ty;
            ty += 2;
        }

        if (error > 0) {
            --x;
            tx += 2;
            error += (tx - (radius << 1));
        }
		counter += 8;
    }
}

void DrawCircle(SDL_Renderer* renderer, SDL_Elipse *elipse)
{
	compute_circle(points, elipse->x, elipse->y, elipse->h, points_size);
	SDL_RenderDrawPoints(renderer, points, points_size);
}

bool draw_objects(entity_id* object_ids, size_t capacity, elipse_sset_d* eset, speed_sset_d* sset, SDL_Window* sdl_window, SDL_Renderer* renderer, int benchmark_mode)
{
    SDL_Event event;
    while (!benchmark_mode && SDL_PollEvent(&event)) {
        switch (event.type) {
			case SDL_QUIT:
				return false;
        }
    }
	
	if(!benchmark_mode) {
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderClear(renderer);

		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	}
	
	// Y-axis collision
    for (int i = 0; i < eset->count; ++i) {
		float y_pos = eset->data[i].y;
		float half_h = eset->data[i].h / 2.0f;
        if ((y_pos + half_h >= WINDOW_HEIGHT) || (y_pos - half_h <= 0)) {
            sset->data[i].y_speed = -sset->data[i].y_speed;
        }
    }

	// X-axis collision
	for (int i = 0; i < eset->count; ++i) {
		float x_pos = eset->data[i].x;
		float half_h = eset->data[i].h / 2.0f;
        if (x_pos + half_h >= WINDOW_WIDTH || x_pos - half_h <= 0) {
            sset->data[i].x_speed = -sset->data[i].x_speed;
        }
	}

	for (size_t i = 0; i < eset->count; ++i) {
		sset->data[i].y_speed *= 0.99f;
		sset->data[i].x_speed *= 0.99f;
		
		eset->data[i].y += sset->data[i].y_speed;
		eset->data[i].x += sset->data[i].x_speed;
	}

	for(int i = 0; !benchmark_mode && i < eset->count; ++i) {
		DrawCircle(renderer, &eset->data[i]);
	}

	if(!benchmark_mode)
		SDL_RenderPresent(renderer);
    return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////
// main function

int run_ecs_simulation(int amount_of_objects, int benchmark_mode, int frames_count)
{
	static uint32_t objects_amount = amount_of_objects;
	static int FRAMES_COUNT = frames_count;
	SDL_Window* sdl_window = NULL;
	SDL_Renderer* renderer = NULL;
	
	if(!benchmark_mode) {
		SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0");
		SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

		if (SDL_Init(SDL_INIT_VIDEO) < 0) {
			fprintf(stderr, "SDL could not be initialized!\nSDL_Error:", SDL_GetError());
			return 1;
		}

		sdl_window = SDL_CreateWindow("Simulation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
		if (sdl_window == NULL) {
			fprintf(stderr, "SDL Window could not be created!\nSDL_Error:", SDL_GetError());
			SDL_Quit();
			return 1;
		}

		renderer = SDL_CreateRenderer(sdl_window, -1, 0);
		if (!renderer) {
			fprintf(stderr, "SDL Renderer could not be created!\nSDL_Error:", SDL_GetError());
			SDL_Quit();
			return 1;
		}
	}

	Arena* level_arena = ArenaAlloc((sizeof(elipse_sset_d) + sizeof(speed_sset_d) + sizeof(entity_manager)) * (objects_amount + 1)); 
	elipse_sset_d* eset = PushStruct(level_arena, elipse_sset_d);
	eset = elipse_sset_d_init(eset, level_arena, objects_amount);

	speed_sset_d* sset = PushStruct(level_arena, speed_sset_d);
	sset = speed_sset_d_init(sset, level_arena, objects_amount);

	entity_manager* em = PushStruct(level_arena, entity_manager);
	em = entity_manager_init(em, level_arena, objects_amount);

	entity_id object_ids[10];

    for (int i = 0; i < objects_amount; ++i) {
		entity_id entity_id = create_id(em);
		float x = (rand() % WINDOW_WIDTH);
		float y = (rand() % WINDOW_HEIGHT);
        SDL_Elipse elipse = create_elipse(x, y, 10, 10);
        add_elipse_d(eset, elipse, entity_id, em);

        Speed sp = { (float) (i + 1), (float) 9.81 / 60 * 5 * (i + 1) };
        add_speed_d(sset, sp, entity_id, em);
    }
	
	const auto start = std::chrono::high_resolution_clock::now();
	int i = 0;
    while (draw_objects(object_ids, objects_amount, eset, sset, sdl_window, renderer, benchmark_mode) && i < FRAMES_COUNT) {
		i++;
    }
	const auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	cout << "The generation of " << FRAMES_COUNT << " frames, for " << objects_amount << " objects, lasted = " << duration.count() / 10e5 << " seconds" << endl;
	
	ArenaRelease((&level_arena));
	if(!benchmark_mode) {
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(sdl_window);
		SDL_Quit();
	}
    return 0;
}
