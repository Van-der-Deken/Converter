#include <GL/glew.h>
#include <GL/glut.h>
#include <iostream>
#include "classes/Mesh.h"
#include "Converter.h"

Mesh mesh;
Mesh buddha;

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_ALPHA | GLUT_DOUBLE);
    glutInitWindowSize(700, 700);
    glutCreateWindow("Converter");

    glewInit();
    glewExperimental = true;

    std::ios::sync_with_stdio(false);
    mesh.loadFromOBJ("C:\\Users\\Y500\\Documents\\Models\\Dragon_max.obj", false);
//    buddha.loadFromOBJ("C:\\Users\\Y500\\Documents\\Models\\Buddha_max.obj", false);
    std::cout << "Triangles:" << mesh.getTrianglesAmount() << std::endl;

    Converter converter;
    converter.setResolution(glm::uvec3(200, 156, 199));
    converter.setFillerValue(1000000.0f);
    converter.setDelta(1.0f);
    converter.loadFiller("filler.glsl");
    converter.loadModifier("modifier.glsl");
    converter.loadKernel("kernel.glsl");
    v shellMin = mesh.getMinAABB(0.14f)/*buddha.getMinAABB(1.0f)*/;
    v shellMax = mesh.getMaxAABB(0.14f)/*buddha.getMaxAABB(1.0f)*/;
    converter.setShellMin(glm::vec3(shellMin.x, shellMin.y, shellMin.z));
    converter.setShellMax(glm::vec3(shellMax.x, shellMax.y, shellMax.z));
    converter.computeDistanceField(mesh.getTriangles()/*buddha.getTriangles()*/);
    std::cout << "Error:" << glGetError() << std::endl;

    return 0;
}
