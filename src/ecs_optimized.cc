#include <SDL2/SDL.h>

#include "../include/arena.h"
#include "../include/sparse_set.h"

#include <arm_neon.h>
#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;

/////////////////////////////////////////////////////////////////////////////////////////////
// const 

const static uint32_t WINDOW_WIDTH = 1000;
const static uint32_t WINDOW_HEIGHT = 800;
const int CIRCLE_WIDTH = 10;

const uint32_t points_size = (10 * 8 * 35 / 49 + (8 - 1)) & -8;
static SDL_Point* points_template = NULL;

static Arena* level_arena = NULL; 

pthread_mutex_t mtx_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t task_available = PTHREAD_COND_INITIALIZER, task_done = PTHREAD_COND_INITIALIZER;
static int frame_ready = 0;
static int finished_threads = 0;
static int stop_threads = 0;

uint32_t thread_remaining = 0;

uint32_t task_counter = 0;
const static int THREAD_COUNT = 4; // optimal amount of threads (if possible)
static int nprocs = 1;

/////////////////////////////////////////////////////////////////////////////////////////////
// Entities

typedef struct Range {
    uint32_t st, end;
} Range;

typedef struct {
    void* (*func)(void*);
    void* arg;
} task_t;

/////////////////////////////////////////////////////////////////////////////////////////////
// Data structures 

static elipse_sset* eset = NULL;
static speed_sset* sset = NULL;
static entity_manager* em = NULL;

/////////////////////////////////////////////////////////////////////////////////////////////
// functions 

void compute_circle(float& centerX, float& centerY, const uint32_t& h, SDL_Point* points, size_t st_ind, size_t points_size)
{
    int32_t x = (h - 1);
    int32_t y = 0;
    int32_t tx = 1;
    int32_t ty = 1;
    int32_t error = (tx - (h << 1));

    uint32_t counter = st_ind * points_size;
    while (x >= y) {
        // Each of the following renders an octant of the circle

        int32_t centerxx = centerX + x;
        int32_t centerxy = centerX + y;

        points[counter] = { centerxx, (int32_t)centerY - y };
        points[counter + 1] = { centerxx, (int32_t)centerY + y };

        points[counter + 2] = { (int32_t)centerX - x, (int32_t)centerY - y };
        points[counter + 3] = { (int32_t)centerX - x, (int32_t)centerY + y };

        points[counter + 4] = { (int32_t)centerX + y, (int32_t)centerY - x };
        points[counter + 5] = { (int32_t)centerX + y, (int32_t)centerY + x };

        points[counter + 6] = { (int32_t)centerX - y, (int32_t)centerY - x };
        points[counter + 7] = { (int32_t)centerX - y, (int32_t)centerY + x };

        if (error <= 0) {
            ++y;
            error += ty;
            ty += 2;
        }

        if (error > 0) {
            --x;
            tx += 2;
            error += (tx - (h << 1));
        }
        counter += 8;
    }
}

void DrawCircle(SDL_Renderer* renderer, SDL_Point* points, uint32_t points_size)
{
    SDL_RenderDrawPoints(renderer, points, points_size);
}

