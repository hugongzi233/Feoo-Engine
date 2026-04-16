#pragma once

#include "feoo_device.hpp"
#include "feoo_game_object.hpp"
#include "feoo_pipeline.hpp"

// std
#include <memory>
#include <vector>

#include "../UI/feoo_imgui.hpp"

namespace feoo {
    class FeooCamera;

    class RenderSystem {
    public:
        RenderSystem(
            FeooDevice &device, VkRenderPass renderPass, FeooImgui *imguiPtr);

        ~RenderSystem();

        RenderSystem(const RenderSystem &) = delete;

        RenderSystem &operator=(const RenderSystem &) = delete;

        void renderGameObjects(
            VkCommandBuffer commandBuffer,
            std::vector<FeooGameObject> &gameObjects,
            const FeooCamera &camera);

        void renderImgui(VkCommandBuffer commandBuffer);

        VkDescriptorSetLayout getTextureSetLayout() const { return textureSetLayout; }

    private:
        void createTextureSetLayout();

        void createPipelineLayout();

        void createPipeline(VkRenderPass renderPass);

        FeooDevice &feooDevice;

        std::unique_ptr<FeooPipeline> feooPipeline;
        VkDescriptorSetLayout textureSetLayout;
        VkPipelineLayout pipelineLayout;
        FeooImgui *imgui = nullptr;
    };
}
