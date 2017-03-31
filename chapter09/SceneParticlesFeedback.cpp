//
// Created by yangyanjun on 2017/3/31.
//

#include "SceneParticlesFeedback.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
using std::endl;
using std::cerr;

#include "bmpreader.h"

#include "glutils.h"

using glm::vec3;

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

SceneParticlesFeedback::SceneParticlesFeedback() :
        width(800), height(600), drawBuf(1), time(0), deltaT(0) {}

void SceneParticlesFeedback::initScene()
{

    compileAndLinkShader();

    GLuint programHandle = prog.getHandle();
    renderSub = glGetSubroutineIndex(programHandle, GL_VERTEX_SHADER, "render");
    updateSub = glGetSubroutineIndex(programHandle, GL_VERTEX_SHADER, "update");

    glClearColor(0.1f,0.1f,0.1f,0.0f);

    glPointSize(10.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    projection = mat4(1.0f);
    model = mat4(1.0f);

    initBuffers();

    const char * texName = "media/texture/bluewater.bmp";
    glActiveTexture(GL_TEXTURE0);
    BMPReader::loadTex(texName);

    prog.setUniform("ParticleTex", 0);
    prog.setUniform("ParticleLifetime", 3.5f);
    prog.setUniform("Accel", vec3(0.0f,-0.4f,0.0f));
}

void SceneParticlesFeedback::initBuffers()
{
    nParticles = 4000;

    // Generate the buffers
    glGenBuffers(2, posBuf);    // position buffers
    glGenBuffers(2, velBuf);    // velocity buffers
    glGenBuffers(2, startTime); // Start time buffers
    glGenBuffers(1, &initVel);  // Initial velocity buffer (never changes, only need one)

    // Allocate space for all buffers
    int size = nParticles * 3 * sizeof(GLfloat);
    glBindBuffer(GL_ARRAY_BUFFER, posBuf[0]);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_COPY);
    glBindBuffer(GL_ARRAY_BUFFER, posBuf[1]);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_COPY);
    glBindBuffer(GL_ARRAY_BUFFER, velBuf[0]);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_COPY);
    glBindBuffer(GL_ARRAY_BUFFER, velBuf[1]);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_COPY);
    glBindBuffer(GL_ARRAY_BUFFER, initVel);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, startTime[0]);
    glBufferData(GL_ARRAY_BUFFER, nParticles * sizeof(float), NULL, GL_DYNAMIC_COPY);
    glBindBuffer(GL_ARRAY_BUFFER, startTime[1]);
    glBufferData(GL_ARRAY_BUFFER, nParticles * sizeof(float), NULL, GL_DYNAMIC_COPY);

    // Fill the first position buffer with zeroes
    GLfloat *data = new GLfloat[nParticles * 3];
    for( int i = 0; i < nParticles * 3; i++ ) data[i] = 0.0f;
    glBindBuffer(GL_ARRAY_BUFFER, posBuf[0]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);

    // Fill the first velocity buffer with random velocities
    vec3 v(0.0f);
    float velocity, theta, phi;
    for( int i = 0; i < nParticles; i++ ) {

        theta = glm::mix(0.0f, glm::pi<float>() / 6.0f, randFloat());
        phi = glm::mix(0.0f, glm::two_pi<float>(), randFloat());

        v.x = sinf(theta) * cosf(phi);
        v.y = cosf(theta);
        v.z = sinf(theta) * sinf(phi);

        velocity = glm::mix(1.25f,1.5f,randFloat());
        v = glm::normalize(v) * velocity;

        data[3*i]   = v.x;
        data[3*i+1] = v.y;
        data[3*i+2] = v.z;
    }
    glBindBuffer(GL_ARRAY_BUFFER,velBuf[0]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
    glBindBuffer(GL_ARRAY_BUFFER,initVel);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);

    // Fill the first start time buffer
    delete [] data;
    data = new GLfloat[nParticles];
    float time = 0.0f;
    float rate = 0.001f;
    for( int i = 0; i < nParticles; i++ ) {
        data[i] = time;
        time += rate;
    }
    glBindBuffer(GL_ARRAY_BUFFER,startTime[0]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, nParticles * sizeof(float), data);

    glBindBuffer(GL_ARRAY_BUFFER,0);
    delete [] data;

    // Create vertex arrays for each set of buffers
    glGenVertexArrays(2, particleArray);

    // Set up particle array 0
    glBindVertexArray(particleArray[0]);
    glBindBuffer(GL_ARRAY_BUFFER, posBuf[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, velBuf[0]);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, startTime[0]);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, initVel);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(3);

    // Set up particle array 1
    glBindVertexArray(particleArray[1]);
    glBindBuffer(GL_ARRAY_BUFFER, posBuf[1]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, velBuf[1]);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, startTime[1]);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, initVel);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    // Setup the feedback objects
    glGenTransformFeedbacks(2, feedback);

    // Transform feedback 0
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback[0]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, posBuf[0]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, velBuf[0]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, startTime[0]);

    // Transform feedback 1
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback[1]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, posBuf[1]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, velBuf[1]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, startTime[1]);

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

    GLint value;
    glGetIntegerv(GL_MAX_TRANSFORM_FEEDBACK_BUFFERS, &value);
    printf("MAX_TRANSFORM_FEEDBACK_BUFFERS = %d\n", value);
}

