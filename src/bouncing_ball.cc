/* NOTE:
 * The idea of this program is: to compare the efficiency of different code architectures (OOP and ESC).
 * Problem: the program draws 100, 1k, 10k, 100k, 1m of objects and simulates how the ball (circle) falls and then bounces back.
 */

#include <SDL2/SDL.h>
#include <iostream>
#include "arena.h"

using namespace std;

const static uint32_t WINDOW_WIDTH = 1000;
const static uint32_t WINDOW_HEIGHT = 800;
const uint32_t FRAMES_COUNT = 1000;

const uint32_t objects_amount = 100000;


const uint32_t points_size = (10 * 8 * 35 / 49 + (8 - 1)) & -8;
SDL_Point points[points_size]; 

/////////////////////////////////////////////////////////////////////////////////////////////
/*
 * ECS implementation
 *
 *
 * Sparse set features:
 * add
 * delete (Keeping track of free ids, generations, alive)
 * search
 * clear
 *
 *
 * Sparse set ids features:
 */

// Entity
typedef struct SDL_Elipse {
    float x, y;
    uint32_t w, h;
} SDL_Elipse;

typedef struct {
    float x_speed, y_speed;
} Speed;

typedef struct sparse_set {
    uint32_t* dense;
    uint32_t* sparse;
    SDL_Elipse* data;
    size_t capacity, count;
} elipse_sset;

typedef struct {
    uint32_t* dense;
    uint32_t* sparse;
    Speed* data;
    size_t capacity, count;
} speed_sset;


typedef struct {
    // pool allocator
    uint32_t* free_list;
    size_t free_count;
    size_t next_id;
    uint32_t* generations;

	uint32_t capacity;
} entity_manager;


typedef struct {
	uint32_t id;
	uint16_t generation;
} entity_id;

SDL_Elipse create_elipse(float_t x, float_t y, uint32_t w, uint32_t h)
{
    SDL_Elipse elipse;
    elipse.x = x;
    elipse.y = y;
    elipse.w = w;
    elipse.h = h;
    return elipse;
}

SDL_Rect create_rect(float_t x, float_t y, float_t w, float_t h)
{
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    return rect;
}

// System

entity_manager* entity_manager_init(entity_manager* em, Arena* arena, size_t capacity) {
	em->capacity = capacity;
    em->generations = PushArray(arena, uint32_t, capacity); 
    em->free_list = PushArray(arena, uint32_t, capacity); 
    em->free_count = 0;
    em->next_id = 0;
	return em;
}

entity_id create_id(entity_manager* em)
{
    uint32_t id;
    if (em->free_count == 0) {
        id = em->next_id++;
		em->generations[id] = 0;
    } else {
        id = em->free_list[--em->free_count];
		em->generations[id]++;
    }

	uint16_t gen = em->generations[id];
    //return (gen << 16) | id;
	entity_id ei = {id, gen};
	return ei;
}

// elipse_sset functions
elipse_sset* elipse_sset_init(elipse_sset* set, Arena* arena, size_t capacity)
{
	set->dense = PushArray(arena, uint32_t, capacity); 
	set->sparse = PushArray(arena, uint32_t, capacity); 
	set->data = PushArray(arena, SDL_Elipse, capacity); 
    set->capacity = capacity;
    set->count = 0;
    return set;
}


void add(sparse_set* set, SDL_Elipse elipse, entity_id id, entity_manager* em)
{

    //printf("add() called: id=%u, gen=%u, count=%zu, capacity=%zu\n", id.id, id.generation, set->count, set->capacity);

    if (set->count > set->capacity || id.generation != em->generations[id.id]) {
		//printf("Error could not be added! capacity(%zu) id smaller than count(%zu)\n", set->capacity, set->count);
        return;
    }

	set->dense[set->count] = id.id;
    set->sparse[id.id] = set->count;
    set->data[set->count] = elipse;
    set->count++;
	//printf("  SUCCESS: added at count=%zu\n", set->count - 1);
}


void remove(elipse_sset* set, entity_id id, entity_manager* em)
{
    size_t old_id = set->sparse[id.id];
    size_t last = --set->count;

    set->dense[old_id] = last;
    set->data[old_id] = set->data[last];
    set->sparse[set->dense[old_id]] = old_id;

    em->free_list[em->free_count] = old_id;
    em->free_count++;
}


