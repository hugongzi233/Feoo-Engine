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

    private:
        void createPipelineLayout();

        void createPipeline(VkRenderPass renderPass);

        FeooDevice &feooDevice;

        std::unique_ptr<FeooPipeline> feooPipeline;
        VkPipelineLayout pipelineLayout;
        FeooImgui *imgui = nullptr;
    };
}
