#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_descriptor_heap : require
#extension GL_EXT_nonuniform_qualifier : require

layout(local_size_x=1,local_size_y=1,local_size_z=1) in;

layout(descriptor_heap) readonly buffer junk
{
    vec3 a;
} heap[];

layout(buffer_reference) readonly buffer buf
{
    uint a[];
};

layout(push_constant) uniform push_constants
{
    buf b;
} constants;

void main()
{
    buf b = constants.b;
    uint i = b.a[0].x;

    //vec3 a = heap[nonuniformEXT(d)].a;
    //vec3 a = b.a[i];
    vec3 a = heap[nonuniformEXT(i)].a;
}
