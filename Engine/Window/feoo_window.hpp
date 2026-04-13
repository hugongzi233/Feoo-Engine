#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

namespace feoo {
    class FeooWindow {
    public:
        FeooWindow(int width, int height, const std::string &title);

        ~FeooWindow();

        void resetWindowResizedFlag() { framebufferResized = false; }

        FeooWindow(const FeooWindow &) = delete;

        FeooWindow &operator=(const FeooWindow &) = delete;

        bool shouldClose() const {
            return glfwWindowShouldClose(window);
        }

        bool wasWindowResized() { return framebufferResized; }
        VkExtent2D getExtent() { return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }

        bool wasFramebufferResized() {
            return framebufferResized;
        }

        void resetFramebufferResizedFlag() {
            framebufferResized = false;
        }

        void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

        GLFWwindow *getGLFWwindow() const { return window; }

    private:
        static void framebufferResizeCallback(GLFWwindow *window, int width, int height);

        void initWindow();

        int width;
        int height;
        bool framebufferResized = false;

        std::string windowTitle;
        GLFWwindow *window;
    };
}