void* physics_computations(void* argv)
{
    const Range* range = (Range*)argv;
    const uint32_t start_ind = range->st, objects_amount = range->end;
    const float half_h = eset->h[0] / 2.0f;

    const uint32_t leftover = objects_amount % 4;
    float32x4_t vPadding = vdupq_n_f32(half_h);
    float32x4_t vWindowWidth = vdupq_n_f32(WINDOW_WIDTH), vWindowHeight = vdupq_n_f32(WINDOW_HEIGHT); // window size
    float32x4_t vZero = vdupq_n_f32(0.0f), vair_resistance = vdupq_n_f32(0.99f);

    uint32_t circle_id = start_ind;
    for (; circle_id + 4 <= objects_amount; circle_id += 4) {
        __builtin_prefetch(&eset->x[circle_id + 32], 0, 1); // last param should be 1
        __builtin_prefetch(&eset->y[circle_id + 32], 0, 1); // last param should be 1

        float32x4_t vX = vld1q_f32(&eset->x[circle_id]);
        float32x4_t vY = vld1q_f32(&eset->y[circle_id]);

        // x collision
        uint32x4_t xCollision = vorrq_u32(
            vcgtq_f32(vaddq_f32(vX, vPadding), vWindowWidth), // (xpos + padding) greater than vWindowWidth
            vcleq_f32(vsubq_f32(vX, vPadding), vZero) // (xpos - padding) less than 0
        );

        //// y collision
        uint32x4_t yCollision = vorrq_u32(
            vcgtq_f32(vaddq_f32(vY, vPadding), vWindowHeight), // (ypos + padding) greater than vWindowWidth
            vcleq_f32(vsubq_f32(vY, vPadding), vZero) // (ypos - padding) less than 0
        );

        float32x4_t vYSpeed = vld1q_f32(&sset->y_speed[circle_id]);
        float32x4_t vXSpeed = vld1q_f32(&sset->x_speed[circle_id]);

        // selecting only collided circles
        vXSpeed = vbslq_f32(xCollision, vnegq_f32(vXSpeed), vXSpeed);
        vYSpeed = vbslq_f32(yCollision, vnegq_f32(vYSpeed), vYSpeed);

        // computing air-resistance
        vYSpeed = vmulq_f32(vYSpeed, vair_resistance);
        vXSpeed = vmulq_f32(vXSpeed, vair_resistance);

        // computing new center position
        vX = vaddq_f32(vX, vXSpeed);
        vY = vaddq_f32(vY, vYSpeed);

        vst1q_f32(&sset->x_speed[circle_id], vXSpeed);
        vst1q_f32(&sset->y_speed[circle_id], vYSpeed);

        vst1q_f32(&eset->x[circle_id], vX);
        vst1q_f32(&eset->y[circle_id], vY);

        // storing new converted circle positions
        int32_t xC[4], yC[4];
        vst1q_s32(xC, vcvtnq_s32_f32(vX));
        vst1q_s32(yC, vcvtnq_s32_f32(vY));

        //  moving circles
        for (int point_id = 0; point_id < points_size; point_id += 4) {
            // loading SDL_Point
            int32x4x2_t xyT = vld2q_s32((const int32_t*)&points_template[point_id]); // values: x1, x2, x3, x4

            // int32_t xNew[4], yNew[4];
            for (int laneC = 0; laneC < 4; ++laneC) {
                // computing new 4 positions for 1 circle
                int32x4_t vXnew = vaddq_s32(vdupq_n_s32(xC[laneC]), xyT.val[0]);
                int32x4_t vYnew = vaddq_s32(vdupq_n_s32(yC[laneC]), xyT.val[1]);

                int id = (circle_id + laneC) * points_size + (point_id);

                // storing
                int32x4x2_t vXYnew = { vXnew, vYnew };
                vst2q_s32((int32_t*)&eset->points[id], vXYnew);
            }
        }
    }

    for (; circle_id < objects_amount; circle_id++) {
        sset->y_speed[circle_id] *= 0.99f;
        sset->x_speed[circle_id] *= 0.99f;

        // objects movement
        eset->y[circle_id] += sset->y_speed[circle_id];
        eset->x[circle_id] += sset->x_speed[circle_id];
    }

    return NULL;
}

bool draw_objects(entity_id* object_ids, const size_t& capacity, Range* ranges, pthread_t* threads, const uint32_t& nprocs, SDL_Window* sdl_window, SDL_Renderer* renderer, int benchmark_mode)
{
    SDL_Event event;
    while (!benchmark_mode && SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            return false;
        }
    }

    pthread_mutex_lock(&mtx_lock);
    frame_ready = 1;
    finished_threads = 0;

    pthread_cond_broadcast(&task_available);

	while (finished_threads < nprocs) {
        pthread_cond_wait(&task_done, &mtx_lock);
    }

    frame_ready = 0;
    pthread_mutex_unlock(&mtx_lock);

    return true;
}

void* create_circles(void* argv)
{
    Range* range = (Range*)argv;
    uint32_t i = range->st;
    while (i < range->end) {
        entity_id entity_id = create_id(em);

        float x = (arc4random() % WINDOW_WIDTH);
        float y = (arc4random() % WINDOW_HEIGHT);

        add_elipse(eset, x, y, CIRCLE_WIDTH, CIRCLE_WIDTH, NULL, entity_id, em);
        add_speed(sset, (float)(i + 1), (float)9.81 / 60 * 5 * (i + 1), entity_id, em);
        compute_circle(x, y, CIRCLE_WIDTH, eset->points, i, points_size);

        i++;
    }
    return NULL;
}

// thread worker loop

