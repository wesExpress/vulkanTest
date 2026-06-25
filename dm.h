#ifndef __DM_DEFINES_H__
#define __DM_DEFINES_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef DEBUG
#define DM_DEBUG
#endif

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define DM_KILABYTE 1024
#define DM_MEGABYTE (DM_KILABYTE * 1024)
#define DM_GIGABYTE (DM_MEGABYTE * 1024)

// arena
typedef struct dm_arena_t
{
    size_t size, capacity;
    void* start;
    void* current;
} dm_arena;

// window
typedef struct dm_window_t
{
    u16 width, height;

    size_t offset;
} dm_window;

// renderer

typedef struct dm_renderer_t
{
    u16 width, height;

    size_t offset;
} dm_renderer;

// context
typedef enum dm_context_flag_t
{
    DM_CONTEXT_FLAG_IS_RUNNING     = 1,
    DM_CONTEXT_FLAG_RENDERER_VSYNC = 2,
    DM_CONTEXT_FLAG_WINDOW_RESIZED = 4,
} dm_context_flag;

typedef struct dm_context_t
{
    dm_window window;
    dm_renderer renderer;

    dm_context_flag flags;

    dm_arena arena;
} dm_context;

// functions
bool dm_init(dm_context* context, u16 width, u16 height, const char* title, dm_context_flag flags, size_t memory_size);
void dm_shutdown(dm_context* context);

#endif
