#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <string>

#include "shader.h"
#include "camera.h"
#include "terrain.h"
#include "skybox.h"

// ---- window / viewport -------------------------------------------------- //
const unsigned int SCR_W = 1280;
const unsigned int SCR_H = 720;

// ---- globals ------------------------------------------------------------ //
Camera camera(glm::vec3(256.0f, 100.0f, 256.0f));

float lastX = SCR_W / 2.0f;
float lastY = SCR_H / 2.0f;
bool  firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool wireframe    = false;
bool fKeyWasDown  = false;   // edge-detect for wireframe toggle

// ---- callbacks ---------------------------------------------------------- //
void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

void mouse_callback(GLFWwindow*, double xd, double yd) {
    float xpos = (float)xd, ypos = (float)yd;
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    camera.ProcessMouseMovement(xpos - lastX, lastY - ypos);
    lastX = xpos; lastY = ypos;
}

void scroll_callback(GLFWwindow*, double, double yoff) {
    camera.ProcessMouseScroll((float)yoff);
}

// ---- input -------------------------------------------------------------- //
void processInput(GLFWwindow* win) {
    if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(win, true);

    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD,  deltaTime);
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT,     deltaTime);
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT,    deltaTime);
    if (glfwGetKey(win, GLFW_KEY_SPACE)      == GLFW_PRESS) camera.ProcessKeyboard(UP,   deltaTime);
    if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camera.ProcessKeyboard(DOWN, deltaTime);

    // F — toggle wireframe (edge-triggered)
    bool fDown = glfwGetKey(win, GLFW_KEY_F) == GLFW_PRESS;
    if (fDown && !fKeyWasDown) {
        wireframe = !wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    }
    fKeyWasDown = fDown;
}

// ---- main --------------------------------------------------------------- //
int main() {
    // ---------- Init GLFW ----------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4); // 4× MSAA

    GLFWwindow* win = glfwCreateWindow(SCR_W, SCR_H, "Procedural Terrain — WASD to fly | F for wireframe", nullptr, nullptr);
    if (!win) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    glfwSetCursorPosCallback(win, mouse_callback);
    glfwSetScrollCallback(win, scroll_callback);
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // ---------- Load OpenGL ----------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);

    // ---------- Shaders ----------
    Shader terrainShader("shaders/terrain.vert", "shaders/terrain.frag");
    Shader skyboxShader ("shaders/skybox.vert",  "shaders/skybox.frag");

    // ---------- Scene objects ----------
    Terrain terrain(8, 32, 4.0f, 80.0f);
    terrain.generateChunks();

    Skybox skybox;

    // ---------- Lights / global uniforms ----------
    // Sun direction (pointing FROM the sun, i.e. where light travels)
    glm::vec3 lightDir = glm::normalize(glm::vec3(-0.6f, -0.9f, 0.4f));
    glm::vec3 fogColor = glm::vec3(0.62f, 0.78f, 0.95f);

    terrainShader.use();
    terrainShader.setVec3 ("lightDir",   lightDir);
    terrainShader.setVec3 ("fogColor",   fogColor);
    terrainShader.setFloat("fogDensity", 0.0025f);   // tweak for more/less fog

    // ---------- Frame stats ----------
    double fpsTimer = 0.0;
    int    fpsFrames = 0;

    // ---------- Render loop ----------
    while (!glfwWindowShouldClose(win)) {
        float now   = (float)glfwGetTime();
        deltaTime   = now - lastFrame;
        lastFrame   = now;

        fpsTimer  += deltaTime;
        fpsFrames++;
        if (fpsTimer >= 1.0) {
            std::string title = "Procedural Terrain — "
                              + std::to_string(fpsFrames) + " FPS | "
                              "WASD fly | Space/Shift up/down | F wireframe | ESC quit";
            glfwSetWindowTitle(win, title.c_str());
            fpsTimer = 0; fpsFrames = 0;
        }

        processInput(win);

        // Dynamic LOD update (cheap: only rebuilds chunks that changed LOD level)
        terrain.update(camera.Position);

        // Clear
        glClearColor(fogColor.r, fogColor.g, fogColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Build matrices
        glm::mat4 proj  = glm::perspective(glm::radians(camera.Zoom),
                                            (float)SCR_W / (float)SCR_H,
                                            0.5f, 3000.0f);
        glm::mat4 view  = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        // ----- Terrain -----
        terrainShader.use();
        terrainShader.setMat4("projection", proj);
        terrainShader.setMat4("view",       view);
        terrainShader.setMat4("model",      model);
        terrainShader.setVec3("viewPos",    camera.Position);
        terrain.render(terrainShader);

        // ----- Skybox -----
        // Strip translation from view matrix so sky is always at infinity
        glm::mat4 skyView = glm::mat4(glm::mat3(view));
        skyboxShader.use();
        skyboxShader.setMat4("projection", proj);
        skyboxShader.setMat4("view",       skyView);
        skybox.render(skyboxShader);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