void* thread_work(void* argc)
{
    task_t* task = (task_t*)argc;

    while (1) {
        pthread_mutex_lock(&mtx_lock);

        while (!frame_ready && !stop_threads) {
            pthread_cond_wait(&task_available, &mtx_lock);
        }

        if (stop_threads) {
            pthread_mutex_unlock(&mtx_lock);
            break;
        }

        pthread_mutex_unlock(&mtx_lock);

        task->func(task->arg);

        pthread_mutex_lock(&mtx_lock);
        finished_threads++;

        if (finished_threads == nprocs) {
            pthread_cond_signal(&task_done);
        }

        pthread_mutex_unlock(&mtx_lock);
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// main function

int run_ecs_optimized(int amount_of_objects, int benchmark_mode, int frames_count)
{
    static uint32_t objects_amount = amount_of_objects;
    static int FRAMES_COUNT = frames_count;
	level_arena = ArenaAlignedAlloc((sizeof(elipse_sset) + sizeof(speed_sset) + sizeof(entity_manager) + (sizeof(SDL_Point) * points_size)) * (objects_amount + 1)); // 80 mb

	eset = PushStruct(level_arena, elipse_sset);
	sset = PushStruct(level_arena, speed_sset);
	em = PushStruct(level_arena, entity_manager);

    SDL_Window* sdl_window = NULL;
    SDL_Renderer* renderer = NULL;

    if (!benchmark_mode) {
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

    // data strcutres allocations
    eset = elipse_sset_init(eset, level_arena, objects_amount);
    sset = speed_sset_init(sset, level_arena, objects_amount);
    em = entity_manager_init(em, level_arena, objects_amount);

    float x0 = 0, y0 = 0;
    points_template = PushArray(level_arena, SDL_Point, points_size);
    compute_circle(x0, y0, CIRCLE_WIDTH, points_template, 0, points_size);

    entity_id object_ids[1];
	if(objects_amount >= 100) {
		nprocs = sysconf(_SC_NPROCESSORS_ONLN) > 2 ? std::min((int)THREAD_COUNT, (int)sysconf(_SC_NPROCESSORS_ONLN)) : nprocs;
	}
    std::cout << "Thread count: " << nprocs << endl;

	for(uint32_t i = 0; i < objects_amount; i++) {
        entity_id entity_id = create_id(em);

        float x = (arc4random() % WINDOW_WIDTH);
        float y = (arc4random() % WINDOW_HEIGHT);

        add_elipse(eset, x, y, CIRCLE_WIDTH, CIRCLE_WIDTH, NULL, entity_id, em);
        add_speed(sset, (float)(i + 1), (float)9.81 / 60 * 5 * (i + 1), entity_id, em);
        compute_circle(x, y, CIRCLE_WIDTH, eset->points, i, points_size);
	}

    // init threads and tasks
    pthread_t threads[nprocs];
	task_t* tasks = (task_t*)malloc(sizeof(task_t) * nprocs);

    const uint32_t id_group_size = objects_amount / nprocs;
    Range ranges[nprocs];
    for (int i = 0; i < nprocs; ++i) {
        ranges[i] = { i * (id_group_size), (i + 1) * id_group_size };
		tasks[i].func = &physics_computations; 
		tasks[i].arg = &ranges[i];
        pthread_create(&threads[i], NULL, thread_work, &tasks[i]);
    }

    const auto start = std::chrono::high_resolution_clock::now();
    int i = 0;
    while (draw_objects(object_ids, objects_amount, ranges, threads, nprocs, sdl_window, renderer, benchmark_mode) && i < FRAMES_COUNT) {
        if (!benchmark_mode) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderClear(renderer);
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

            SDL_RenderDrawPoints(renderer, eset->points, objects_amount * points_size);

            SDL_RenderPresent(renderer);
        }
        i++;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    printf("The generation of %i frames, for %i objects, lasted = %.4f seconds (without deinitialization), amount of threads = %i\n", FRAMES_COUNT, objects_amount, duration.count() / 1e6, nprocs);

    ArenaRelease((&level_arena));
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    printf("The generation of %i frames, for %i objects, lasted = %.4f seconds, amount of threads = %i\n", FRAMES_COUNT, objects_amount, duration.count() / 1e6, nprocs);

    if (!benchmark_mode) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(sdl_window);
        SDL_Quit();
    }
    return 0;
}
