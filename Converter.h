#ifndef CONVERTER_H_INCLUDED
#define CONVERTER_H_INCLUDED

#include <fstream>
#include <vector>
#include "../glm/glm.hpp"
#include "classes/ShaderProgram.h"
#include "classes/GLBuffer.h"
#include "ITrianglesLoader.h"
#include "IStopwatch.h"

namespace conv
{
    struct Initializer
    {
        GLfloat sizeFactor = 2.0f;
        std::string SDFfilename = "";
        glm::uvec3 resolution{0};
        ITrianglesLoader* triaglesLoader = nullptr;
        IStopwatch* stopwatch = nullptr;
        GLfloat fillerValue = 0.0f;
        GLfloat delta = 0.0f;
        std::string fillerPath = "";
        std::string modifierPath = "";
        std::string kernelPath = "";
    };

    class Converter
    {
        public:
            Converter();
            Converter(const std::ostream &inLogStream);
            ~Converter();
            void initialize(const Initializer &initializer);
            void compute();
        private:
            void writeFile();
            glm::uvec3 computeGroups(const uint32_t &inTrianglesAmount);
            GLfloat maxSizeFactor();
            glm::uvec3 maxResolution();

            bool unconstructed = true;
            bool uninitialized = true;

            GLBuffer triangles{GL_SHADER_STORAGE_BUFFER, std::cout};
            GLBuffer prismAABBs{GL_SHADER_STORAGE_BUFFER, std::cout};
            GLBuffer sdf{GL_SHADER_STORAGE_BUFFER, std::cout};

            ShaderProgram filler{std::cout};
            ShaderProgram modifier{std::cout};
            ShaderProgram kernel{std::cout};

            ITrianglesLoader* loader = nullptr;
            bool loaderCreated = false;
            FILE* sdfFile = nullptr;
            std::string sdfFilename = "";
            uint32_t SDFSize = 0;

            GLfloat delta = 0.0f;
            glm::uvec3 resolution{0};
            GLfloat fillerValue = 0.0f;
            std::ostream logStream{std::cout.rdbuf()};
            IStopwatch* stopwatch = nullptr;

            uint32_t MAX_ALLOWED_SSBO_SIZE = 0;
            uint32_t MAX_TRIANGLES_SSBO_SIZE = 0;
            uint32_t MAX_PRISM_AABB_SSBO_SIZE = 0;
            uint32_t MAX_SDF_SSBO_SIZE = 0;
            uint16_t MAX_WORK_GROUP_COUNT = 0;
            uint16_t TRIANGLE_SIZE = sizeof(ModelTriangle);
            uint16_t PRISM_AABB_SIZE = sizeof(glm::vec4);
            uint16_t SDF_ELEMENT_SIZE = 4 * sizeof(GLfloat);
            GLfloat SIZE_FACTOR = 2.0f;
    };
}

#endif // CONVERTER_H_INCLUDED
