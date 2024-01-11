#include <stdio.h>
#include <functional>
#include <fstream>

#include <hkui/vendor/GL/Glefw.hpp>
#include <hkui/inputHelpers/InputHelper.hpp>
#include "Application.hpp"

int main(int, char**)
{
    const int32_t windowWidth = 500;
    const int32_t windowHeight = 400;

    const int32_t minWidth = 500;
    const int32_t minHeight = 400;
    const int32_t maxWidth = 1920;
    const int32_t maxHeight = 1080;

    /* Init glfw */
    if (GLFW_FALSE == glfwInit())
    {
        perror("Something happened while trying to initialize GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create mock window just to succeed initializing glew*/
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "UnSnapshot C++", NULL, NULL);
    if (window == NULL)
    {
        perror("Failed to create glew initializing window\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    // glfwSwapInterval(0); // zero to disable Vsync

    /* Init glew after glfw (we need to have a valid context bound first) */
    if (glewInit() != GLEW_OK)
    {
        perror("GLEW failed to initialize\n");
        return false;
    }

    /* Quick gpu info */
    printf("GPU Vendor: %s\nGPU Renderer: %s\n", glGetString(GL_VENDOR), glGetString(GL_RENDERER));

    glfwSetWindowSizeLimits(window, minWidth, minHeight, maxWidth, maxHeight);

    Application app(window);
    app.setup();

    inputHelpers::InputHelper& inHelper = inputHelpers::InputHelper::get();
    inHelper.observe(window);
    inHelper.registerOnKeyAction(std::bind(&Application::onKeyPress, &app,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3,
        std::placeholders::_4));

    inHelper.registerOnWindowResizeAction(std::bind(&Application::onWindowResize, &app,
        std::placeholders::_1,
        std::placeholders::_2));

    inHelper.registerOnMouseButtonAction(std::bind(&Application::onButtonAction, &app,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3));

    inHelper.registerOnMouseMoveAction(std::bind(&Application::onMouseMoveAction, &app,
        std::placeholders::_1,
        std::placeholders::_2));

    inHelper.registerOnMouseDropAction(std::bind(&Application::onMouseDrop, &app,
        std::placeholders::_1,
        std::placeholders::_2));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!glfwWindowShouldClose(window))
    {
        app.loop();
        glfwSwapBuffers(window);
        glfwWaitEvents();
    }

    /* Free no longer needed init window. User must make sure now there's a context bound
       by him before using any glew/glfw API calls
    */
    glfwDestroyWindow(window);
    glfwTerminate();
}