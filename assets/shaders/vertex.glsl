#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_descriptor_heap : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location=0) out vec3 vertex_color;

struct vertex
{
    vec3 position;
    vec3 color;
};

layout (descriptor_heap) buffer vertexBuffer 
{
    vertex v;
} vertices[];

layout (push_constant) uniform push_constants
{
    uint64_t vertex_address;
    float    t;
} constants;

void main()
{
    vertex v = vertices[nonuniformEXT(gl_VertexIndex)].v;

    vec3 position = v.position;
    position.x += constants.t;

    vec3 color    = v.color; 

    gl_Position = vec4(position, 1.f);
    vertex_color = color;
}
