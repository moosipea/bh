#define GLFW_INCLUDE_NONE

#include <stdio.h>
#include <GLFW/glfw3.h>

int main(void) {
    if (!glfwInit()) {
        fprintf(stderr, "GLFW initialization failed\n");
        return 1;
    }

    GLFWwindow *window = glfwCreateWindow(640, 480, "Hello, world!", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Window creation failed\n");
    }

    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
