#include "dm.h"

// arena
void dm_arena_create(size_t size, dm_arena* arena)
{
    arena->capacity = size;
    arena->start = malloc(size);
    arena->current = arena->start;
}

void dm_arena_detroy(dm_arena* arena)
{
    if(arena->start) free(arena->start);
}

void* dm_arena_alloc(dm_arena* arena, size_t size, size_t* offset)
{
    if(arena->size + size >= arena->capacity) return NULL;

    arena->size += size;
    arena->current += size;

    return arena->current - size;;
}

void* dm_arena_get_ptr(dm_arena arena, size_t offset)
{
    return arena.start + offset;
}

extern bool dm_window_create(dm_context* context, u16 width, u16 height, const char* title);
extern void dm_window_destroy(dm_context* context);

extern bool dm_renderer_init(dm_context* context, u16 width, u16 height);
extern void dm_renderer_shutdown(dm_context* context);

// context
bool dm_init(dm_context* context, u16 width, u16 height, const char* title, dm_context_flag flags, size_t memory_size)
{
    dm_arena_create(memory_size, &context->arena);

    if(!dm_window_create(context, width, height, title)) return false;
    if(!dm_renderer_init(context, width, height))
    {
        dm_window_destroy(context);
        return false;
    }

    return true;
}

void dm_shutdown(dm_context* context)
{
    dm_renderer_shutdown(context);
    dm_window_destroy(context);

    dm_arena_detroy(&context->arena);
}
