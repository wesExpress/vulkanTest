#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_descriptor_heap : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location=0) out vec3 vertex_color;

struct vertex
{
    vec4 position;
    vec4 color;
};

layout (descriptor_heap) buffer vbheap
{
    vertex vertices[];
} vertex_buffer_heap[];

layout (buffer_reference) readonly buffer push_refernce
{
    uint  vb_index;
    float t;
} indices;

layout (push_constant) uniform push_constants
{
    push_refernce ref; 
} constants;

void main()
{
    push_refernce ref = constants.ref;

    uint index = 0;
    index = ref.vb_index;
    vertex v = vertex_buffer_heap[nonuniformEXT(index)].vertices[nonuniformEXT(gl_VertexIndex)];

    vec3 position = v.position.xyz;
    position.x += ref.t;

    vec3 color    = v.color.rgb; 

    gl_Position = vec4(position, 1.f);
    vertex_color = color;
}
