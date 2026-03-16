#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Arena {
    unsigned char* memory; // actual memory
    size_t size; // total size of the whole arena
    size_t offset; // currently used memory size
} Arena;

#ifdef __cplusplus
extern "C" {
#endif

Arena* ArenaAlloc(size_t size);
Arena* ArenaAlignedAlloc(size_t size);

void ArenaRelease(Arena** arena);

void* ArenaPush(Arena* arena, uint64_t size);
void* ArenaPushZero(Arena* arena, uint64_t size);

void ArenaPop(Arena* arena, uint64_t size);

void ArenaClear(Arena* arena);

#ifdef __cplusplus
}
#endif

// some helpful macros:
#define PushArray(arena, type, count) \
    (type*)ArenaPush((arena), sizeof(type) * count)
#define PushArrayZero(arena, type, count) \
    (type*)ArenaPushZero((arena), sizeof(type) * count)
#define PushStruct(arena, type) (type*)PushArray((arena), type, 1)
#define PushStructZero(arena, type) (type*)PushArrayZero((arena), type, 1)

#endif
