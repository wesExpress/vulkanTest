#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_descriptor_heap : require
#extension GL_EXT_nonuniform_qualifier : require

layout(local_size_x=1,local_size_y=1,local_size_z=1) in;

layout(descriptor_heap) buffer junk
{
    vec3 a;
} heap[];

void main()
{
}
