//
// Created by dream on 2025/11/25.
//
#pragma once

#include "imgui.h"
#include <glm/vec3.hpp>

#include "../Core/gola_device.hpp"
#include "../Core/gola_swap_chain.hpp"
#include "../Window/gola_window.hpp"

namespace gola {
    class GolaImgui {
    public:
        GolaImgui() = default;

        ~GolaImgui();

        GolaImgui(const GolaImgui &) = delete;

        GolaImgui &operator=(const GolaImgui &) = delete;

        // Initialize ImGui for Vulkan+GLFW. Must be called after swapchain (render pass) exists.
        void init(GolaDevice &device, GolaSwapChain &swapChain, GLFWwindow *window);

        // Start a new ImGui frame
        void newFrame();

        void setupCustomFont();

        void setupCustomStyle();

        // Populate UI widgets (user-provided UI lives here)
        void buildUI();

        // Record ImGui draw commands into the currently recording command buffer
        void render(VkCommandBuffer commandBuffer);

        // Cleanup ImGui resources
        void cleanup();

        glm::vec3 getMainColor();

    private:
        void createDescriptorPool(VkDevice device);

        VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;
        VkDevice device_ = VK_NULL_HANDLE;

        // Example UI state exposed inside the ImGui wrapper
        int triangleCount = 2;
        int drawCallCount = 1;
        float exposure = 1.0f;
        float mainColor[3] = {0.1f, 0.1f, 0.1f};
        bool vsyncEnabled = true;
        bool showPerformanceWindow = true;
    };
}
