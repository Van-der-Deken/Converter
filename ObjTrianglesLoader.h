#ifndef OBJTRIANGLESLOADER_H_INCLUDED
#define OBJTRIANGLESLOADER_H_INCLUDED

#include <fstream>
#include <vector>
#include <tuple>
#include "ITrianglesLoader.h"

namespace conv
{
    class ObjTrianglesLoader : public ITrianglesLoader
    {
        public:
            ObjTrianglesLoader() : ITrianglesLoader() {};
            ObjTrianglesLoader(const std::string &path);
            void load() override;
            std::vector<ModelTriangle> getTriangles() override;
        private:
            bool isVertex(const std::string &line);
            bool isNormal(const std::string &line);
            bool isTexCoord(const std::string &line);
            bool isFace(const std::string &line);
            std::tuple<glm::uvec3, glm::uvec3, glm::uvec3> getIndices(const std::string &line);
            glm::vec3 parse(const std::string &line);

            std::ifstream objFile;
            std::vector<glm::vec4> vertices;
            std::vector<glm::vec4> normals;
            std::vector<glm::vec3> texCoord;
            std::vector<ModelTriangle> triangles;
    };
}

#endif // OBJTRIANGLESLOADER_H_INCLUDED
