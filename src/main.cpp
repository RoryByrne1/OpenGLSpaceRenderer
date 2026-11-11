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
    float yaw = -90.0f;
    float pitch = 0.0f;
};

struct Planet {
    Sphere sphere;
};

struct PlanetarySystem {
    PointLight star;
    std::vector<Planet> planets;
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window, Camera& camera, float deltaTime);
void updateCameraDirection(Camera& camera);

Material getPaperMaterial() {
    return Material(glm::vec3(0.7f), glm::vec3(0.0f), 5.0f, 0.0f, 0.0f, 0.0f);
}

int main()
{
    // setup the camera
    Camera camera{};
    camera.position = glm::vec3(0.0f, 0.0f, 0.0f);
    camera.yaw = -90.0f;
    camera.pitch = 0.0f;

    updateCameraDirection(camera);

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

    glm::vec3 ambient_lighting = glm::vec3(0.005f);


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
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // unnecessary?
        glClear(GL_COLOR_BUFFER_BIT);

        // activate the shader program
        ourShader.use();

        updateCameraDirection(camera);
        ourShader.setVec3("cameraPosition", camera.position);
        ourShader.setVec3("cameraForward", camera.forward);
        ourShader.setVec3("cameraUp", camera.up);

        ourShader.setVec3("scene.ambient", ambient_lighting);
        ourShader.setInt("scene.sphereCount", system.planets.size());
        ourShader.setInt("scene.lightCount", 1);
        for (int i = 0; i < system.planets.size(); i++)
        {
            std::string sphere_str = "scene.spheres[" + std::to_string(i) + "]";
            ourShader.setVec3(sphere_str + ".position", system.planets[i].sphere.position);
            ourShader.setFloat(sphere_str + ".radius", system.planets[i].sphere.radius);
            ourShader.setVec3(sphere_str + ".material.diffuse", system.planets[i].sphere.material.diffuse);
            ourShader.setVec3(sphere_str + ".material.specular", system.planets[i].sphere.material.specular);
            ourShader.setFloat(sphere_str + ".material.glossiness", system.planets[i].sphere.material.glossiness);
            ourShader.setFloat(sphere_str + ".material.reflection", system.planets[i].sphere.material.reflection);
            ourShader.setFloat(sphere_str + ".material.refraction", system.planets[i].sphere.material.refraction);
            ourShader.setFloat(sphere_str + ".material.ior", system.planets[i].sphere.material.ior);
        }
        ourShader.setVec3("scene.lights[0].position", system.star.position);
        ourShader.setVec3("scene.lights[0].color", system.star.color);

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

    float rotationSpeed = 30.0f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera.yaw -= rotationSpeed;

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera.yaw += rotationSpeed;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera.pitch += rotationSpeed;

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera.pitch -= rotationSpeed;

    // prevent straight up or straight down
    camera.pitch = glm::clamp(camera.pitch, -89.0f, 89.0f);

    updateCameraDirection(camera);
}

void updateCameraDirection(Camera& camera)
{
    glm::vec3 direction;

    direction.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    direction.y = sin(glm::radians(camera.pitch));
    direction.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));

    camera.forward = glm::normalize(direction);

    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

    glm::vec3 right = glm::normalize(
        glm::cross(camera.forward, worldUp)
    );

    camera.up = glm::normalize(
        glm::cross(right, camera.forward)
    );
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}