#version 430

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform float filler;
uniform uvec3 size;
uniform uint vecType;

layout (std430, binding = 1) buffer IndicesBuffer
{
    uvec4 Indices[];
};

layout (rgba32f) restrict uniform image2D Buffer;

void main()
{
    uint index = gl_GlobalInvocationID.x * size.y * size.z + gl_GlobalInvocationID.y * size.z +
                        gl_GlobalInvocationID.z;
    if(vecType == 0)
    {
        ivec2 coordinates = ivec2(gl_WorkGroupID.x, gl_WorkGroupID.y);
        imageStore(Buffer, coordinates, vec4(filler));
        memoryBarrierImage();
    }
    else
    {
        Indices[gl_WorkGroupID.z] = uvec4(uint(filler), uint(filler), uint(filler), 0);
        memoryBarrierBuffer();
    }
}