#include <GL/glew.h>
#include <GL/glut.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "classes/Mesh.h"
#include "Converter.h"

struct Initializer
{
    std::string modelPath = "";
    bool withTexCoords = false;
    std::string sdfPath = "";
    GLfloat delta = 0.f;
    GLfloat shellDisplacement = 0.f;
    glm::uvec3 resolution = glm::uvec3(0);
    bool filled = false;
};

Mesh mesh;
Timer modelLoading;
Initializer initializer;

void loadParameters()
{
    std::ifstream parametersFile("converter.parm");
    std::string line;
    std::map<std::string, std::string> parameters;
    parameters["Model path"] = "";
    parameters["Model with UVs"] = "";
    parameters["SDF path"] = "";
    parameters["Delta"] = "";
    parameters["Shell displacement"] = "";
    parameters["Resolution"] = "";
    char delimeter = ':';
    size_t pos = 0;
    GLfloat value;
    glm::uvec3 uvalue;
    bool bvalue;
    char rubbish[2];
    while(std::getline(parametersFile, line))
    {
        pos = line.find(delimeter);
        if(parameters.count(line.substr(0, pos)) == 0)
        {
            std::cout << "Unknown key:" << line.substr(0, pos) << std::endl;
            parametersFile.close();
            return;
        }
        else
            parameters[line.substr(0, pos)] = line.substr(pos + 1);
    }
    parametersFile.close();
    initializer.modelPath = parameters["Model path"];
    initializer.sdfPath = parameters["SDF path"];
    std::istringstream stream(parameters["Delta"]);
    stream >> value;
    initializer.delta = value;
    stream.str(parameters["Model with UVs"]);
    stream.clear();
    stream >> std::boolalpha >> bvalue;
    initializer.withTexCoords = bvalue;
    stream.str(parameters["Shell displacement"]);
    stream.clear();
    stream >> value;
    initializer.shellDisplacement = value;
    stream.str(parameters["Resolution"]);
    stream.clear();
    stream >> uvalue.x >> rubbish[0] >> uvalue.y >> rubbish[1] >> uvalue.z;
    initializer.resolution = uvalue;
    initializer.filled = true;
}

int main(int argc, char **argv) {
    std::ios::sync_with_stdio(false);
    loadParameters();
    if(!initializer.filled)
    {
        std::cout << "Initializer struct incorrect\n";
        return 1;
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_ALPHA | GLUT_DOUBLE);
    glutInitWindowSize(1, 1);
    glutCreateWindow("Converter");
    glewInit();
    glewExperimental = true;

    modelLoading.start();
    mesh.loadFromOBJ(initializer.modelPath, initializer.withTexCoords);
    modelLoading.end();
    std::cout << "Triangles:" << mesh.getTrianglesAmount() << std::endl;
    std::cout << "Time, spent on model loading:" << modelLoading.getPeriod() << " milliseconds" << std::endl;

    Converter converter;
    converter.setFillerValue(1000000.0f);
    converter.setDelta(initializer.delta);
    converter.setSdfFilename(initializer.sdfPath);
    converter.loadFiller("filler.glsl");
    converter.loadModifier("modifier.glsl");
    converter.loadKernel("kernel.glsl");
    v shellMin = mesh.getMinAABB(initializer.shellDisplacement);
    v shellMax = mesh.getMaxAABB(initializer.shellDisplacement);
    converter.setShellMin(glm::vec3(shellMin.x, shellMin.y, shellMin.z));
    converter.setShellMax(glm::vec3(shellMax.x, shellMax.y, shellMax.z));
    if(initializer.resolution.x == 0)
        converter.setMaxResolution();
    else
        converter.setResolution(initializer.resolution);
    converter.computeDistanceField(mesh.getTriangles());

    return 0;
}
