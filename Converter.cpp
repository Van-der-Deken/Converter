#include "Converter.h"
#include <sstream>
#include <string>
#include <future>
#include "../glm/gtc/type_ptr.inl"
#include "ObjTrianglesLoader.h"

using namespace conv;

Converter::Converter()
{
    GLint value = 0;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &value);
    if(value < 3)
    {
        logStream << "CRITICAL PROBLEM:You has only " << value
                  << " SSBO binding points, while converter needs at least 3. Terminating\n";
        return;
    }
    GLenum error = glGetError();
    if(error != GL_NO_ERROR)
    {
        logStream << "OpenGL error " << error
                  << "occurred. Terminating\n";
        return;
    }
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &value);
    MAX_WORK_GROUP_COUNT = (uint16_t)value;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &value);
    MAX_ALLOWED_SSBO_SIZE = (uint32_t)value;
    MAX_TRIANGLES_SSBO_SIZE = (uint32_t)value / (SIZE_FACTOR * TRIANGLE_SIZE);
    MAX_PRISM_AABB_SSBO_SIZE = (uint32_t)value / (SIZE_FACTOR * PRISM_AABB_SIZE);
    MAX_SDF_SSBO_SIZE = (uint32_t)value / (SIZE_FACTOR * SDF_ELEMENT_SIZE);
    unconstructed = false;
}

Converter::Converter(const std::ostream &inLogStream) : logStream(inLogStream.rdbuf())
{
    GLint value = 0;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &value);
    if(value < 3)
    {
        logStream << "CRITICAL PROBLEM:You has only " << value
                  << " SSBO binding points, while converter needs at least 3. Terminating\n";
        return;
    }
    GLenum error = glGetError();
    if(error != GL_NO_ERROR)
    {
        logStream << "OpenGL error " << error
                  << "occurred. Terminating\n";
        return;
    }
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &value);
    MAX_WORK_GROUP_COUNT = (uint16_t)value;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &value);
    MAX_ALLOWED_SSBO_SIZE = (uint32_t)value;
    MAX_TRIANGLES_SSBO_SIZE = (uint32_t)value / (SIZE_FACTOR * TRIANGLE_SIZE);
    MAX_PRISM_AABB_SSBO_SIZE = (uint32_t)value / (SIZE_FACTOR * PRISM_AABB_SIZE);
    MAX_SDF_SSBO_SIZE = (uint32_t)value / (SIZE_FACTOR * SDF_ELEMENT_SIZE);

    triangles.setErrorStream(logStream);
    prismAABBs.setErrorStream(logStream);
    sdf.setErrorStream(logStream);
    filler.setErrorStream(logStream);
    modifier.setErrorStream(logStream);
    kernel.setErrorStream(logStream);
    unconstructed = false;
}

Converter::~Converter()
{
    triangles.deleteBuffer();
    prismAABBs.deleteBuffer();
    sdf.deleteBuffer();

    filler.deleteProgram();
    modifier.deleteProgram();
    kernel.deleteProgram();

    if(loaderCreated)
        delete loader;
}

void Converter::initialize(const Initializer &initializer)
{
    if(unconstructed)
        return;
    SIZE_FACTOR = initializer.sizeFactor == 0 ? maxSizeFactor() : initializer.sizeFactor;
    if(SIZE_FACTOR <= 1.0f)
    {
        logStream << "Size factor " << SIZE_FACTOR
                  << " less than or equal to 1. Set to default value 2\n";
        SIZE_FACTOR = 2.0f;
    }
    sdfFilename = initializer.SDFfilename;
    resolution = initializer.resolution;
    loader = initializer.triaglesLoader;
    stopwatch = initializer.stopwatch;
    fillerValue = initializer.fillerValue;
    delta = initializer.delta;
    if(!filler.loadShaderFromFile(initializer.fillerPath, GL_COMPUTE_SHADER))
        filler.printError();
    if(!filler.link())
        filler.printError();
    if(!modifier.loadShaderFromFile(initializer.modifierPath, GL_COMPUTE_SHADER))
        modifier.printError();
    if(!modifier.link())
        modifier.printError();
    if(!kernel.loadShaderFromFile(initializer.kernelPath, GL_COMPUTE_SHADER))
        kernel.printError();
    if(!kernel.link())
        kernel.printError();
    uninitialized = false;
}

