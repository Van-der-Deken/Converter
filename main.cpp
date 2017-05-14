#include <GL/glew.h>
#include <GL/glut.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include "Converter.h"
#include "ObjTrianglesLoader.h"
#include "MinGWStopwatch.h"

conv::Initializer parse(const std::string &paramFilename)
{
    std::ifstream parametersFile(paramFilename);
    std::string line;
    std::map<std::string, std::string> parameters;
    parameters["Model path"] = "";
    parameters["Size factor"] = "";
    parameters["SDF path"] = "";
    parameters["Resolution"] = "";
    parameters["Loader type"] = "";
    parameters["Filler value"] = "";
    parameters["Delta"] = "";
    parameters["Path to filler shader"] = "";
    parameters["Path to modifier shader"] = "";
    parameters["Path to kernel shader"] = "";
    char delimeter = ':';
    size_t pos = 0;
    char rubbish[2];
    std::stringstream source;
    conv::Initializer initializer;
    while(std::getline(parametersFile, line))
    {
        pos = line.find(delimeter);
        if(parameters.count(line.substr(0, pos)) == 0)
        {
            std::cout << "Unknown key:" << line.substr(0, pos) << std::endl;
            parametersFile.close();
            return initializer;
        }
        else
            parameters[line.substr(0, pos)] = line.substr(pos + 1);
    }
    parametersFile.close();
    initializer.sizeFactor = std::stof(parameters["Size factor"]);
    initializer.SDFfilename = parameters["SDF path"];
    source.str(parameters["Resolution"]);
    source >> initializer.resolution.x >> rubbish[0]
           >> initializer.resolution.y >> rubbish[1]
           >> initializer.resolution.z;
    if(parameters["Loader type"] == "Obj Loader")
        initializer.trianglesLoader.reset(new conv::ObjTrianglesLoader(parameters["Model path"]));
    else
        return initializer;
    initializer.stopwatch.reset(new conv::MinGWStopwatch);
    initializer.fillerValue = std::stof(parameters["Filler value"]);
    initializer.delta = std::stof(parameters["Delta"]);
    initializer.fillerPath = parameters["Path to filler shader"];
    initializer.modifierPath = parameters["Path to modifier shader"];
    initializer.kernelPath = parameters["Path to kernel shader"];
    return initializer;
}

int main(int argc, char **argv) {
    std::ios::sync_with_stdio(false);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_ALPHA | GLUT_DOUBLE);
    glutInitWindowSize(1, 1);
    glutCreateWindow("Converter");
    glewInit();
    glewExperimental = true;

    conv::Converter converter;
    conv::Initializer initializer = parse("converter.parm");
    if(initializer.trianglesLoader.get() == nullptr)
        return 1;
    converter.initialize(initializer);
    converter.compute();
    auto laps = initializer.stopwatch->getLaps();
    for(uint16_t i = 0; i < laps.size(); ++i)
    {
        switch(i)
        {
            case 0:
                std::cout << "Loading model: " << laps[0] << " milliseconds\n";
                break;
            case 1:
                std::cout << "Preparing for computations: " << laps[1] << " milliseconds\n";
                break;
            case 2:
                std::cout << "Computing SDF: " << laps[2] << " milliseconds\n";
                break;
            case 3:
                std::cout << "Writing SDF to file: " << laps[3] << " milliseconds\n";
                break;
            default:
                std::cout << "Unknown lap description\n";
        }
    }
    glm::uvec3 resolution = converter.getResolution();
    std::cout << "Resolution: " << resolution.x << "," << resolution.y << "," << resolution.z << std::endl;

    return 0;
}
