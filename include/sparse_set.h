#ifndef SPARSE_SET_H
#define SPARSE_SET_H

#include <SDL2/SDL.h>

#include "../include/arena.h"
#include <iostream>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct SDL_Elipse {
    float x, y;
    uint32_t w, h;
} SDL_Elipse;

typedef struct {
    float x_speed, y_speed;
} Speed;


typedef struct {
    uint32_t* dense;
    uint32_t* sparse;
    // instead of SDL_Elipse* data;
    float *x, *y;
    uint32_t *w, *h;
    SDL_Point* points;

    size_t capacity, count;
} elipse_sset;

typedef struct {
    uint32_t* dense;
    uint32_t* sparse;
	SDL_Elipse* data;
    size_t capacity, count;
} elipse_sset_d;

typedef struct {
    uint32_t* dense;
    uint32_t* sparse;
    // instead of Speed* data;
    float* x_speed;
    float* y_speed;

    size_t capacity, count;
} speed_sset;

typedef struct {
    uint32_t* dense;
    uint32_t* sparse;
    Speed* data;
    size_t capacity, count;
} speed_sset_d;

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


SDL_Elipse create_elipse(float_t x, float_t y, uint32_t w, uint32_t h);

SDL_Rect create_rect(float_t x, float_t y, float_t w, float_t h);

#ifdef __cplusplus
extern "C" {
#endif

// System

entity_manager* entity_manager_init(entity_manager* em, Arena* arena, size_t capacity);

entity_id create_id(entity_manager* em);

// elipse_sset functions
elipse_sset_d* elipse_sset_d_init(elipse_sset_d* set, Arena* arena, size_t capacity);

void add_elipse_d(elipse_sset_d* set, SDL_Elipse elipse, entity_id id, entity_manager* em);

elipse_sset* elipse_sset_init(elipse_sset* set, Arena* arena, size_t capacity);

void add_elipse(elipse_sset* set, float x, float y, uint32_t w, uint32_t h, SDL_Point* points, entity_id id, entity_manager* em);

void remove_id(elipse_sset* set, entity_id id, entity_manager* em);

void clear(elipse_sset* set);

size_t search(elipse_sset* set, uint32_t id);

// speed_sset functions

speed_sset* speed_sset_init(speed_sset* set, Arena* arena, size_t capacity);

void add_speed(speed_sset* set, float x_speed, float y_speed, entity_id id, entity_manager* em);


speed_sset_d* speed_sset_d_init(speed_sset_d* set, Arena* arena, size_t capacity);

void add_speed_d(speed_sset_d* set, Speed sp, entity_id id, entity_manager* em);

#ifdef __cplusplus
}
#endif

#endif
