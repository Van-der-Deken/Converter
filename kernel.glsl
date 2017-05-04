#version 450

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
uniform uint fullyInRange;
uniform vec3 shellMin;
uniform vec3 step;
uniform uvec3 resolution;
uniform uint maxIndex;
uniform uint aabbAndTriangleIndex;

struct Triangle
{
    vec4 v1;
    vec4 v2;
    vec4 v3;
    vec4 n;
};

layout (std140, binding = 0) buffer TrianglesBuffer
{
    Triangle Triangles[];
};

layout (std140, binding = 1) buffer PrismAABBBuffer
{
    vec4 aabbMin[];
};

layout (std430, binding = 2) coherent buffer SDFBuffer
{
    vec4 SDF[];
};

float distanceToEdge(in vec3 P0, in vec3 P1, in vec3 P)
{
    vec3 v = P1 - P0;
    vec3 w = P - P0;
    float c1 = dot(w, v);
    float c2 = dot(v, v);
    float b = c1 / c2;
    return length(P - (P0 + b * v));
}

void computeDistance(in vec3 inPoint, in uint inIndex)
{
    Triangle triangle = Triangles[aabbAndTriangleIndex];
    float distance = dot(inPoint - triangle.v1.xyz, triangle.n.xyz);
    float sgn = sign(distance);
    sgn = sgn == 0 ? 1.0 : sgn;
    float distToV1 = length(inPoint - triangle.v1.xyz);
    float distToV2 = length(inPoint - triangle.v2.xyz);
    float distToV3 = length(inPoint - triangle.v3.xyz);
    float minDistToVertex = min(distToV1, min(distToV2, distToV3));
    float distToE1 = distanceToEdge(triangle.v1.xyz, triangle.v2.xyz, inPoint);
    float distToE2 = distanceToEdge(triangle.v2.xyz, triangle.v3.xyz, inPoint);
    float distToE3 = distanceToEdge(triangle.v1.xyz, triangle.v3.xyz, inPoint);
    float minDistToEdge = min(distToE1, min(distToE2, distToE3));
    distance = min(abs(distance), min(minDistToVertex, minDistToEdge)) * sgn;
    float distValue = SDF[inIndex].w;
    SDF[inIndex] = vec4(inPoint.x, inPoint.y, inPoint.z, min(abs(distance), abs(distValue)));
    memoryBarrierBuffer();
}

void main()
{
    vec3 point = vec3(aabbMin[aabbAndTriangleIndex]) + vec3(step.x * gl_WorkGroupID.x,
                                                            step.y * gl_WorkGroupID.y,
                                                            step.z * gl_WorkGroupID.z);
    uint index = uint((point.x - shellMin.x) / step.x) * resolution.y * resolution.z +
                 uint((point.y - shellMin.y) / step.y) * resolution.z +
                 uint((point.z - shellMin.z) / step.z);
    if(fullyInRange == 1)
        computeDistance(point, index);
    else
    {
        if(index <= maxIndex)
            computeDistance(point, index);
    }
}