//
// Created by Y500 on 08.03.2017.
//

#include <bits/unique_ptr.h>
#include "Converter.h"
#include <GL/glut.h>
#include "../glm/gtc/type_ptr.inl"

Converter::Converter() : logStream(std::cout.rdbuf()), ready(false)
{
    SIZE_FACTOR = 4;
    TRIANGLE_SIZE = sizeof(Triangle);
    PRISM_AABB_SIZE = sizeof(PrismAABB);
    DISTANCE_AND_COORD_SIZE = sizeof(GLfloat);

    GLint value = 0;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &value);
    if(value < 3)
    {
        logStream << "CRITICAL PROBLEM:You has only " << value
                  << " SSBO binding points, while converter needs at least 3\n";
        ready = false;
        return;
    }
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &value);
    MAX_WORK_GROUP_COUNT = (uint16_t)value;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &value);
    MAX_TRIANGLES_SSBO_SIZE = (uint32_t)value / (SIZE_FACTOR * TRIANGLE_SIZE);
    MAX_PRISM_AABB_SSBO_SIZE = (uint32_t)value / (SIZE_FACTOR * PRISM_AABB_SIZE);
    MAX_SDF_SSBO_SIZE = (uint32_t)value / (SIZE_FACTOR * DISTANCE_AND_COORD_SIZE);
    triangles.setErrorStream(logStream);
    triangles.setType(GL_SHADER_STORAGE_BUFFER);
    prismAABBs.setErrorStream(logStream);
    prismAABBs.setType(GL_SHADER_STORAGE_BUFFER);
    sdf.setErrorStream(logStream);
    sdf.setType(GL_SHADER_STORAGE_BUFFER);
    filler.setErrorStream(logStream);
    modifier.setErrorStream(logStream);
    kernel.setErrorStream(logStream);
}

Converter::Converter(const std::ostream &inLogStream) : logStream(inLogStream.rdbuf())
{
    SIZE_FACTOR = 4;
    TRIANGLE_SIZE = sizeof(Triangle);
    PRISM_AABB_SIZE = sizeof(PrismAABB);
    DISTANCE_AND_COORD_SIZE = sizeof(GLfloat);

    GLint value = 0;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &value);
    if(value < 3)
    {
        logStream << "CRITICAL PROBLEM:You has only " << value
                  << " SSBO binding points, while converter needs at least 3\n";
        ready = false;
        return;
    }
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &value);
    MAX_WORK_GROUP_COUNT = (uint16_t)value;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &value);
    MAX_TRIANGLES_SSBO_SIZE = (uint32_t)value / (SIZE_FACTOR * TRIANGLE_SIZE);
    MAX_PRISM_AABB_SSBO_SIZE = (uint32_t)value / (SIZE_FACTOR * PRISM_AABB_SIZE);
    MAX_SDF_SSBO_SIZE = (uint32_t)value / (SIZE_FACTOR * DISTANCE_AND_COORD_SIZE);
    triangles.setErrorStream(logStream);
    triangles.setType(GL_SHADER_STORAGE_BUFFER);
    prismAABBs.setErrorStream(logStream);
    prismAABBs.setType(GL_SHADER_STORAGE_BUFFER);
    sdf.setErrorStream(logStream);
    sdf.setType(GL_SHADER_STORAGE_BUFFER);
    filler.setErrorStream(logStream);
    modifier.setErrorStream(logStream);
    kernel.setErrorStream(logStream);
}

Converter::~Converter()
{
    triangles.deleteBuffer();
    prismAABBs.deleteBuffer();
    sdf.deleteBuffer();

    filler.deleteProgram();
    modifier.deleteProgram();
    kernel.deleteProgram();
}

void Converter::setSizeFactor(uint16_t inSizeFactor)
{
    SIZE_FACTOR = inSizeFactor;
}

void Converter::setResolution(const glm::uvec3 &inResolution)
{
    resolution = inResolution;
}

void Converter::setShellMin(const glm::vec3 &inShellMin)
{
    shellMin = inShellMin;
}

void Converter::setShellMax(const glm::vec3 &inShellMax)
{
    shellMax = inShellMax;
}

void Converter::setFillerValue(GLfloat inFillerValue)
{
    fillerValue = inFillerValue;
}

bool Converter::loadFiller(const std::string &path)
{
    if(!filler.loadShaderFromFile(path, GL_COMPUTE_SHADER))
    {
        filler.printError();
        return false;
    }
    if(!filler.link())
    {
        filler.printError();
        return false;
    }
    return true;
}

bool Converter::loadModifier(const std::string &path)
{
    if(!modifier.loadShaderFromFile(path, GL_COMPUTE_SHADER))
    {
        modifier.printError();
        return false;
    }
    if(!modifier.link())
    {
        modifier.printError();
        return false;
    }
    return true;
}

