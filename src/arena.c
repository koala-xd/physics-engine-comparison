#include <malloc/_malloc_type.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/arena.h"

#define uchar unsigned char

Arena* ArenaAlloc(size_t size)
{
    Arena* arena = (Arena*)malloc(sizeof(Arena));
    arena->memory = (uchar*)malloc(size);
    arena->size = size;
    arena->offset = 0;
    return arena;
};

Arena* ArenaAlignedAlloc(size_t size) {
    Arena* arena = (Arena*)malloc(sizeof(Arena));
    arena->memory = (uchar*)aligned_alloc(16, size);
    arena->size = size;
    arena->offset = 0;
	return arena;
};

void ArenaRelease(Arena** arena)
{
    if (*arena) {
        free((*arena)->memory);
        free(*arena);
        *arena = NULL;
    }
};

void* ArenaPush(Arena* arena, uint64_t size)
{
    if (arena->size < size || arena->size - arena->offset < size) {
        fprintf(stderr, "Impossible to allocate memory of size %llu\n", size);
        return NULL;
    }
    void* ptr = arena->memory + arena->offset;
    arena->offset += size;
    return ptr;
};

void* ArenaPushZero(Arena* arena, uint64_t size)
{ // similar as calloc function
    if (sizeof(arena->memory) < size || arena->size - arena->offset < size) {
        fprintf(stderr, "Impossible to allocate memory of size %llu\n", size);
        return NULL;
    }
    void* ptr = arena->memory + arena->offset;
    memset(ptr, 0, size);
    arena->offset += size;
    return ptr;
};

void ArenaPop(Arena* arena, uint64_t size)
{
    if (size >= arena->offset) {
        fprintf(stderr, "Impossible to allocate memory of size %llu\n", size);
        return;
    }
    arena->offset -= size;
};

void ArenaClear(Arena* arena) { arena->offset = 0; };

//int main(int argc, char* argv[])
//{
//    Arena* a = ArenaAlloc(1024 * 8); // 8kb
//    const int arr1_size = 10;
//    int arr1[arr1_size] = { 1, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
//    int* arr1_ptr = PushArray(a, int, arr1_size);
//    // Print the "new" array values directly
//    printf("Arena allocated values:\n");
//    for (int i = 0; i < arr1_size; i++) {
//        printf("arr1_ptr[%d] = %d\n", i, arr1_ptr[i]);
//    }
//    memcpy(arr1_ptr, arr1, sizeof(int) * arr1_size);
//    printf("The array pointer is %p\n", (void*)arr1_ptr);
//    // Print the "new" array values directly
//    printf("The array pointer is %p\n", (void*)arr1_ptr);
//    printf("Arena allocated values:\n");
//    for (int i = 0; i < arr1_size; i++) {
//        printf("arr1_ptr[%d] = %d\n", i, arr1_ptr[i]);
//    }
//
//    Arena* aa = ArenaAlloc(1024 * 8 * 4);
//    Arena* enemies = PushStruct(aa, Arena);
//    Arena* ui_arena = PushStruct(aa, Arena);
//    Arena* particles_arena = PushStruct(aa, Arena);
//
//    enemies->memory = PushArray(aa, unsigned char, 1024 * 16);
//    enemies->offset = 0;
//    enemies->size = 1024 * 16;
//
//    ui_arena->memory = PushArray(aa, unsigned char, 1024 * 8);
//    ui_arena->offset = 0;
//    ui_arena->size = 1024 * 8;
//
//    particles_arena->memory = PushArray(aa, unsigned char, 1024 * 4);
//    particles_arena->offset = 0;
//    particles_arena->size = 1024 * 4;
//
//    ArenaClear(enemies);
//    if (enemies->offset == 0) {
//        printf("Arena *enemies is freed\n");
//    }
//    ArenaRelease(&aa);
//    if (aa == NULL) {
//        printf("Arena *aa is freed\n");
//    }
//    return 0;
//}
