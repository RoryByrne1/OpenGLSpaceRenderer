#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm/glm.hpp>

#include <iostream>
#include <vector>

#include "Shader.h"
#include "Geometry.h"

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

struct Camera {
    glm::vec3 position;
    glm::vec3 forward;
    glm::vec3 up;
};

struct Planet {
    Sphere sphere;
};

struct PlanetarySystem {
    PointLight star;
    std::vector<Planet> planets;
};

struct Scene {
    glm::vec3 ambient;
    std::vector<PointLight> lights;
    std::vector<Sphere> spheres;
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window, Camera& camera, float deltaTime);

Material getPaperMaterial() {
    return Material(glm::vec3(0.7, 0.7, 0.7), glm::vec3(0, 0, 0), 5.0, 0.0, 0.0, 0.0);
}

int main()
{
    // setup the camera
    Camera camera{};
    camera.position = glm::vec3(0.0f, 0.0f, 0.0f);
    camera.forward = glm::vec3(0.0f, 0.0f, -1.0f);
    camera.up = glm::vec3(0.0f, 1.0f, 0.0f);

    // setup the planetary system
    PlanetarySystem system{};

    PointLight star{};
    star.color = glm::vec3(1.0f, 0.95f, 0.85f);
    star.position = glm::vec3(0.0f, 0.0f, 0.0f);

    Planet planet{};
    planet.sphere.position = glm::vec3(-1.0f, 0.0f, -5.0f);
    planet.sphere.material = getPaperMaterial();
    planet.sphere.radius = 1.0f;

    system.star = star;
    system.planets.push_back(planet);


    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialise GLAD" << std::endl;
        return -1;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    Shader ourShader("shaders/shader.vert", "shaders/shader.frag");

    // set up vertex data (and buffers) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,

        -1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // bind the Vertex Array Object first, then bind and set vertex buffers, and then configure vertex attributess
    glBindVertexArray(VAO);

    // copy our vertices array in a vertex buffer for OpenGL to use
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // then set the vertex attributes pointers
    // position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // you can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens; modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary
    glBindVertexArray(0);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window, camera, deltaTime);

        // render
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // activate the shader program
        ourShader.use();

        ourShader.setVec3("cameraPosition", camera.position);
        ourShader.setVec3("cameraForward", camera.forward);
        ourShader.setVec3("cameraUp", camera.up);

        glfwGetFramebufferSize(window, &width, &height);
        glUniform2i(glGetUniformLocation(ourShader.ID, "viewport"), width, height);

        // render the triangles
        glBindVertexArray(VAO); // seeing as we only have a single VAO its not necessary to bind it every time
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // glBindVertexArray(0); // no need to unbind it every time

        // glfw: swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // de-allocate all resources once they've outlived their purpose
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();
    return 0;
}

// process all input, query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window, Camera& camera, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float velocity = 1.0f * deltaTime;

    glm::vec3 right = glm::normalize(glm::cross(camera.forward, camera.up));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.position += camera.forward * velocity;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.position -= camera.forward * velocity;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.position -= right * velocity;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.position += right * velocity;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.position += camera.up * velocity;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.position -= camera.up * velocity;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}