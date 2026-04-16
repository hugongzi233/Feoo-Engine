#include "render_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <array>
#include <cassert>
#include <stdexcept>

#include "feoo_camera.hpp"

namespace feoo {
    struct SimplePushConstantData {
        glm::mat4 transform{1.0f};
        alignas(16) glm::vec3 color{1.0f, 1.0f, 1.0f};
    };

    RenderSystem::RenderSystem(
        FeooDevice &device, VkRenderPass renderPass, FeooImgui *imguiPtr)
        : feooDevice{device}, imgui{imguiPtr} {
        createTextureSetLayout();
        createPipelineLayout();
        createPipeline(renderPass);
    }

    RenderSystem::~RenderSystem() {
        vkDestroyPipelineLayout(feooDevice.device(), pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(feooDevice.device(), textureSetLayout, nullptr);
    }

    void RenderSystem::createTextureSetLayout() {
        VkDescriptorSetLayoutBinding textureBinding{};
        textureBinding.binding = 0;
        textureBinding.descriptorCount = 1;
        textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureBinding.pImmutableSamplers = nullptr;
        textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &textureBinding;

        if (vkCreateDescriptorSetLayout(feooDevice.device(), &layoutInfo, nullptr, &textureSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture descriptor set layout!");
        }
    }

    void RenderSystem::createPipelineLayout() {
        VkDescriptorSetLayout setLayouts[] = {textureSetLayout};

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = setLayouts;
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
            if (!obj.model) {
                continue;
            }

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

            obj.model->draw(commandBuffer, pipelineLayout);
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
