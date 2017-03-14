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
    DISTANCE_AND_COORD_SIZE = sizeof(GLfloat);

    GLint value = 0;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &value);
    if(value < 2)
    {
        logStream << "CRITICAL PROBLEM:You has only " << value
                  << " SSBO binding points, while converter needs at least 2\n";
        ready = false;
        return;
    }
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &value);
    MAX_WORK_GROUP_COUNT = (uint16_t)value;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &value);
    MAX_TRIANGLES_SSBO_SIZE = (uint32_t)value / (SIZE_FACTOR * TRIANGLE_SIZE);
    triangles.setErrorStream(logStream);
    triangles.setType(GL_SHADER_STORAGE_BUFFER);
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
    DISTANCE_AND_COORD_SIZE = sizeof(GLfloat);

    GLint value = 0;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &value);
    if(value < 2)
    {
        logStream << "CRITICAL PROBLEM:You has only " << value
                  << " SSBO binding points, while converter needs at least 2\n";
        ready = false;
        return;
    }
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &value);
    MAX_WORK_GROUP_COUNT = (uint16_t)value;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &value);
    MAX_TRIANGLES_SSBO_SIZE = (uint32_t)value / (SIZE_FACTOR * TRIANGLE_SIZE);
    triangles.setErrorStream(logStream);
    triangles.setType(GL_SHADER_STORAGE_BUFFER);
    sdf.setErrorStream(logStream);
    sdf.setType(GL_SHADER_STORAGE_BUFFER);
    filler.setErrorStream(logStream);
    modifier.setErrorStream(logStream);
    kernel.setErrorStream(logStream);
}

Converter::~Converter()
{
    triangles.deleteBuffer();
    sdf.deleteBuffer();

    filler.deleteProgram();
    modifier.deleteProgram();
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
    triangles.data(trianglesSize * TRIANGLE_SIZE, NULL, GL_STATIC_DRAW);
    triangles.bindBase(0);

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
        computeDistance();
    }
}

void Converter::openFiles(const uint16_t &index)
{
    std::string distancesFilename = "distanceValues_";
    stringStream << distancesFilename << index;
    sdfFile.open(stringStream.str(), std::ios_base::binary | std::ios_base::out);
    stringStream.str("");
}

void Converter::computeDistance()
{

}

void Converter::writeFiles()
{

}
