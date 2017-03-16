//
// Created by Y500 on 08.03.2017.
//

#ifndef CONVERTER_H
#define CONVERTER_H

#include <iosfwd>
#include <sstream>
#include <fstream>
#include <bits/unique_ptr.h>
#include "../classes/GLBuffer.h"
#include "../classes/GLTexture.h"
#include "../classes/ShaderProgram.h"
#include "../classes/Mesh.h"

struct PrismAABB
{
    GLfloat min[3];
    GLfloat totalSize;
    GLfloat pointsAmount[3];
    GLfloat minIndex;
    GLfloat step[3];
    GLfloat maxIndex;

    PrismAABB()
    {
        for(int i = 0; i < 3; ++i)
        {
            min[i] = 0;
            pointsAmount[i] = 0;
            step[i] = 0;
        }
        totalSize = 0;
        minIndex = 0;
        maxIndex = 0;
    }

    PrismAABB& operator=(const PrismAABB &lValue)
    {
        for(int i = 0; i < 3; ++i)
        {
            min[i] = lValue.min[i];
            pointsAmount[i] = lValue.pointsAmount[i];
            step[i] = lValue.step[i];
        }
        totalSize = lValue.totalSize;
        minIndex = lValue.minIndex;
        maxIndex = lValue.maxIndex;
        return *this;
    }
};

class Converter {
    public:
        Converter();
        Converter(const std::ostream &inLogStream);
        ~Converter();
        void setSizeFactor(uint16_t inSizeFactor);
        void setResolution(const glm::uvec3 &inResolution);
        void setShellMin(const glm::vec3 &inShellMin);
        void setShellMax(const glm::vec3 &inShellMax);
        void setFillerValue(GLfloat inFillerValue);
        bool loadFiller(const std::string &path);
        bool loadModifier(const std::string &path);
        bool loadKernel(const std::string &path);
        void computeDistanceField(const std::vector<Triangle> &inTriangles);
        void computeDistanceField(const std::string &fileWithTriangles);
    private:
        void openFiles(const uint16_t &index);
        void computeDistance(const uint32_t &inTriangleSize);
        void writeFiles();

        std::stringstream stringStream;

        GLBuffer triangles;
        GLBuffer prismAABBs;
        GLBuffer sdf;

        ShaderProgram filler;
        ShaderProgram modifier;
        ShaderProgram kernel;

        std::ifstream trianglesFile;
        std::ofstream sdfFile;

        bool ready;
        glm::uvec3 resolution;
        glm::vec3 shellMin;
        glm::vec3 shellMax;
        GLfloat fillerValue;
        std::ostream logStream;

        uint32_t MAX_TRIANGLES_SSBO_SIZE;
        uint32_t MAX_PRISM_AABB_SSBO_SIZE;
        uint32_t MAX_SDF_SSBO_SIZE;
        uint16_t MAX_WORK_GROUP_COUNT;
        uint16_t TRIANGLE_SIZE;
        uint16_t PRISM_AABB_SIZE;
        uint16_t DISTANCE_AND_COORD_SIZE;
        uint16_t SIZE_FACTOR;
};


#endif //CONVERTER_H