bool Converter::loadKernel(const std::string &path)
{
    if(!kernel.loadShaderFromFile(path, GL_COMPUTE_SHADER))
    {
        kernel.printError();
        return false;
    }
    if(!kernel.link())
    {
        kernel.printError();
        return false;
    }
    return true;
}

void Converter::computeDistanceField(const std::vector<Triangle> &inTriangles)
{
    uint16_t i = 0;
    uint16_t triangleCycles = (uint16_t)ceil((double)inTriangles.size() / MAX_TRIANGLES_SSBO_SIZE);
    uint32_t trianglesSize = triangleCycles > 1 ? MAX_TRIANGLES_SSBO_SIZE : inTriangles.size();

    std::vector<Triangle>::const_iterator startPosition = inTriangles.begin();
    triangles.bind();
    triangles.data(trianglesSize * TRIANGLE_SIZE, &(*startPosition), GL_STATIC_DRAW);
    triangles.bindBase(0);

    prismAABBs.bind();
    prismAABBs.data(trianglesSize * PRISM_AABB_SIZE, NULL, GL_STATIC_DRAW);
    prismAABBs.bindBase(1);

    for(i = 0; i < triangleCycles; ++i)
    {
        modifier.use();
        modifier.bindUniformVector(SP_VEC3, "shellMin", glm::value_ptr(shellMin));
        modifier.bindUniformVector(SP_VEC3, "step", glm::value_ptr(glm::vec3((shellMax.x - shellMin.x) / resolution.x,
                                                                             (shellMax.y - shellMin.y) / resolution.y,
                                                                             (shellMax.z - shellMin.z) / resolution.z)));
        modifier.bindUniformVector(SP_UVEC3, "resolution", glm::value_ptr(resolution));
        modifier.bindUniform("epsilon", 0.01f);
        glDispatchCompute(1, 1, 1024);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        computeDistance(trianglesSize);
        startPosition += MAX_TRIANGLES_SSBO_SIZE;
    }
}

void Converter::openFiles(const uint16_t &index)
{
    std::string distancesFilename = "distanceValues_";
    stringStream << distancesFilename << index;
    sdfFile.open(stringStream.str(), std::ios_base::binary | std::ios_base::out);
    stringStream.str("");
}

void Converter::computeDistance(const uint32_t &inTriangleSize)
{
    uint32_t sdfSize = resolution.x * resolution.y * resolution.z < MAX_SDF_SSBO_SIZE ?
                       resolution.x * resolution.y * resolution.z :
                       MAX_SDF_SSBO_SIZE;
    sdf.bind();
    sdf.data(sdfSize * DISTANCE_AND_COORD_SIZE, NULL, GL_STATIC_DRAW);
    sdf.bindBase(2);

    prismAABBs.bind();
    PrismAABB *prismAABBPointer = (PrismAABB*)prismAABBs.map(GL_READ_ONLY);
    uint32_t minIndex = (uint32_t)prismAABBPointer[0].minIndex;
    uint32_t maxIndex = (uint32_t)prismAABBPointer[0].maxIndex;
    for(uint32_t i = 1; i < inTriangleSize; ++i)
    {
        if(prismAABBPointer[i].minIndex < minIndex)
            minIndex = prismAABBPointer[i].minIndex;
        if(prismAABBPointer[i].maxIndex > maxIndex)
            maxIndex = prismAABBPointer[i].maxIndex;
    }
    uint32_t sdfMaxIndex = sdfSize + minIndex - 1;
    std::vector<uint32_t> skippedIndices(0);
    uint16_t pointsAmount = 0;
    for(uint32_t i = 0; i < inTriangleSize; ++i)
    {
        if(prismAABBPointer[i].minIndex < sdfMaxIndex)
        {
            kernel.use();
            kernel.bindUniformVector(SP_UVEC3, "resolution", glm::value_ptr(resolution));
            kernel.bindUniformVector(SP_VEC3, "shellMin", glm::value_ptr(shellMin));
            kernel.bindUniformui("minIndex", minIndex);
            kernel.bindUniformui("maxIndex", maxIndex);
            kernel.bindUniformui("aabbAndTriangleIndex", i);
            if(prismAABBPointer[i].maxIndex < sdfMaxIndex)
                kernel.bindUniformui("fullyInRange", 1);
            else
            {
                kernel.bindUniformui("fullyInRange", 0);
                skippedIndices.push_back(i);
            }
            pointsAmount = (uint16_t)prismAABBPointer[i].totalSize;
            prismAABBs.unmap();
            glDispatchCompute(1, 1, pointsAmount);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            kernel.unuse();
            prismAABBPointer = (PrismAABB*)prismAABBs.map(GL_READ_ONLY);
        }
        else
            skippedIndices.push_back(i);
    }
}

void Converter::writeFiles()
{

}
