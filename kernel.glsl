#version 450

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
uniform uint fullyInRange;
uniform uvec3 resolution;
uniform uint minIndex;
uniform uint maxIndex;

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

uniform Triangle triangle;

layout (std430, binding = 1) buffer SDFBuffer
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
    float distance = dot(inPoint - triangle.v1, triangle.n);
    float sgn = sign(distance);
    sgn = sgn == 0 ? 1.0 : sgn;
    vec3 projectedPoint = inPoint - distance * triangle.n;
    float areaQBC = 0.5 * length(cross(triangle.v2 - projectedPoint, triangle.v3 - projectedPoint));
    float areaQCA = 0.5 * length(cross(triangle.v3 - projectedPoint, triangle.v1 - projectedPoint));
    float areaQAB = 0.5 * length(cross(triangle.v1 - projectedPoint, triangle.v2 - projectedPoint));
    float areaABC = 0.5 * length(cross(triangle.v2 - triangle.v1, triangle.v3 - triangle.v1));
    float areaSummed = areaQAB + areaQBC + areaQCA;
    float denominator = pow(length(triangle.v2 - triangle.v1), 2);
    float uAB = dot(triangle.v2 - projectedPoint, triangle.v2 - triangle.v1) / denominator;
    float vAB = dot(projectedPoint - triangle.v1, triangle.v2 - triangle.v1) / denominator;
    denominator = pow(length(triangle.v3 - triangle.v2), 2);
    float uBC = dot(triangle.v3 - projectedPoint, triangle.v3 - triangle.v2) / denominator;
    float vBC = dot(projectedPoint - triangle.v2, triangle.v3 - triangle.v2) / denominator;
    denominator = pow(length(triangle.v3 - triangle.v1), 2);
    float uAC = dot(triangle.v3 - projectedPoint, triangle.v3 - triangle.v1) / denominator;
    float vAC = dot(projectedPoint - triangle.v1, triangle.v3 - triangle.v1) / denominator;
    if(areaABC != areaSummed)
    {
        if(uAB > 0 && vAB > 0)
            distance = distanceToEdge(triangle.v1, triangle.v2, inPoint) * sgn;
        else
        {
            if(uBC > 0 && vBC > 0)
                distance = distanceToEdge(triangle.v2, triangle.v3, inPoint) * sgn;
            else
            {
                if(uAC > 0 && vAC > 0)
                    distance = distanceToEdge(triangle.v1, triangle.v3, inPoint) * sgn;
                else
                {
                    float distToV1 = length(inPoint - triangle.v1);
                    float distToV2 = length(inPoint - triangle.v2);
                    float distToV3 = length(inPoint - triangle.v3);
                    distance = min(distToV1, min(distToV2, distToV3)) * sgn;
                }
            }
        }
    }
    inIndex -= minIndex;
    float distValue = SDF[inIndex].w;
    if(abs(distance) < abs(distValue))
    {
        SDF[inIndex] = vec4(inPoint.x, inPoint.y, inPoint.z, distance);
        memoryBarrierBuffer();
    }
}

void main()
{
    vec3 point = vec3(triangle.v2.x + gl_WorkGroupID.x * triangle.n.x,
                      triangle.v2.y + gl_WorkGroupID.y * triangle.n.y,
                      triangle.v2.z + gl_WorkGroupID.z * triangle.n.z);
    uint index = uint((point.x - triangle.v3.x) / triangle.n.x) * resolution.y * resolution.z +
                 uint((point.y - triangle.v3.y) / triangle.n.y) * resolution.z +
                 uint((point.z - triangle.v3.z) / triangle.n.z);
    if(fullyInRange == 1)
        computeDistance(point, index);
    else
    {
        if(index <= maxIndex)
            computeDistance(point, index);
    }
}