#include "DarkMatter/dm.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 360

typedef struct vertex_t
{
    float pos[4];
    float color[4];
} vertex;

typedef struct push_constants_t
{
    u32 vb_index;
    float t;
} push_constants;

dm_handle create_texture(dm_context *context, const char *path)
{
    dm_handle handle = { 0 };

    int width, height, n_channels;
    void* data = stbi_load(path, &width, &height, &n_channels, 4);
    if(!data)
    {
        LOG_ERROR("Could not load texture");
        return handle;
    }

    dm_texture2d_desc texture_desc = {
        .data=data,
        .width=width,
        .height=height,
        .size=width*height*n_channels,
        .type=DM_TEXTURE2D_TYPE_SAMPLED
    };

    if(!dm_renderer_create_texture(context, texture_desc, &handle)) return (dm_handle){ 0 };
    stbi_image_free(data);

    return handle;
}

int main(void)
{
    dm_context context = { 0 };

    if(!dm_init(&context, SCREEN_WIDTH, SCREEN_HEIGHT, "Test", 0, DM_MEGABYTE))
    {
        LOG_FATAL("Initializing DarkMatter failed");
        return 1;
    }

    dm_handle swapchain, pipe, vb_cpu, vb_gpu, ib_cpu, ib_gpu, push_data_cpu, push_data_gpu;
    dm_handle texture, heap, sampler_heap;

    push_constants constants = { 0 };

    // render target
    dm_render_target_desc desc = {
        .type=DM_RENDER_TARGET_TYPE_SWAPCHAIN,
        .color_attachment.load_op=DM_RENDER_ATTACHMENT_LOAD_OP_CLEAR,
        .color_attachment.store_op=DM_RENDER_ATTACHMENT_STORE_OP_STORE,
        .depth_attachment.load_op=DM_RENDER_ATTACHMENT_LOAD_OP_CLEAR,
        .depth_attachment.store_op=DM_RENDER_ATTACHMENT_STORE_OP_DONT_CARE
    };

    if(!dm_renderer_create_render_target(&context, desc, &swapchain)) return 1;

    // descriptor heaps
    dm_descriptor_heap_desc heap_desc = {
        .type=DM_DESCRIPTOR_HEAP_TYPE_RESOURCE,
        .buffer_count=2,
        .texture_count=1
    };

    if(!dm_renderer_create_descriptor_heap(&context, heap_desc, &heap)) 
    {
        LOG_FATAL("Creating resource heap failed");
        return 1;
    }

    heap_desc.type = DM_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    heap_desc.buffer_count = 0;
    heap_desc.texture_count = 0;
    heap_desc.sampler_count = 1;

    if(!dm_renderer_create_descriptor_heap(&context, heap_desc, &sampler_heap))
    {
        LOG_FATAL("Creating sampler heap failed");
        return 1;
    }

    // raster pipe
    dm_raster_shader vertex_shader = {
        .path="../../assets/shaders/test.glsl",
        .entry="main"
    };
    dm_raster_shader fragment_shader = {
        .path="../../assets/shaders/fragment.glsl",
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

    // vertex and index buffer
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
        .data=vertices,
    };

    if(!dm_renderer_create_buffer(&context, buffer_desc, &vb_cpu)) return 1;

    buffer_desc.reside = DM_BUFFER_RESIDE_GPU;
    buffer_desc.data   = NULL;
    if(!dm_renderer_create_buffer(&context, buffer_desc, &vb_gpu)) return 1;

    u32 indices[] = {
        0,2,1
    };

    dm_buffer_desc index_desc = {
        .size=sizeof(indices),
        .stride=sizeof(u32),
        .type=DM_BUFFER_TYPE_INDEX,
        .reside=DM_BUFFER_RESIDE_CPU,
        .data=indices,
    };

    if(!dm_renderer_create_buffer(&context, index_desc, &ib_cpu)) return 1;

    index_desc.reside = DM_BUFFER_RESIDE_GPU;
    index_desc.data   = NULL;
    if(!dm_renderer_create_buffer(&context, index_desc, &ib_gpu)) return 1;

    dm_render_command_copy_buffer(&context, vb_cpu, vb_gpu);
    dm_render_command_copy_buffer(&context, ib_cpu, ib_gpu);

    dm_buffer_desc push_desc = {
        .type=DM_BUFFER_TYPE_STORAGE,
        .reside=DM_BUFFER_RESIDE_CPU,
        .size=sizeof(push_constants),
        .data=&constants,
    };

    if(!dm_renderer_create_buffer(&context, push_desc, &push_data_cpu)) return 1;

    push_desc.reside = DM_BUFFER_RESIDE_GPU;
    push_desc.data   = NULL;

    if(!dm_renderer_create_buffer(&context, push_desc, &push_data_gpu)) return 1;

    // random texture
    texture = create_texture(&context, "../../assets/textures/awesomeface.png");
    if(texture.r_type==DM_RESOURCE_TYPE_INVALID) return 1;

    // descriptors
    if(!dm_renderer_upload_resource_to_heap(&context, heap, &vb_gpu))  return 1;
    //if(!dm_renderer_upload_resource_to_heap(&context, heap, &ib_gpu))  return 1;
    //if(!dm_renderer_upload_resource_to_heap(&context, heap, &texture)) return 1;

    /*
     */
    constants.vb_index = vb_gpu.heap_index;

    dm_render_command_update_buffer(&context, push_data_cpu, &constants, sizeof(constants));
    dm_render_command_copy_buffer(&context, push_data_cpu, push_data_gpu);

    // main loop
    while(dm_is_running(context))
    {
        dm_update(&context);

        constants.t += 0.005f;

        dm_render_command_update_buffer(&context, push_data_cpu, &constants, sizeof(constants));
        dm_render_command_copy_buffer(&context, push_data_cpu, push_data_gpu);

        if(!dm_begin_render(&context)) break;
            dm_render_command_bind_descriptor_heap(&context, heap);

            dm_render_command_begin_rendering(&context, swapchain, 0.1f, 0.1f, 0.5f, 1, 1);
                dm_render_command_push_constants(&context, push_data_gpu);
                dm_render_command_bind_pipeline(&context, pipe);
                dm_render_command_bind_index_buffer(&context, ib_gpu, 0);
                dm_render_command_draw(&context, 3, 1); 
            dm_render_command_end_rendering(&context, swapchain);

        if(!dm_end_render(&context))   break;
    }

    dm_shutdown(&context);

    return 0;
}