void Converter::compute()
{
    if(uninitialized)
    {
        logStream << "You must call initialize() before calling compute\n";
        return;
    }
    if(stopwatch != nullptr)
        stopwatch->start();
    if(loader == nullptr)
    {
        logStream << "Model file loader not specified. Terminating\n";
        return;
    }
    else
        loader->load();
    if(stopwatch != nullptr)
        stopwatch->lap();

    auto modelTriangles = loader->getTriangles();
    uint32_t trianglesSSBOSize = modelTriangles.size() < MAX_TRIANGLES_SSBO_SIZE ? modelTriangles.size() : MAX_TRIANGLES_SSBO_SIZE;
    auto startPosition = modelTriangles.begin();

    triangles.bind();
    triangles.data(TRIANGLE_SIZE * trianglesSSBOSize, &(*startPosition), GL_STATIC_DRAW);
    triangles.bindBase(0);

    prismAABBs.bind();
    prismAABBs.data(PRISM_AABB_SIZE * trianglesSSBOSize, NULL, GL_DYNAMIC_DRAW);
    prismAABBs.bindBase(1);

    if(resolution.x == 0)
        resolution = maxResolution();
    glm::vec3 shellMin = loader->getAABBMin();
    glm::vec3 shellMax = loader->getAABBMax();
    glm::vec3 step((shellMax.x - shellMin.x) / resolution.x,
                   (shellMax.y - shellMin.y) / resolution.y,
                   (shellMax.z - shellMin.z) / resolution.z);
    GLfloat epsilon = glm::length(step) * delta;
    GLuint maxIndex = resolution.x * resolution.y * resolution.z;

    modifier.use();
    modifier.bindUniformVector(UTypes::VEC3, "shellMin", glm::value_ptr(shellMin));
    modifier.bindUniformVector(UTypes::VEC3, "step", glm::value_ptr(step));
    modifier.bindUniformVector(UTypes::UVEC3, "resolution", glm::value_ptr(resolution));
    modifier.bindUniform("epsilon", epsilon);

    glm::uvec3 groups = computeGroups(trianglesSSBOSize);
    if(stopwatch != nullptr)
        stopwatch->lap();
    glDispatchCompute(groups.x, groups.y, groups.z);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    SDFSize = resolution.x * resolution.y * resolution.z < MAX_SDF_SSBO_SIZE ?
                       resolution.x * resolution.y * resolution.z :
                       MAX_SDF_SSBO_SIZE;
    sdf.bind();
    sdf.data(SDFSize * SDF_ELEMENT_SIZE, NULL, GL_DYNAMIC_READ);
    sdf.bindBase(2);

    std::vector<glm::uvec4> pointsAmount;
    triangles.bind();
    ModelTriangle* trianglePointer = (ModelTriangle*)triangles.map(GL_READ_ONLY);
    for(uint32_t i = 0; i < trianglesSSBOSize; ++i)
        pointsAmount.push_back(glm::uvec4((uint32_t)trianglePointer[i].firstVertex.w,
                                          (uint32_t)trianglePointer[i].secondVertex.w,
                                          (uint32_t)trianglePointer[i].thirdVertex.w,
                                          (uint32_t)trianglePointer[i].faceNormal.w));
    triangles.unmap();

    filler.use();
    filler.bindUniform("filler", fillerValue);
    glDispatchCompute(resolution.x, resolution.y, resolution.z);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    kernel.use();
    kernel.bindUniformVector(UTypes::UVEC3, "resolution", glm::value_ptr(resolution));
    kernel.bindUniformVector(UTypes::VEC3, "shellMin", glm::value_ptr(shellMin));
    kernel.bindUniformVector(UTypes::VEC3, "step", glm::value_ptr(step));
    kernel.bindUniformui("maxIndex", maxIndex);

    for(uint32_t i = 0; i < trianglesSSBOSize; ++i)
    {
        kernel.bindUniformui("aabbAndTriangleIndex", i);
        kernel.bindUniformui("fullyInRange", pointsAmount[i].w);
        glDispatchCompute((GLuint)pointsAmount[i].x, (GLuint)pointsAmount[i].y, (GLuint)pointsAmount[i].z);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
    writeFile();
}

void Converter::writeFile()
{
    sdfFile = fopen(sdfFilename.c_str(), "wb");
    sdf.bind();
    GLfloat *sdfPointer = (GLfloat*)sdf.map(GL_READ_ONLY);
    if(stopwatch != nullptr)
        stopwatch->lap();
    uint32_t realSize = 0;
    std::vector<GLfloat> data(0);
    for(uint32_t i = 0; i < SDFSize; ++i)
    {
        if(sdfPointer[i * 4 + 3] != fillerValue)
        {
            data.push_back(sdfPointer[i * 4]);
            data.push_back(sdfPointer[i * 4 + 1]);
            data.push_back(sdfPointer[i * 4 + 2]);
            data.push_back(sdfPointer[i * 4 + 3]);
        }
    }
    realSize = (uint32_t)data.size();
    FILE* fileSDF = fopen(sdfFilename.c_str(), "wb");
    fwrite(&realSize, sizeof(uint32_t), 1, fileSDF);
    uint32_t start = 0;
    int count = 0;
    while(start != realSize)
    {
        count = (realSize - start) > 65536 ? 65536 : realSize - start;
        fwrite(&data[start], sizeof(GLfloat), count, fileSDF);
        start += count;
    }
    fclose(sdfFile);
    data.clear();
    if(stopwatch != nullptr)
        stopwatch->lap();
}

glm::uvec3 Converter::computeGroups(const uint32_t& inTrianglesAmount)
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

GLfloat Converter::maxSizeFactor()
{
    //TODO: Find by tests real value for "magic" number
    uint32_t forInternalUse = 0;
    return (GLfloat)MAX_ALLOWED_SSBO_SIZE / (MAX_ALLOWED_SSBO_SIZE - (TRIANGLE_SIZE + PRISM_AABB_SIZE + forInternalUse));
}

glm::uvec3 Converter::maxResolution()
{
    glm::vec3 shellMin = loader->getAABBMin();
    glm::vec3 shellMax = loader->getAABBMax();
    glm::vec3 lengthes(shellMax.x - shellMin.x, shellMax.y - shellMin.y, shellMax.z - shellMin.z);
    GLfloat minLength = glm::min(lengthes.x, glm::min(lengthes.y, lengthes.z));
    lengthes /= minLength;
    uint16_t totalParts = glm::round(lengthes.x) * glm::round(lengthes.y) * glm::round(lengthes.z);
    if(totalParts == 0)
        logStream << "setMaxResolution must called after setShellMin and setShellMax\n";
    uint16_t partSize = (uint16_t)glm::floor(glm::pow((double)MAX_SDF_SSBO_SIZE / (1 * totalParts), 0.333333/*power 1/3*/));
    return (glm::uvec3(glm::round(lengthes.x), glm::round(lengthes.y), glm::round(lengthes.z)) *= partSize);
}