float SceneParticlesFeedback::randFloat() {
    return ((float)rand() / RAND_MAX);
}

void SceneParticlesFeedback::update( float t )
{
    deltaT = t - time;
    time = t;
}

void SceneParticlesFeedback::render()
{
    // Update pass
    glUniformSubroutinesuiv(GL_VERTEX_SHADER, 1, &updateSub);

    prog.setUniform("Time", time);
    prog.setUniform("H", deltaT);

    glEnable(GL_RASTERIZER_DISCARD);

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback[drawBuf]);

    glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query);

    glBeginTransformFeedback(GL_POINTS);
    glBindVertexArray(particleArray[1-drawBuf]);
    glDrawArrays(GL_POINTS, 0, nParticles);
    glEndTransformFeedback();

    glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

    GLuint ptsWritten;
    glGetQueryObjectuiv(query, GL_QUERY_RESULT, &ptsWritten);
    //printf("Written: %d\n", ptsWritten);
    glDisable(GL_RASTERIZER_DISCARD);

    // Render pass
    glUniformSubroutinesuiv(GL_VERTEX_SHADER, 1, &renderSub);
    glClear( GL_COLOR_BUFFER_BIT );
    view = glm::lookAt(vec3(3.0f * cos(angle),1.5f,3.0f * sin(angle)), vec3(0.0f,1.5f,0.0f), vec3(0.0f,1.0f,0.0f));
    setMatrices();

//    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
//    glBindVertexArray(particleArray[drawBuf]);
//    glDrawArrays(GL_POINTS, 0, nParticles);

    glBindVertexArray(particleArray[drawBuf]);
    glDrawTransformFeedback(GL_POINTS, feedback[drawBuf]);

    // Swap buffers
    drawBuf = 1 - drawBuf;
}

void SceneParticlesFeedback::setMatrices()
{
    mat4 mv = view * model;
    prog.setUniform("MVP", projection * mv);
}

void SceneParticlesFeedback::resize(int w, int h)
{
    glViewport(0,0,w,h);
    width = w;
    height = h;
    projection = glm::perspective(glm::radians(60.0f), (float)w/h, 0.3f, 100.0f);
}

void SceneParticlesFeedback::compileAndLinkShader()
{
    try {
        prog.compileShader("shader/transfeedback.vert");
        prog.compileShader("shader/transfeedback.frag");

        //////////////////////////////////////////////////////
        // Setup the transform feedback (must be done before linking the program)
        GLuint progHandle = prog.getHandle();
        const char * outputNames[] = { "Position", "Velocity", "StartTime" };
        glTransformFeedbackVaryings(progHandle, 3, outputNames, GL_SEPARATE_ATTRIBS);
        ///////////////////////////////////////////////////////

        prog.link();
        prog.use();
    } catch(GLSLProgramException &e ) {
        cerr << e.what() << endl;
        exit( EXIT_FAILURE );
    }
}

