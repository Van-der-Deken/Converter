//
// Created by Y500 on 08.03.2017.
//

#include <bits/unique_ptr.h>
#include "Converter.h"
#include <GL/glut.h>
#include "../glm/gtc/type_ptr.inl"
#include <sys/time.h>

Converter::Converter() : logStream(std::cout.rdbuf()), ready(false)
{
    SIZE_FACTOR = 4;
    TRIANGLE_SIZE = sizeof(Triangle);
    PRISM_AABB_SIZE = sizeof(PrismAABB);
    DISTANCE_AND_COORD_SIZE = 4 * sizeof(GLfloat);

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
    prismAABBs.data(trianglesSize * PRISM_AABB_SIZE, NULL, GL_DYNAMIC_READ);
    prismAABBs.bindBase(1);

    for(i = 0; i < triangleCycles; ++i)
    {
        modifier.use();
        modifier.bindUniformVector(SP_VEC3, "shellMin", glm::value_ptr(shellMin));
        modifier.bindUniformVector(SP_VEC3, "step", glm::value_ptr(glm::vec3((shellMax.x - shellMin.x) / resolution.x,
                                                                             (shellMax.y - shellMin.y) / resolution.y,
                                                                             (shellMax.z - shellMin.z) / resolution.z)));
        modifier.bindUniformVector(SP_UVEC3, "resolution", glm::value_ptr(resolution));
        modifier.bindUniform("epsilon", 0.001f);
        glDispatchCompute(1, 1, 1024);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        openFiles(i);
        computeDistance(trianglesSize);
        writeFiles();
        startPosition += MAX_TRIANGLES_SSBO_SIZE;
    }
    std::cout << "Time:" << time << std::endl;
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
    GLsync sync;
    sdfSize = resolution.x * resolution.y * resolution.z < MAX_SDF_SSBO_SIZE ?
                       resolution.x * resolution.y * resolution.z :
                       MAX_SDF_SSBO_SIZE;
    sdf.bind();
    sdf.data(sdfSize * DISTANCE_AND_COORD_SIZE, NULL, GL_DYNAMIC_READ);
    sdf.bindBase(2);

    prismAABBs.bind();
    PrismAABB *prismAABBPointer = (PrismAABB*)prismAABBs.map(GL_READ_ONLY);
    std::vector<uint32_t> minIndices(0);
    std::vector<uint32_t> maxIndices(0);
    std::vector<glm::vec3> pointsAmount(0);
    uint32_t minIndex = (uint32_t)prismAABBPointer[0].minIndex;
    uint32_t maxIndex = (uint32_t)prismAABBPointer[0].maxIndex;
    minIndices.push_back(minIndex);
    maxIndices.push_back(maxIndex);
    pointsAmount.push_back(glm::vec3(prismAABBPointer[0].pointsAmount[0],
                                     prismAABBPointer[0].pointsAmount[1],
                                     prismAABBPointer[0].pointsAmount[2]));
    for(uint32_t i = 1; i < inTriangleSize; ++i)
    {
        if(prismAABBPointer[i].minIndex < minIndex)
            minIndex = prismAABBPointer[i].minIndex;
        if(prismAABBPointer[i].maxIndex > maxIndex)
            maxIndex = prismAABBPointer[i].maxIndex;
        minIndices.push_back(prismAABBPointer[i].minIndex);
        maxIndices.push_back(prismAABBPointer[i].maxIndex);
        pointsAmount.push_back(glm::vec3(prismAABBPointer[i].pointsAmount[0],
                                         prismAABBPointer[i].pointsAmount[1],
                                         prismAABBPointer[i].pointsAmount[2]));
    }
    prismAABBs.unmap();
    uint32_t sdfMaxIndex = sdfSize + minIndex - 1;
    std::vector<uint32_t> skippedIndices(0);
    filler.use();
    filler.bindUniform("filler", fillerValue);
    glDispatchCompute(resolution.x, resolution.y, resolution.z);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
//    sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
//    glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000);
    struct timeval t;
    gettimeofday(&t, NULL);
    time = (uint64_t)t.tv_sec * 1000 + t.tv_usec / 1000;
    for(uint32_t i = 0; i < inTriangleSize; ++i)
    {
        if(minIndices[i] < sdfMaxIndex)
        {
            kernel.use();
            kernel.bindUniformVector(SP_UVEC3, "resolution", glm::value_ptr(resolution));
            kernel.bindUniformVector(SP_VEC3, "shellMin", glm::value_ptr(shellMin));
            kernel.bindUniformui("minIndex", minIndex);
            kernel.bindUniformui("maxIndex", maxIndex);
            kernel.bindUniformui("aabbAndTriangleIndex", i);
            if(maxIndices[i] < sdfMaxIndex)
                kernel.bindUniformui("fullyInRange", 1);
            else
            {
                kernel.bindUniformui("fullyInRange", 0);
                skippedIndices.push_back(i);
            }
            glDispatchCompute((GLuint)pointsAmount[i].x, (GLuint)pointsAmount[i].y, (GLuint)pointsAmount[i].z);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            kernel.unuse();
//            sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
//            glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000);
        }
        else
            skippedIndices.push_back(i);
    }
    std::cout << "Error:" << glGetError() << std::endl;
}

void Converter::writeFiles()
{
    sdf.bind();
    GLfloat *sdfPointer = (GLfloat*)sdf.map(GL_READ_ONLY);
    struct timeval t;
    gettimeofday(&t, NULL);
    time = (uint64_t)t.tv_sec * 1000 + t.tv_usec / 1000 - time;
    uint32_t realSize = 0;
    std::vector<GLfloat> data(0);
    for(uint32_t i = 0; i < sdfSize; ++i)
        if(sdfPointer[i * 4 + 3] != fillerValue)
            for(short j = 0; j < 4; ++j)
                data.push_back(sdfPointer[i * 4 + j]);
    realSize = (uint32_t)data.size();
    sdfFile.write(reinterpret_cast<char*>(&realSize), sizeof(uint32_t));
    for(uint32_t i = 0; i < realSize; ++i)
        sdfFile.write(reinterpret_cast<char*>(&data[i]), sizeof(GLfloat));
    sdfFile.close();
    data.clear();
    std::cout << realSize << std::endl;
}
