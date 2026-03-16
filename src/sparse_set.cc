#include <pthread.h>
#include <SDL2/SDL.h>
#include "../include/sparse_set.h"
#include <stdlib.h>

const uint32_t points_size = (10 * 8 * 35 / 49 + (8 - 1)) & -8;

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

entity_manager* entity_manager_init(entity_manager* em, Arena* arena, size_t capacity)
{
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
    // return (gen << 16) | id;
    entity_id ei = { id, gen };
    return ei;
}

// elipse_sset functions
elipse_sset* elipse_sset_init(elipse_sset* set, Arena* arena, size_t capacity)
{
    set->dense = PushArray(arena, uint32_t, capacity);
    set->sparse = PushArray(arena, uint32_t, capacity);
    // set->data = PushArray(arena, SDL_Elipse, capacity);
    set->x = PushArray(arena, float, capacity);
    set->y = PushArray(arena, float, capacity);
    set->w = PushArray(arena, uint32_t, capacity);
    set->h = set->w;

    set->points = PushArray(arena, SDL_Point, capacity * points_size);

    set->capacity = capacity;
    set->count = 0;
    return set;
}


elipse_sset_d* elipse_sset_d_init(elipse_sset_d* set, Arena* arena, size_t capacity) {
	set->dense = PushArray(arena, uint32_t, capacity); 
	set->sparse = PushArray(arena, uint32_t, capacity); 
	set->data = PushArray(arena, SDL_Elipse, capacity); 
    set->capacity = capacity;
    set->count = 0;
    return set;
}

void add_elipse(elipse_sset* set, float x, float y, uint32_t w, uint32_t h, SDL_Point* points, entity_id id, entity_manager* em)
{
    if (set->count > set->capacity || id.generation != em->generations[id.id]) {
        // printf("Error could not be added! capacity(%zu) id smaller than count(%zu)\n", set->capacity, set->count);
        return;
    }

    set->dense[set->count] = id.id;
    set->sparse[id.id] = set->count;
    // set->data[set->count] = elipse;
    set->x[set->count] = x;
    set->y[set->count] = y;
    set->h[set->count] = h;
    set->count++;
    // printf("  SUCCESS: added at count=%zu\n", set->count - 1);
}

void add_elipse_d(elipse_sset_d* set, SDL_Elipse elipse, entity_id id, entity_manager* em)
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

void remove_id(elipse_sset* set, entity_id id, entity_manager* em)
{
    size_t old_id = set->sparse[id.id];
    size_t last = --set->count;

    set->dense[old_id] = last;
    // set->data[old_id] = set->data[last];
    set->x[old_id] = set->x[last];
    set->y[old_id] = set->y[last];
    set->h[old_id] = set->h[last];

    set->sparse[set->dense[old_id]] = old_id;

    em->free_list[em->free_count] = old_id;
    em->free_count++;
}

void clear(elipse_sset* set)
{
    set->count = 0;
}

size_t search(elipse_sset* set, uint32_t id)
{
    size_t index = set->sparse[id];
    return index;
}

// speed_sset functions
speed_sset* speed_sset_init(speed_sset* set, Arena* arena, size_t capacity)
{
    set->dense = PushArray(arena, uint32_t, capacity);
    set->sparse = PushArray(arena, uint32_t, capacity);
    // set->data = PushArray(arena, Speed, capacity);
    set->x_speed = PushArray(arena, float, capacity);
    set->y_speed = PushArray(arena, float, capacity);

    set->capacity = capacity;
    set->count = 0;
    return set;
}

void add_speed(speed_sset* set, float x_speed, float y_speed, entity_id id, entity_manager* em)
{
    if (set->count >= set->capacity || id.generation != em->generations[id.id]) {
        // printf("Error could not be added! capacity(%zu) id smaller than count(%zu)\n", set->capacity, set->count);
        return;
    }

    set->dense[set->count] = id.id;
    set->sparse[id.id] = set->count;
    // set->data[set->count] = sp;
    set->x_speed[set->count] = x_speed;
    set->y_speed[set->count] = y_speed;
    set->count++;
}


speed_sset_d* speed_sset_d_init(speed_sset_d* set, Arena* arena, size_t capacity)
{
	set->dense = PushArray(arena, uint32_t, capacity); 
	set->sparse = PushArray(arena, uint32_t, capacity); 
	set->data = PushArray(arena, Speed, capacity); 
    set->capacity = capacity;
    set->count = 0;
    return set;
}


void add_speed_d(speed_sset_d* set, Speed sp, entity_id id, entity_manager* em)
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

