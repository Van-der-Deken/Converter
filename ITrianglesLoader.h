#ifndef ITRIANGLESLOADER_H_INCLUDED
#define ITRIANGLESLOADER_H_INCLUDED

#include <vector>
#include <string>
#include "../glm/glm.hpp"

namespace conv
{
    struct ModelTriangle
    {
        glm::vec4 firstVertex{0.0f};
        glm::vec4 secondVertex{0.0f};
        glm::vec4 thirdVertex{0.0f};
        glm::vec4 faceNormal{0.0f};
    };

    class ITrianglesLoader
    {
        public:
            ITrianglesLoader() {};
            ITrianglesLoader(const std::string &path) {filepath = path;};
            virtual void load() = 0;
            virtual std::vector<ModelTriangle> getTriangles() = 0;
            virtual ~ITrianglesLoader() {};
            bool isTrianglesRead() {return trianglesRead;};
            void setFilename(const std::string &filename) {filepath = filename;};
            glm::vec3 getAABBMin() {return min;};
            glm::vec3 getAABBMax() {return max;};
        protected:
            std::string filepath = "";
            bool trianglesRead = false;
            glm::vec3 min{0.0f};
            glm::vec3 max{0.0f};
    };
}

#endif // ITRIANGLESLOADER_H_INCLUDED
