#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <ctime>
#include <iostream>

struct Color {
    float r;
    float g;
    float b;
};

Color clearColor = {1.0f, 1.0f, 1.0f}; // initial background: white
bool timerEnabled = false;
double lastTimerTime = 0.0;
const double TIMER_INTERVAL = 1.0;

float RandomFloat()
{
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

void SetRandomColor()
{
    clearColor = {RandomFloat(), RandomFloat(), RandomFloat()};
}

void InputProcess(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;

    if (action != GLFW_PRESS) {
        return;
    }

    switch (key) {
    case GLFW_KEY_C:
        clearColor = {0.0f, 1.0f, 1.0f}; // cyan: green + blue
        timerEnabled = false;
        break;
    case GLFW_KEY_M:
        clearColor = {1.0f, 0.0f, 1.0f}; // magenta: red + blue
        timerEnabled = false;
        break;
    case GLFW_KEY_Y:
        clearColor = {1.0f, 1.0f, 0.0f}; // yellow: red + green
        timerEnabled = false;
        break;
    case GLFW_KEY_A:
        SetRandomColor();
        timerEnabled = false;
        break;
    case GLFW_KEY_G:
        clearColor = {0.5f, 0.5f, 0.5f}; // gray
        timerEnabled = false;
        break;
    case GLFW_KEY_K:
        clearColor = {0.0f, 0.0f, 0.0f}; // black
        timerEnabled = false;
        break;
    case GLFW_KEY_T:
        timerEnabled = true;
        lastTimerTime = glfwGetTime();
        SetRandomColor();
        break;
    case GLFW_KEY_S:
        timerEnabled = false;
        break;
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    default:
        break;
    }
}

void UpdateTimerColor()
{
    if (!timerEnabled) {
        return;
    }

    const double currentTime = glfwGetTime();
    if (currentTime - lastTimerTime >= TIMER_INTERVAL) {
        SetRandomColor();
        lastTimerTime = currentTime;
    }
}

void DrawScene()
{
    glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

int main()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    if (!glfwInit()) {
        std::cerr << "GLFW 초기화 실패!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Color Practice", nullptr, nullptr);
    if (!window) {
        std::cerr << "윈도우 생성 실패!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, InputProcess);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW 초기화 실패!" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, 800, 600);

    while (!glfwWindowShouldClose(window)) {
        UpdateTimerColor();
        DrawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