void clear(elipse_sset* set)
{
    set->count = 0;
}


SDL_Elipse* search(elipse_sset* set, uint32_t id)
{
    size_t index = set->sparse[id];
    return &set->data[index];
}

// speed_sset functions
speed_sset* speed_sset_init(speed_sset* set, Arena* arena, size_t capacity)
{
	set->dense = PushArray(arena, uint32_t, capacity); 
	set->sparse = PushArray(arena, uint32_t, capacity); 
	set->data = PushArray(arena, Speed, capacity); 
    set->capacity = capacity;
    set->count = 0;
    return set;
}


void add(speed_sset* set, Speed sp, entity_id id, entity_manager* em)
{
    if (set->count >= set->capacity || id.generation != em->generations[id.id]) {
		//printf("Error could not be added! capacity(%zu) id smaller than count(%zu)\n", set->capacity, set->count);
        return;
    }

    set->dense[set->count] = id.id;
    set->sparse[id.id] = set->count;
    set->data[set->count] = sp;
    set->count++;
}



/////////////////////////////////////////////////////////////////////////////////////////////
// main function
//
//

// custom elipse/circle draw function (ECS)

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
	//vector<Point> points = compute_circle_oop(elipse.x, elipse.y, elipse.h, points_size);
	compute_circle(points, elipse->x, elipse->y, elipse->h, points_size);
	SDL_RenderDrawPoints(renderer, points, points_size);
}

bool draw_objects(entity_id* object_ids, size_t capacity, elipse_sset* eset, speed_sset* sset, SDL_Window* sdl_window, SDL_Renderer* renderer)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
			case SDL_QUIT:
				return false;
        }
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

	const auto start = std::chrono::high_resolution_clock::now();
	
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

	for(int i = 0; i < eset->count; ++i) {
		DrawCircle(renderer, &eset->data[i]);
	}
	const auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	//cout << "The generation of each frame lasted = " << duration.count() << " microseconds" << endl;

    SDL_RenderPresent(renderer);
    return true;
}

int main(int argc, char* argv[])
{
    SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not be initialized!\nSDL_Error:", SDL_GetError());
        return 1;
    }

    SDL_Window* sdl_window = SDL_CreateWindow("Test Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (sdl_window == NULL) {
        fprintf(stderr, "SDL Window could not be created!\nSDL_Error:", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(sdl_window, -1, 0);
    if (!renderer) {
        fprintf(stderr, "SDL Renderer could not be created!\nSDL_Error:", SDL_GetError());
        SDL_Quit();
        return 1;
    }

	Arena* level_arena = ArenaAlloc(5000 * 1024); //50 kb
	elipse_sset* eset = PushStruct(level_arena, elipse_sset);
	eset = elipse_sset_init(eset, level_arena, objects_amount);

	speed_sset* sset = PushStruct(level_arena, speed_sset);
	sset = speed_sset_init(sset, level_arena, objects_amount);

	entity_manager* em = PushStruct(level_arena, entity_manager);
	em = entity_manager_init(em, level_arena, objects_amount);

	//entity_manager* em = entity_manager_init(objects_amount);
    //elipse_sset* eset = elipse_sset_init(objects_amount);
    //speed_sset* sset = speed_sset_init(objects_amount);

	entity_id object_ids[10];


    for (int i = 0; i < objects_amount; ++i) {
		entity_id entity_id = create_id(em);
		//object_ids[i] = entity_id;
		float x = (rand() % WINDOW_WIDTH);
		float y = (rand() % WINDOW_HEIGHT);
        SDL_Elipse elipse = create_elipse(x, y, 10, 10);
        add(eset, elipse, entity_id, em);

        Speed sp = { (float) (i + 1), (float) 9.81 / 60 * 5 * (i + 1) };
        add(sset, sp, entity_id, em);
    }
	
	const auto start = std::chrono::high_resolution_clock::now();
	int i = 0;
    while (draw_objects(object_ids, objects_amount, eset, sset, sdl_window, renderer) && i < FRAMES_COUNT) {
        //SDL_Delay(16);
		i++;
    }
	ArenaRelease((&level_arena));
	const auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	cout << "The generation of " << FRAMES_COUNT << " frames, for " << objects_amount << " objects, lasted = " << duration.count() / 10e5 << " seconds" << endl;

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
    return 0;
}
