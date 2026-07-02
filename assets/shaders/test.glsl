#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_descriptor_heap : require
#extension GL_EXT_nonuniform_qualifier : require

struct vertex
{
    vec4 position;
    vec4 color;
};

layout (descriptor_heap) readonly buffer vertex_buffer
{
    vertex vertices[];
} vb_heap[];

layout (buffer_reference) readonly buffer reference
{
    uint index;
    float t;
};

layout (push_constant) uniform push_constants 
{
    reference r;
} constants;

layout(location=0) out vec3 vertex_color;

void main()
{
    reference r = constants.r;

    vertex v = vb_heap[r.index].vertices[gl_VertexIndex];

    vec3 position = v.position.xyz;
    position.x += r.t;

    gl_Position = vec4(position,1);

    vertex_color = v.color.rgb;
}
