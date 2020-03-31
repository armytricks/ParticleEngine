#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ParticleManager.h"
#include "Component.h"

// Final include
#include "ShaderLoader.h"

#define WIDTH 1280
#define HEIGHT 720

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Particle Engine", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return EXIT_FAILURE;
    }

    glViewport(0, 0, WIDTH, HEIGHT);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Create and compile our GLSL program from the shaders
    GLuint particleShader = ShaderLoader::LoadShaders("ParticleVS.vertexshader", "ParticleFS.fragmentshader");

    // Set ortho camera (2D) - If 1280x720, world space -> x in [-6.4, 6.4], y in [-3.6, 3.6]
    // EVERYTHING IS CENTERED IN OPENGL
    float scaled_width = WIDTH / 200.f;
    float scaled_height = HEIGHT / 200.f;
    glm::mat4 Projection = glm::ortho(-scaled_width, scaled_width, -scaled_height, scaled_height, 0.0f, 100.0f); // In world coordinates
    // Camera matrix
    glm::mat4 View = glm::lookAt(
        glm::vec3(0, 0, 1), // Camera just behind origin, in World Space
        glm::vec3(0, 0, 0), // and looks at the origin
        glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
    );

    // Our ModelViewProjection : multiplication of our 3 matrices
    glm::mat4 mv = Projection * View; // Remember, matrix multiplication is the other way around
    // Handle for MV uniform
    GLuint mvMatrixID = glGetUniformLocation(particleShader, "MV");

    ParticleManager particleManager = ParticleManager();
    glm::vec2 translations[ParticleManager::MAX_PARTICLES];
    glm::vec3 colours[ParticleManager::MAX_PARTICLES];

    Component* quadGrid = new Component();
    quadGrid->setVelocity(glm::vec2(0.5, 0));
    int index = 0;
    float offset = 0.1f;
    for (int y = -4; y < 4; y += 2)
    {
        for (int x = -4; x < 4; x += 2)
        {
            glm::vec2 translation;
            translation.x = (float)x / 10.0f + offset;
            translation.y = (float)y / 10.0f + offset + 0.1f;

            Particle& p = particleManager.getUnusedParticle();
            p.setLife(1.f);
            p.setPosition(translation);
            //p.setMass(0.3f);
            //p.setDensity(0.3f);
            p.setRestitution(0.8f);
            p.setColour(glm::vec3(abs(y) / 10.f, abs(x) / 10.f, 1.f));

            // Add particle to component
            quadGrid->addChild(&p);
            index++;
        }
    }

    // TEST CODE
    // Results:
    // Particles are exactly 0.1x0.1 in size in worldspace
    for (int y = -360; y <= 360; y += 15) {
        Particle& p = particleManager.getUnusedParticle();
        p.setLife(1.f);
        p.setPosition(glm::vec2(5.f, y / 100.f));
        p.setColour(glm::vec3(0, 1, 0));
        p.setDensity(0);
        p.setRestitution(1);
    }
    //for (int x = -640; x <= 640; x += 10) {
    //    Particle& p = particleManager.getUnusedParticle();
    //    p.setLife(1.f);
    //    p.setPosition(glm::vec2(x / 100.f, 3.5f));
    //    p.setColour(glm::vec3(0, 1, 0));
    //}
    //for (int x = -640; x <= 640; x += 10) {
    //    Particle& p = particleManager.getUnusedParticle();
    //    p.setLife(1.f);
    //    p.setPosition(glm::vec2(x / 100.f, 3.39f));
    //    p.setColour(glm::vec3(1, 0, 0));
    //}

    // store instance data in an array buffer
    // --------------------------------------
    unsigned int instanceTranslationsVBO;
    glGenBuffers(1, &instanceTranslationsVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceTranslationsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * ParticleManager::MAX_PARTICLES, &translations[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    unsigned int instanceColoursVBO;
    glGenBuffers(1, &instanceColoursVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceColoursVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * ParticleManager::MAX_PARTICLES, &colours[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float quadVertices[] = {
        // positions     // colors
        -0.05f,  0.05f,
         0.05f, -0.05f,
        -0.05f, -0.05f,
        -0.05f,  0.05f,
         0.05f, -0.05f,
         0.05f,  0.05f,
    };

    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    // Instance colours
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, instanceColoursVBO);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(1, 1); // tell OpenGL this is an instanced vertex attribute.

    // also set instance data
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, instanceTranslationsVBO); // this attribute comes from a different vertex buffer
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(2, 1); // tell OpenGL this is an instanced vertex attribute.

    // Time stepping
    const double fps = 144.0;
    const double dt = 1.0 / fps;
    double accumulator = 0;
    double frameStart = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        // Time stepping
        const double currentTime = glfwGetTime();
        // Store time elapsed since last frame begain
        accumulator += currentTime - frameStart;
        // Record starting of this frame
        frameStart = currentTime;
        // Avoid spiral of death and clamp dt
        if (accumulator > 0.2)
            accumulator = 0.2;


        // Handle exit event
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        // Clear
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update at fixed FPS -> deterministic physics
        while (accumulator > dt) {

            if ((int) currentTime % 5 == 0) {
                if (quadGrid != nullptr) {
                    quadGrid->releaseChildren();
                    delete quadGrid;
                    quadGrid = nullptr;
                    std::cout << "Particles released!" << std::endl;
                }
            }

            if (quadGrid != nullptr) {
                // Update components
                quadGrid->update(dt);
            }

            // Update particles
            particleManager.update(dt, translations, colours);

            accumulator -= dt;
        }

        // TODO: linear interpolation

        // Render particles
        if (particleManager.getParticlesCount() > 0) {

            // Use particle shader
            glUseProgram(particleShader);

            // Send model * view matrix to shader
            // Doesn't need to be in loop, but in case future updates involve moving the camera...
            glUniformMatrix4fv(mvMatrixID, 1, GL_FALSE, &mv[0][0]);

            // Send updated data to shader
            glBindBuffer(GL_ARRAY_BUFFER, instanceTranslationsVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * particleManager.getParticlesCount(), &translations[0], GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ARRAY_BUFFER, instanceColoursVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * particleManager.getParticlesCount(), &colours[0], GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            // Instanced rendering of all particles
            glBindVertexArray(quadVAO);
            glDrawArraysInstanced(GL_TRIANGLES, 0, 6, particleManager.getParticlesCount()); // particlesCount triangles of 6 vertices each (a quad)
            glBindVertexArray(0);
        }

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}