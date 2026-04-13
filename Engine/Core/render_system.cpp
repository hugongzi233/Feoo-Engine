#include "render_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <stdexcept>

#include "feoo_camera.hpp"

namespace feoo {
    struct SimplePushConstantData {
        glm::mat4 transform{1.0f};
        alignas(16) glm::vec3 color;
    };

    RenderSystem::RenderSystem(
        FeooDevice &device, VkRenderPass renderPass, FeooImgui *imguiPtr)
        : feooDevice{device}, imgui{imguiPtr} {
        createPipelineLayout();
        createPipeline(renderPass);
    }

    RenderSystem::~RenderSystem() {
        vkDestroyPipelineLayout(feooDevice.device(), pipelineLayout, nullptr);
    }

    void RenderSystem::createPipelineLayout() {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(feooDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

    void feoo::RenderSystem::createPipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        FeooPipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        feooPipeline = std::make_unique<FeooPipeline>(
            feooDevice,
            "Engine/shaders/simple_shader.vert.spv",
            "Engine/shaders/simple_shader.frag.spv",
            pipelineConfig);
    }

    void feoo::RenderSystem::renderGameObjects(
        VkCommandBuffer commandBuffer,
        std::vector<FeooGameObject> &gameObjects, const FeooCamera &camera) {
        feooPipeline->bind(commandBuffer);

        auto projectionView = camera.getProjection() * camera.getView();

        for (auto &obj: gameObjects) {
            SimplePushConstantData push{};
            push.color = obj.color;
            push.transform = projectionView * obj.transform.mat4();

            vkCmdPushConstants(
                commandBuffer,
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(SimplePushConstantData),
                &push);

            obj.model->bind(commandBuffer);
            obj.model->draw(commandBuffer);
        }
    }

    void RenderSystem::renderImgui(VkCommandBuffer commandBuffer) {
        if (imgui) {
            imgui->newFrame();
            imgui->buildUI();
            imgui->render(commandBuffer);
        }
    }
}
