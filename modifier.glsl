#version 450

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
uniform vec3 shellMin;
uniform vec3 step;
uniform uvec3 resolution;
uniform float epsilon;

struct Triangle
{
    vec3 v1;
    float padding3;
    vec3 v2;
    float padding7;
    vec3 v3;
    float padding11;
    vec3 n;
    float padding15;
};

struct PrismAABB
{
    vec3 min;
    float totalSize;
    vec3 pointsAmount;
    float minIndex;
    vec3 step;
    float maxIndex;
};

layout (std140, binding = 0) buffer TrianglesBuffer
{
    Triangle Triangles[];
};

layout (std140, binding = 1) buffer PrismAABBBuffer
{
    PrismAABB PrismAABBs[];
};

void constructPrismAABB(in Triangle t, out vec3 AABBmin, out vec3 AABBmax)
{
    vec3 displacement = epsilon * t.n;
    vec3 plusFaceV1 = t.v1 + displacement;
    vec3 plusFaceV2 = t.v2 + displacement;
    vec3 plusFaceV3 = t.v3 + displacement;
    vec3 minusFaceV1 = t.v1 - displacement;
    vec3 minusFaceV2 = t.v2 - displacement;
    vec3 minusFaceV3 = t.v3 - displacement;
    vec3 minMinusFace = min(minusFaceV1, min(minusFaceV2, minusFaceV3));
    vec3 maxMinusFace = max(minusFaceV1, max(minusFaceV2, minusFaceV3));
    vec3 minPlusFace = min(plusFaceV1, min(plusFaceV2, plusFaceV3));
    vec3 maxPlusFace = max(plusFaceV1, max(plusFaceV2, plusFaceV3));
    AABBmin = min(minMinusFace, minPlusFace);
    AABBmax = max(maxMinusFace, maxPlusFace);
}

void main()
{
    uint index = gl_WorkGroupID.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z + gl_WorkGroupID.y * gl_WorkGroupSize.z +
                 gl_WorkGroupID.z;
    Triangle triangle = Triangles[index];
    PrismAABB prismAABB = PrismAABBs[index];
    vec3 prismAABBmin = vec3(0, 0, 0);
    vec3 prismAABBmax = vec3(0, 0, 0);
    constructPrismAABB(triangle, prismAABBmin, prismAABBmax);
    vec3 aabbSize = vec3(prismAABBmax.x - prismAABBmin.x, prismAABBmax.y - prismAABBmin.y,
                    prismAABBmax.z - prismAABBmin.z);
    vec3 pointsAmount = vec3(floor(aabbSize.x / step.x),
                             floor(aabbSize.y / step.y),
                             floor(aabbSize.z / step.z));
    float totalSize = pointsAmount.x * pointsAmount.y * pointsAmount.z;
    uint minIndex = uint((prismAABBmin.x - shellMin.x) / step.x) * resolution.y * resolution.z +
                    uint((prismAABBmin.y - shellMin.y) / step.y) * resolution.z +
                    uint((prismAABBmin.z - shellMin.z) / step.z);
    uint maxIndex = uint((prismAABBmax.x - shellMin.x) / step.x) * resolution.y * resolution.z +
                    uint((prismAABBmax.y - shellMin.y) / step.y) * resolution.z +
                    uint((prismAABBmax.z - shellMin.z) / step.z);
    prismAABB.min = prismAABBmin;
    prismAABB.pointsAmount = pointsAmount;
    prismAABB.step = step;
    prismAABB.totalSize = totalSize;
    prismAABB.minIndex = minIndex;
    prismAABB.maxIndex = maxIndex;
    PrismAABBs[index] = prismAABB;
    memoryBarrierBuffer();
}