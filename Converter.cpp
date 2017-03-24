//
// Created by Y500 on 08.03.2017.
//

#include <bits/unique_ptr.h>
#include "Converter.h"
#include <GL/glut.h>
#include "../glm/gtc/type_ptr.inl"

Converter::Converter() : logStream(std::cout.rdbuf())
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

void Converter::setSdfFilename(const std::string &path)
{
    sdfFilename = path;
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

void Converter::setDelta(GLfloat inDelta)
{
    delta = inDelta;
}

void Converter::setMaxResolution()
{
    glm::vec3 lengthes(shellMax.x - shellMin.x, shellMax.y - shellMin.y, shellMax.z - shellMin.z);
    GLfloat minLength = glm::min(lengthes.x, glm::min(lengthes.y, lengthes.z));
    lengthes /= minLength;
    uint16_t totalParts = glm::round(lengthes.x) * glm::round(lengthes.y) * glm::round(lengthes.z);
    if(totalParts == 0)
        logStream << "setMaxResolution must called after setShellMin and setShellMax\n";
    uint16_t partSize = (uint16_t)glm::floor(glm::pow((double)MAX_SDF_SSBO_SIZE / (1 * totalParts), 0.333333/*power 1/3*/));
    resolution = (glm::uvec3(glm::round(lengthes.x), glm::round(lengthes.y), glm::round(lengthes.z)) *= partSize);
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
    std::cout << "Resolution:" << resolution.x << "," << resolution.y << "," << resolution.z << std::endl;
    sdfGenerating.start();
    uint16_t triangleCycles = (uint16_t)ceil((double)inTriangles.size() / MAX_TRIANGLES_SSBO_SIZE);
    uint32_t trianglesSize = triangleCycles > 1 ? MAX_TRIANGLES_SSBO_SIZE : inTriangles.size();

    std::vector<Triangle>::const_iterator startPosition = inTriangles.begin();
    triangles.bind();
    triangles.data(trianglesSize * TRIANGLE_SIZE, &(*startPosition), GL_STATIC_DRAW);
    triangles.bindBase(0);

    prismAABBs.bind();
    prismAABBs.data(trianglesSize * PRISM_AABB_SIZE, NULL, GL_DYNAMIC_READ);
    prismAABBs.bindBase(1);

    modifier.use();
    modifier.bindUniformVector(SP_VEC3, "shellMin", glm::value_ptr(shellMin));
    modifier.bindUniformVector(SP_VEC3, "step", glm::value_ptr(glm::vec3((shellMax.x - shellMin.x) / resolution.x,
                                                                         (shellMax.y - shellMin.y) / resolution.y,
                                                                         (shellMax.z - shellMin.z) / resolution.z)));
    modifier.bindUniformVector(SP_UVEC3, "resolution", glm::value_ptr(resolution));
    modifier.bindUniform("delta", delta);
    glm::uvec3 groups = computeGroups(trianglesSize);
    glDispatchCompute(groups.x, groups.y, groups.z);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    openFile();
    computeDistance(trianglesSize);
    writeFile();
    logStream << "Time, spent on SDF computing:" << sdfGenerating.getPeriod() << " milliseconds" << std::endl;
    logStream << "Time, spent on SDF writing:" << fileWriting.getPeriod() << " milliseconds" << std::endl;
}

void Converter::openFile()
{
    sdfFile.open(sdfFilename, std::ios_base::binary | std::ios_base::out);
}

void Converter::computeDistance(const uint32_t &inTriangleSize)
{
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
    uint32_t partialySkipped = 0;
    filler.use();
    filler.bindUniform("filler", fillerValue);
    glDispatchCompute(resolution.x, resolution.y, resolution.z);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    kernel.use();
    kernel.bindUniformVector(SP_UVEC3, "resolution", glm::value_ptr(resolution));
    kernel.bindUniformVector(SP_VEC3, "shellMin", glm::value_ptr(shellMin));
    kernel.bindUniformui("minIndex", minIndex);
    kernel.bindUniformui("maxIndex", maxIndex);
    for(uint32_t i = 0; i < inTriangleSize; ++i)
    {
        if(minIndices[i] < sdfMaxIndex)
        {
            kernel.bindUniformui("aabbAndTriangleIndex", i);
            if(maxIndices[i] < sdfMaxIndex)
                kernel.bindUniformui("fullyInRange", 1);
            else
            {
                kernel.bindUniformui("fullyInRange", 0);
                ++partialySkipped;
            }
            glDispatchCompute((GLuint)pointsAmount[i].x, (GLuint)pointsAmount[i].y, (GLuint)pointsAmount[i].z);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }
    }
    kernel.unuse();
    logStream << "Partially skipped prisms:" << partialySkipped << std::endl;
}

void Converter::writeFile()
{
    sdf.bind();
    GLfloat *sdfPointer = (GLfloat*)sdf.map(GL_READ_ONLY);
    sdfGenerating.end();
    fileWriting.start();
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
    fileWriting.end();
    data.clear();
    std::cout << "Points in SDF:" << realSize / 4 << std::endl;
}

glm::uvec3 Converter::computeGroups(const uint32_t &inTrianglesAmount)
{
    glm::uvec3 groups(1, 1, 1);
    if(inTrianglesAmount <= MAX_WORK_GROUP_COUNT)
        groups.z = inTrianglesAmount;
    else
    {
        uint32_t divisor = MAX_WORK_GROUP_COUNT;
        uint32_t divident = inTrianglesAmount;
        for(short i = 0; i < 3; ++i)
        {
            while((divident % divisor != 0) && (divisor > 1) && (divident > MAX_WORK_GROUP_COUNT))
                --divisor;
            switch(i)
            {
                case 0:
                    groups.z = divisor;
                    break;
                case 1:
                    groups.y = divisor;
                    break;
                case 2:
                    groups.x = divisor;
                    break;
            }
            divident /= divisor;
            if(divident <= MAX_WORK_GROUP_COUNT)
                divisor = divident;
            else
                divisor = MAX_WORK_GROUP_COUNT;
        }
    }
    return groups;
}
