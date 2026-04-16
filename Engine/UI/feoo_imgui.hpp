//
// Created by dream on 2025/11/25.
//
#pragma once

#include "imgui.h"
#include <glm/vec3.hpp>
#include <functional>

#include "../Core/feoo_device.hpp"
#include "../Core/feoo_swap_chain.hpp"
#include "../Window/feoo_window.hpp"

namespace feoo {
    class FeooImgui {
    public:
        FeooImgui() = default;

        ~FeooImgui();

        FeooImgui(const FeooImgui &) = delete;

        FeooImgui &operator=(const FeooImgui &) = delete;

        // Initialize ImGui for Vulkan+GLFW. Must be called after swapchain (render pass) exists.
        void init(FeooDevice &device, FeooSwapChain &swapChain, GLFWwindow *window);

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

        void setDeltaTime(float deltaTime);

        void setCustomUiDrawCallback(std::function<void()> callback);

        void setSceneViewportTexture(ImTextureID textureId);
        void setSceneViewportSize(ImVec2 size);
        void setSceneViewportInputActive(bool active);

        bool isSceneViewportFocused() const { return sceneViewportFocused; }
        ImVec2 getSceneViewportSize() const { return sceneViewportSize; }

    private:
        void createDescriptorPool(VkDevice device);

        VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;
        VkDevice device_ = VK_NULL_HANDLE;

        // Example UI state exposed inside the ImGui wrapper
        float deltaTime = 0.0f;
        int drawCallCount = 1;
        float exposure = 1.0f;
        float mainColor[3] = {0.1f, 0.1f, 0.1f};
        bool vsyncEnabled = true;
        bool showPerformanceWindow = true;
        bool sceneViewportFocused = false;
        bool sceneViewportInputActive = false;
        ImVec2 sceneViewportSize = ImVec2(900.0f, 600.0f);
        ImTextureID sceneViewportTexture = 0;
        std::function<void()> customUiDrawCallback;
    };
}
