#include "feoo_window.hpp"
#include <stdexcept>

namespace feoo {
    FeooWindow::FeooWindow(int width, int height, const std::string &title) : width(width), height(height),
                                                                              windowTitle(title) {
        initWindow();
    }

    FeooWindow::~FeooWindow() {
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }

    void FeooWindow::initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(width, height, windowTitle.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        if (!window) {
            throw std::runtime_error("Failed to create GLFW window");
        }
    }

    void FeooWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) {
        if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface");
        }
    }

    void FeooWindow::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
        auto app = static_cast<FeooWindow *>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
        app->width = width;
        app->height = height;
    }
}
