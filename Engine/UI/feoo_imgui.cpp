//
// Created by dream on 2025/11/25.
//

#include "feoo_imgui.hpp"

#include <print>

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <stdexcept>
#include <vector>

#include <glm/glm.hpp>

// New includes for file-system font lookup and logging
#include <filesystem>
#include <iostream>

namespace feoo {
    static VkDescriptorPool createImguiDescriptorPool(VkDevice device) {
        // Create a descriptor pool following the ImGui example recommended sizes
        VkDescriptorPoolSize pool_sizes[] = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = 1;
        pool_info.pPoolSizes = pool_sizes;

        VkDescriptorPool descriptorPool;
        if (vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create ImGui descriptor pool");
        }
        return descriptorPool;
    }

    FeooImgui::~FeooImgui() {
        // Ensure cleanup if user forgot
    }

    void FeooImgui::createDescriptorPool(VkDevice device) {
        imguiDescriptorPool = createImguiDescriptorPool(device);
    }

    void FeooImgui::init(FeooDevice &device, FeooSwapChain &swapChain, GLFWwindow *window) {
        // Store device for cleanup
        device_ = device.device();
        createDescriptorPool(device_);

        // Setup ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void) io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        //ImGui::StyleColorsDark();
        setupCustomFont();
        setupCustomStyle();

        ImGui_ImplGlfw_InitForVulkan(window, true);

        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = device.getInstance();
        init_info.PhysicalDevice = device.getPhysicalDevice();
        init_info.Device = device_;
        init_info.QueueFamily = device.findPhysicalQueueFamilies().graphicsFamily;
        init_info.Queue = device.graphicsQueue();
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = imguiDescriptorPool;
        init_info.DescriptorPoolSize = 0;
        init_info.MinImageCount = 2;
        init_info.ImageCount = static_cast<uint32_t>(swapChain.imageCount());
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = nullptr;

        // Fill pipeline info inside init_info
        init_info.PipelineInfoMain.RenderPass = swapChain.getRenderPass();
        init_info.PipelineInfoMain.Subpass = 0;
        init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        ImGui_ImplVulkan_Init(&init_info);

        // Upload Fonts - current backend creates fonts automatically on NewFrame but call manually to be safe
        ImGui_ImplVulkan_SetMinImageCount(static_cast<uint32_t>(swapChain.imageCount()));

        std::println("Imgui initialized");
    }

    void FeooImgui::newFrame() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void FeooImgui::setupCustomFont() {
        ImGuiIO &io = ImGui::GetIO();

        const std::string fontFileName = "AlibabaPuHuiTi-3-55-Regular.ttf";
        const std::filesystem::path relativeFontPath = "Engine/Resource/Fonts";

        std::vector<std::filesystem::path> candidates;

        candidates.push_back(std::filesystem::current_path() / relativeFontPath / fontFileName);
        candidates.push_back(std::filesystem::current_path() / "Resource/Fonts" / fontFileName);

        const std::filesystem::path windowsFallback = "C:/Windows/Fonts/msyh.ttc";

        std::filesystem::path chosen;
        for (const auto &p : candidates) {
            if (std::filesystem::exists(p)) {
                chosen = p;
                break;
            }
        }

        if (!chosen.empty()) {
            io.Fonts->AddFontFromFileTTF(chosen.string().c_str(), 18.0f, nullptr,
                                         io.Fonts->GetGlyphRangesChineseFull());
            std::cout << "Loaded ImGui font: " << chosen.string() << std::endl;
        } else if (std::filesystem::exists(windowsFallback)) {
            io.Fonts->AddFontFromFileTTF(windowsFallback.string().c_str(), 18.0f, nullptr,
                                         io.Fonts->GetGlyphRangesChineseFull());
            std::cout << "Bundled font not found; loaded fallback: " << windowsFallback.string() << std::endl;
        } else {
            std::cerr << "Failed to find any font for ImGui; UI may render with default font." << std::endl;
        }
    }

    void FeooImgui::setupCustomStyle() {
        ImGuiStyle &style = ImGui::GetStyle();

        // 圆角设置
        style.WindowRounding = 0.0f;
        style.ChildRounding = 8.0f;
        style.FrameRounding = 6.0f;
        style.PopupRounding = 8.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding = 6.0f;
        style.TabRounding = 6.0f;

        // 边框大小
        style.WindowBorderSize = 0.0f;
        style.FrameBorderSize = 0.0f;
        style.PopupBorderSize = 0.0f;

        // 内边距和间距
        style.WindowPadding = ImVec2(12.0f, 12.0f);
        style.FramePadding = ImVec2(12.0f, 6.0f);
        style.ItemSpacing = ImVec2(8.0f, 6.0f);
        style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);

        // 颜色配置 - 现代深色主题
        ImVec4 *colors = style.Colors;

        // 基础颜色
        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

        // 窗口颜色
        colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.98f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.11f, 0.11f, 0.94f);

        // 边框颜色
        colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

        // 框架颜色
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);

        // 标题栏颜色
        colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.08f, 0.08f, 0.75f);

        // 菜单栏颜色
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);

        // 滚动条颜色
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);

        // 按钮颜色 - 使用青色作为主色调
        colors[ImGuiCol_Button] = ImVec4(0.00f, 0.65f, 0.80f, 0.60f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 0.75f, 0.90f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.55f, 0.70f, 1.00f);

        // 复选框颜色
        colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.65f, 0.80f, 1.00f);

        // 滑块颜色
        colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.65f, 0.80f, 0.60f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 0.75f, 0.90f, 1.00f);

        // 标签颜色
        colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.00f, 0.65f, 0.80f, 0.80f);
        colors[ImGuiCol_TabActive] = ImVec4(0.00f, 0.65f, 0.80f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

        // 标题颜色
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);

        // 分隔线颜色
        colors[ImGuiCol_Separator] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

        // 调整大小手柄颜色
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.35f, 0.35f, 0.35f, 0.30f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.00f, 0.65f, 0.80f, 0.60f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.00f, 0.75f, 0.90f, 1.00f);

        // 表格颜色
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    }

    void FeooImgui::buildUI() {
        // 1. Debug window
        ImGui::Begin("Debug Info");
        ImGui::Text("FPS: %.1f (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
        ImGui::Text("DeltaTime: %.3f", deltaTime);
        ImGui::Text("Draw Calls: %d", drawCallCount);
        ImGui::End();

        // 2. Controls panel
        ImGui::Begin("Controls");
        ImGui::SliderFloat("Exposure", &exposure, 0.1f, 5.0f);
        ImGui::ColorEdit3("Main Color", mainColor);
        ImGui::Checkbox("VSync", &vsyncEnabled);
        ImGui::End();

        // 3. Performance window
        ImGui::Begin("Performance", &showPerformanceWindow);
        ImGui::Text("Performance graphs would go here测试");
        ImGui::End();
    }

    void FeooImgui::render(VkCommandBuffer commandBuffer) {
        ImGui::Render();
        ImDrawData *draw_data = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer, VK_NULL_HANDLE);
    }

    void FeooImgui::cleanup() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        if (imguiDescriptorPool != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device_, imguiDescriptorPool, nullptr);
            imguiDescriptorPool = VK_NULL_HANDLE;
        }
    }

    glm::vec3 FeooImgui::getMainColor() {
        return glm::vec3(mainColor[0], mainColor[1], mainColor[2]);
    }

    void FeooImgui::setDeltaTime(float deltaTime){
        this->deltaTime = deltaTime;
    }
}
