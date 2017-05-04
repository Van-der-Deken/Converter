#version 450

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
uniform vec3 shellMin;
uniform vec3 step;
uniform uvec3 resolution;
uniform float epsilon;

struct Triangle
{
    vec4 v1;
    vec4 v2;
    vec4 v3;
    vec4 n;
};

layout (std430, binding = 0) buffer TrianglesBuffer
{
    Triangle Triangles[];
};

layout (std430, binding = 1) buffer PrismAABBBuffer
{
    vec4 aabbMin[];
};

void constructPrismAABB(in Triangle t, out vec3 AABBmin, out vec3 AABBmax)
{
    vec3 displacement = epsilon * t.n.xyz;
    vec3 minPoint, maxPoint;
    minPoint.x = min(t.v1.x, min(t.v2.x, t.v3.x));
    minPoint.y = min(t.v1.y, min(t.v2.y, t.v3.y));
    minPoint.z = min(t.v1.z, min(t.v2.z, t.v3.z));
    maxPoint.x = max(t.v1.x, max(t.v2.x, t.v3.x));
    maxPoint.y = max(t.v1.y, max(t.v2.y, t.v3.y));
    maxPoint.z = max(t.v1.z, max(t.v2.z, t.v3.z));

    vec3 plusFaceMin = minPoint + displacement;
    vec3 plusFaceMax = maxPoint + displacement;
    vec3 minusFaceMin = minPoint - displacement;
    vec3 minusFaceMax = maxPoint - displacement;

    AABBmin = min(plusFaceMin, minusFaceMin);
    AABBmax = max(plusFaceMax, minusFaceMax);
}

void main()
{
    uint index = gl_WorkGroupID.x * gl_NumWorkGroups.y * gl_NumWorkGroups.z +
                 gl_WorkGroupID.y * gl_NumWorkGroups.z +
                 gl_WorkGroupID.z;
    Triangle triangle = Triangles[index];
    vec3 prismAABBmin = vec3(0, 0, 0);
    vec3 prismAABBmax = vec3(0, 0, 0);
    constructPrismAABB(triangle, prismAABBmin, prismAABBmax);
    prismAABBmin = vec3(ceil((prismAABBmin.x - shellMin.x) / step.x) * step.x + shellMin.x,
                        ceil((prismAABBmin.y - shellMin.y) / step.y) * step.y + shellMin.y,
                        ceil((prismAABBmin.z - shellMin.z) / step.z) * step.z + shellMin.z);
    prismAABBmax = vec3(floor((prismAABBmax.x - shellMin.x) / step.x) * step.x + shellMin.x,
                        floor((prismAABBmax.y - shellMin.y) / step.y) * step.y + shellMin.y,
                        floor((prismAABBmax.z - shellMin.z) / step.z) * step.z + shellMin.z);
    triangle.v1.w = abs(floor((prismAABBmax.x - shellMin.x) / step.x) -
                        ceil((prismAABBmin.x - shellMin.x) / step.x));
    triangle.v2.w = abs(floor((prismAABBmax.y - shellMin.y) / step.y) -
                        ceil((prismAABBmin.y - shellMin.y) / step.y));
    triangle.v3.w = abs(floor((prismAABBmax.z - shellMin.z) / step.z) -
                        ceil((prismAABBmin.z - shellMin.z) / step.z));
    uint maxIndex = uint((prismAABBmax.x - shellMin.x) / step.x) * resolution.y * resolution.z +
                    uint((prismAABBmax.y - shellMin.y) / step.y) * resolution.z +
                    uint((prismAABBmax.z - shellMin.z) / step.z);
    aabbMin[index] = vec4(prismAABBmin, 0.0);
    triangle.n.w = maxIndex > (resolution.x * resolution.y * resolution.z) ? 1.0 : 0.0;
    Triangles[index] = triangle;
    memoryBarrierBuffer();
}