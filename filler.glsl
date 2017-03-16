#version 430

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform float filler;

layout (std430, binding = 2) buffer SDFBuffer
{
    vec4 SDF[];
};

void main()
{
    uint index = gl_GlobalInvocationID.x * gl_NumWorkGroups.y * gl_NumWorkGroups.z +
                 gl_GlobalInvocationID.y * gl_NumWorkGroups.z +
                 gl_GlobalInvocationID.z;
    SDF[index] = vec4(filler);
    memoryBarrierBuffer();
}