#include <sstream>
#include "ObjTrianglesLoader.h"

using namespace conv;

ObjTrianglesLoader::ObjTrianglesLoader(const std::string &path) : ITrianglesLoader(path)
{
    objFile.open(filepath);
}

void ObjTrianglesLoader::load()
{
    std::string line = "";
    ModelTriangle triangle;
    while(std::getline(objFile, line))
    {
        if(isVertex(line))
        {
            vertices.push_back(glm::vec4(parse(line), 0.0f));
            min.x = std::min(min.x, vertices.back().x);
            min.y = std::min(min.y, vertices.back().y);
            min.z = std::min(min.z, vertices.back().z);
            max.x = std::max(max.x, vertices.back().x);
            max.y = std::max(max.y, vertices.back().y);
            max.z = std::max(max.z, vertices.back().z);
        }
        else if(isNormal(line))
        {
            normals.push_back(glm::vec4(parse(line), 0.0f));
        }
        else if(isTexCoord(line))
        {
            texCoord.push_back(parse(line));
        }
        else if(isFace(line))
        {
            auto indices = getIndices(line);
            triangle.firstVertex = vertices[std::get<0>(indices).x - 1];
            triangle.secondVertex = vertices[std::get<1>(indices).x - 1];
            triangle.thirdVertex = vertices[std::get<2>(indices).x - 1];
            glm::vec4 faceNormal = normals[std::get<0>(indices).z - 1];
            faceNormal += normals[std::get<1>(indices).z - 1];
            faceNormal += normals[std::get<2>(indices).z - 1];
            faceNormal /= 3;
            triangle.faceNormal = glm::normalize(faceNormal);
            triangles.push_back(triangle);
        }
    }
    objFile.close();
}

std::vector<ModelTriangle> ObjTrianglesLoader::getTriangles()
{
    return triangles;
}

bool ObjTrianglesLoader::isVertex(const std::string &line)
{
    return line[0] == 'v' && line[1] == ' ';
}

bool ObjTrianglesLoader::isNormal(const std::string &line)
{
    return line[0] == 'v' && line[1] == 'n';
}

bool ObjTrianglesLoader::isTexCoord(const std::string &line)
{
    return line[0] == 'v' && line[1] == 't';
}

bool ObjTrianglesLoader::isFace(const std::string &line)
{
    return line[0] == 'f';
}

std::tuple<glm::uvec3, glm::uvec3, glm::uvec3> ObjTrianglesLoader::getIndices(const std::string &line)
{
    glm::uvec3 firstPoint(0);
    glm::uvec3 secondPoint(0);
    glm::uvec3 thirdPoint(0);
    std::string value = "";
    std::stringstream source(line.substr(2));

    std::getline(source, value, '/');
    firstPoint.x = value.empty() ? -1 : std::stoul(value);
    std::getline(source, value, '/');
    firstPoint.y = value.empty() ? -1 : std::stoul(value);
    std::getline(source, value, ' ');
    firstPoint.z = value.empty() ? -1 : std::stoul(value);

    std::getline(source, value, '/');
    secondPoint.x = value.empty() ? -1 : std::stoul(value);
    std::getline(source, value, '/');
    secondPoint.y = value.empty() ? -1 : std::stoul(value);
    std::getline(source, value, ' ');
    secondPoint.z = value.empty() ? -1 : std::stoul(value);

    std::getline(source, value, '/');
    thirdPoint.x = value.empty() ? -1 : std::stoul(value);
    std::getline(source, value, '/');
    thirdPoint.y = value.empty() ? -1 : std::stoul(value);
    std::getline(source, value);
    thirdPoint.z = value.empty() ? -1 : std::stoul(value);

    return std::make_tuple(firstPoint, secondPoint, thirdPoint);
}

glm::vec3 ObjTrianglesLoader::parse(const std::string &line)
{
    glm::vec3 output(0);
    std::string value = "";
    std::stringstream source(line.substr(3));
    std::getline(source, value, ' ');
    output.x = std::stof(value);
    std::getline(source, value, ' ');
    output.y = std::stof(value);
    std::getline(source, value, ' ');
    output.z = std::stof(value);
    return output;
}
