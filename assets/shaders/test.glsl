#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_descriptor_heap : require
#extension GL_EXT_scalar_block_layout : require
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

layout (buffer_reference) readonly buffer constants 
{
    uint index;
    float t;
};

layout (buffer_reference) readonly buffer vertices
{
    vertex vertex_buffer[];
};

layout (push_constant, scalar) uniform push_reference
{
    //constants c;
    vertices v;
} reference;

layout(location=0) out vec3 vertex_color;

void main()
{
    //constants c = reference.c;

    //vertex v = vb_heap[c.index].vertices[gl_VertexIndex];
    vertices vb = vertices(reference.v);
    vertex v = vb.vertex_buffer[gl_VertexIndex];

    vec3 position = v.position.xyz;
    //position.x += c.t;

    gl_Position = vec4(position,1);

    vertex_color = v.color.rgb;
}
