#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(location=0) out vec3 vertex_color;

layout(push_constant, scalar) uniform push_constants
{
    uint64_t vertex_address;
    float    t;
} constants;

struct vertex
{
    vec3 position;
    vec3 color;
};

layout(buffer_reference, scalar) readonly buffer vertex_buffer
{
    vertex vertices[];
};

void main()
{
    vertex_buffer vb = vertex_buffer(constants.vertex_address);

    vertex v = vb.vertices[gl_VertexIndex];

    vec3 position = v.position;
    //position.x += constants.t;

    vec3 color    = v.color; 

    gl_Position = vec4(position, 1.f);
    vertex_color = color;
}
