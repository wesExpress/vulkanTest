#ifndef __DM_DEFINES_H__
#define __DM_DEFINES_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef NDEBUG
#define DM_DEBUG
#endif

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define DM_KILABYTE 1024
#define DM_MEGABYTE (DM_KILABYTE * 1024)
#define DM_GIGABYTE (DM_MEGABYTE * 1024)

#include "clog/clog.h"

#define LOG_DEBUG(...) DBG(__VA_ARGS__)
#define LOG_TRACE(...) TRC(__VA_ARGS__)
#define LOG_INFO(...)  INF(__VA_ARGS__)
#define LOG_WARN(...)  WRN(__VA_ARGS__)
#define LOG_ERROR(...) ERR(__VA_ARGS__)
#define LOG_FATAL(...) FTL(__VA_ARGS__)

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
void dm_arena_create(size_t size, dm_arena* arena);
void dm_arena_detroy(dm_arena* arena);
void* dm_arena_alloc(dm_arena* arena, size_t size, size_t* offset);
void* dm_arena_get_ptr(dm_arena arena, size_t offset);

bool dm_init(dm_context* context, u16 width, u16 height, const char* title, dm_context_flag flags, size_t memory_size);
void dm_shutdown(dm_context* context);
bool dm_is_running(dm_context context);
void dm_update(dm_context* context);
bool dm_begin_render(dm_context* context);
bool dm_end_render(dm_context* context);

#endif
