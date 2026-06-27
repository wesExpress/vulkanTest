#include "DarkMatter/dm.h"

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 360

typedef struct vertex_t
{
    float pos[3];
    float color[3];
} vertex;

typedef struct push_constants_t
{
    u64 vb_address;
    float t;
} push_constants;

int main(void)
{
    dm_context context = { 0 };

    if(!dm_init(&context, SCREEN_WIDTH, SCREEN_HEIGHT, "Test", 0, DM_MEGABYTE))
    {
        LOG_FATAL("Initializing DarkMatter failed");
        return 1;
    }

    dm_handle swapchain, pipe, vb_cpu, vb_gpu, ib_cpu, ib_gpu;

    dm_render_target_desc desc = {
        .type=DM_RENDER_TARGET_TYPE_SWAPCHAIN
    };

    if(!dm_renderer_create_render_target(&context, desc, &swapchain)) return 1;

    dm_raster_shader vertex_shader = {
        .path="../assets/shaders/vertex.glsl",
        .entry="main"
    };
    dm_raster_shader fragment_shader = {
        .path="../assets/shaders/fragment.glsl",
        .entry="main"
    };

    dm_raster_pipe_desc pipe_desc = {
        .shaders={
            vertex_shader, 
            fragment_shader
        },
        .push_constant_size=sizeof(push_constants)
    };

    if(!dm_renderer_create_raster_pipeline(&context, pipe_desc, &pipe)) return 1;

    vertex vertices[] = {
        { { -0.5f,-0.5f,0.f }, { 1,0,0 } },
        { { 0.5f,-0.5f,0.f }, { 0,1,0 } },
        { { 0.f,0.5f,0.f }, { 0,0,1 } },
    };

    dm_buffer_desc buffer_desc = {
        .size=sizeof(vertices),
        .stride=sizeof(vertex),
        .type=DM_BUFFER_TYPE_VERTEX,
        .reside=DM_BUFFER_RESIDE_CPU,
        .data=vertices
    };

    if(!dm_renderer_create_buffer(&context, buffer_desc, &vb_cpu)) return 1;

    buffer_desc.reside = DM_BUFFER_RESIDE_GPU;
    if(!dm_renderer_create_buffer(&context, buffer_desc, &vb_gpu)) return 1;

    u32 indices[] = {
        0,2,1
    };

    dm_buffer_desc index_desc = {
        .size=sizeof(indices),
        .stride=sizeof(u32),
        .type=DM_BUFFER_TYPE_INDEX,
        .reside=DM_BUFFER_RESIDE_CPU,
        .data=indices
    };

    if(!dm_renderer_create_buffer(&context, index_desc, &ib_cpu)) return 1;

    index_desc.reside = DM_BUFFER_RESIDE_GPU;
    if(!dm_renderer_create_buffer(&context, index_desc, &ib_gpu)) return 1;

    dm_render_command_copy_buffer(&context, vb_cpu, vb_gpu);
    dm_render_command_copy_buffer(&context, ib_cpu, ib_gpu);

    push_constants constants = { 
        .vb_address=dm_renderer_get_buffer_address(&context, vb_gpu)
    };

    // main loop
    while(dm_is_running(context))
    {
        dm_update(&context);

        constants.t += 0.005f;

        vertices[0].pos[0] += constants.t;
        vertices[1].pos[0] += constants.t;
        vertices[2].pos[0] += constants.t;

        dm_render_command_update_buffer(&context, vb_cpu, vertices, sizeof(vertices));
        dm_render_command_copy_buffer(&context, vb_cpu, vb_gpu);

        if(!dm_begin_render(&context)) break;

            dm_render_command_begin_rendering(&context, swapchain, 0.1f, 0.1f, 0.5f, 1, 1);
                dm_render_command_bind_pipeline(&context, pipe);
                dm_render_command_push_constants(&context, pipe, &constants, sizeof(constants));
                dm_render_command_bind_index_buffer(&context, ib_gpu, 0);
                dm_render_command_draw(&context, 3); 
            dm_render_command_end_rendering(&context, swapchain);

        if(!dm_end_render(&context))   break;
    }

    dm_shutdown(&context);

    return 0;
}
