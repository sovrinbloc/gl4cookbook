//
// Created by aicdg on 2017/3/30.
//

#ifndef GL4COOKBOOK_SCENEWOOD_H
#define GL4COOKBOOK_SCENEWOOD_H


#include "scene.h"
#include "glslprogram.h"

#include <glad/glad.h>

#include <glm/glm.hpp>
using glm::mat4;
using glm::vec4;
using glm::vec3;

class SceneWood : public Scene
{
private:
    GLSLProgram prog;

    GLuint quad;
    int width, height;

    mat4 model, view, projection;
    vec3 lightPos;
    float angle;

    void setMatrices();
    void compileAndLinkShader();
    void drawScene();

public:
    SceneWood();

    void initScene();
    void update( float t );
    void render();
    void resize(int, int);
};


#endif //GL4COOKBOOK_SCENEWOOD_H
