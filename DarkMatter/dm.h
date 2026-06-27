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

// rendering
typedef enum dm_pipeline_type_t
{
    DM_PIPELINE_TYPE_INVALID,
    DM_PIPELINE_TYPE_RASTER,
    DM_PIPELINE_TYPE_COMPUTE,
    DM_PIPELINE_TYPE_RAY_TRACE
} dm_pipeline_type;

typedef enum dm_resource_type_t
{
    DM_RESOURCE_TYPE_INVALID,
    DM_RESOURCE_TYPE_SWAPCHAIN,
    DM_RESOURCE_TYPE_RENDER_TARGET,
    DM_RESOURCE_TYPE_BUFFER,
    DM_RESOURCE_TYPE_SAMPLED_TEXTURE2D,
    DM_RESOURCE_TYPE_STORAGE_TEXTURE2D,
    DM_RESOURCE_TYPE_ACCELERATION_STRUCTURE,
    DM_RESOURCE_TYPE_SAMPLER,
} dm_resource_type;

#define DM_FRAMES_IN_FLIGHT 3

#define DM_MAX_PIPES 10
#define DM_MAX_RASTER_PIPES    DM_MAX_PIPES
#define DM_MAX_COMPUTE_PIPES   DM_MAX_PIPES
#define DM_MAX_RAY_TRACE_PIPES DM_MAX_PIPES

#define DM_MAX_TEXTURES 10
#define DM_MAX_BUFFERS  10
#define DM_MAX_ACCELS   10
#define DM_MAX_SAMPLERS 10

typedef struct dm_handle_t
{
    union
    {
        dm_pipeline_type p_type : 8;
        dm_resource_type r_type : 8;
    };

    u32 index : 24;
} dm_handle;

typedef enum dm_render_target_flag_t
{
    DM_RENDER_TARGET_FLAG_COLOR = 1,
    DM_RENDER_TARGET_FLAG_DEPTH = 2
} dm_render_target_flag;

typedef enum dm_renderattachment_load_op_t
{
    DM_RENDER_ATTACHMENT_LOAD_OP_INVALID,
    DM_RENDER_ATTACHMENT_LOAD_OP_LOAD,
    DM_RENDER_ATTACHMENT_LOAD_OP_CLEAR,
    DM_RENDER_ATTACHMENT_LOAD_OP_DONT_CARE
} dm_render_attachment_load_op;

typedef enum dm_render_attachment_store_op_t
{
    DM_RENDER_ATTACHMENT_STORE_OP_INVALID,
    DM_RENDER_ATTACHMENT_STORE_OP_STORE,
    DM_RENDER_ATTACHMENT_STORE_OP_DONT_CARE
} dm_render_attachment_store_op;

typedef struct dm_render_attachment_desc_t
{
    dm_render_attachment_load_op  load_op;
    dm_render_attachment_store_op store_op;

    dm_handle handle; // ignored if swapchain
} dm_render_attachment_desc;

typedef enum dm_render_target_type_t
{
    DM_RENDER_TARGET_TYPE_INVALID,
    DM_RENDER_TARGET_TYPE_SWAPCHAIN,
    DM_RENDER_TARGET_TYPE_CUSTOM
} dm_render_target_type;

typedef struct dm_render_target_desc_t
{
    dm_render_attachment_desc color_attachment;
    dm_render_attachment_desc depth_attachment;

    dm_render_target_flag flags; // ignored if swapchain
    dm_render_target_type type;
} dm_render_target_desc;

typedef enum dm_raster_shader_stage_t
{
    DM_RASTER_SHADER_STAGE_VERTEX,
    DM_RASTER_SHADER_STAGE_FRAGMENT,
    DM_RASTER_SHADER_STAGE_MAX
} dm_raster_shader_stage;

typedef struct dm_raster_shader_t
{
    char path[512];
    char entry[512];
} dm_raster_shader;

typedef struct dm_raster_pipe_desc_t
{
    dm_raster_shader shaders[DM_RASTER_SHADER_STAGE_MAX];

    size_t push_constant_size;
} dm_raster_pipe_desc;

typedef enum dm_buffer_type_t
{
    DM_BUFFER_TYPE_INVALID,
    DM_BUFFER_TYPE_VERTEX,
    DM_BUFFER_TYPE_INDEX,
    DM_BUFFER_TYPE_STORAGE
} dm_buffer_type;

typedef enum dm_buffer_reside_t
{
    DM_BUFFER_RESIDE_INVALID,
    DM_BUFFER_RESIDE_CPU,
    DM_BUFFER_RESIDE_GPU
} dm_buffer_reside;

typedef struct dm_buffer_desc_t
{
    size_t size, stride;
    dm_buffer_type type;
    dm_buffer_reside reside;
    void* data; // must be long-lasting so it does not decay before creating buffer
} dm_buffer_desc;

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
void dm_arena_create(size_t size, dm_arena *arena);
void dm_arena_detroy(dm_arena *arena);
void* dm_arena_alloc(dm_arena *arena, size_t size, size_t *offset);
void* dm_arena_get_ptr(dm_arena arena, size_t offset);

bool dm_init(dm_context *context, u16 width, u16 height, const char *title, dm_context_flag flags, size_t memory_size);
void dm_shutdown(dm_context *context);
bool dm_is_running(dm_context context);
void dm_update(dm_context *context);
bool dm_begin_render(dm_context *context);
bool dm_end_render(dm_context *context);

void* dm_read_bytes(const char *path, size_t *size);

// resources
bool dm_renderer_create_render_target(dm_context *context, dm_render_target_desc desc, dm_handle *handle);
bool dm_renderer_create_raster_pipeline(dm_context *context, dm_raster_pipe_desc desc, dm_handle *handle);
bool dm_renderer_create_buffer(dm_context* context, dm_buffer_desc desc, dm_handle *handle);

u64 dm_renderer_get_buffer_address(dm_context *context, dm_handle handle);

// commands
void dm_render_command_begin_rendering(dm_context *context, dm_handle handle, float r, float g, float b, float a, float d);
void dm_render_command_end_rendering(dm_context *context, dm_handle handle);
void dm_render_command_bind_pipeline(dm_context *context, dm_handle handle);
void dm_render_command_bind_index_buffer(dm_context *context, dm_handle handle, size_t offset);
void dm_render_command_push_constants(dm_context *context, dm_handle handle, void *data, size_t size);
void dm_render_command_draw(dm_context *context, u32 index_count);

void dm_render_command_update_buffer(dm_context *context, dm_handle handle, void *data, size_t size);
void dm_render_command_copy_buffer(dm_context *context, dm_handle src, dm_handle dst);

#endif
