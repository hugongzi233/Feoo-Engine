#pragma once

#include "feoo_device.hpp"
#include "feoo_swap_chain.hpp"
#include "../Window/feoo_window.hpp"

// std
#include <cassert>
#include <memory>
#include <vector>

namespace feoo {
    class FeooRenderer {
    public:
        FeooRenderer(FeooWindow &window, FeooDevice &device);

        ~FeooRenderer();

        FeooRenderer(const FeooRenderer &) = delete;

        FeooRenderer &operator=(const FeooRenderer &) = delete;

        VkRenderPass getSwapChainRenderPass() const { return feooSwapChain->getRenderPass(); }
        FeooSwapChain &getSwapChain() { return *feooSwapChain; }
        float getAspectRatio() const { return feooSwapChain->extentAspectRatio(); }
        bool isFrameInProgress() const { return isFrameStarted; }

        VkCommandBuffer getCurrentCommandBuffer() const {
            assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
            return commandBuffers[currentFrameIndex];
        }

        int getFrameIndex() const {
            assert(isFrameStarted && "Cannot get frame index when frame not in progress");
            return currentFrameIndex;
        }

        VkCommandBuffer beginFrame();

        void endFrame();

        void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);

        void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

    private:
        void createCommandBuffers();

        void freeCommandBuffers();

        void recreateSwapChain();

        FeooWindow &feooWindow;
        FeooDevice &feooDevice;
        std::unique_ptr<FeooSwapChain> feooSwapChain;
        std::vector<VkCommandBuffer> commandBuffers;

        uint32_t currentImageIndex;
        int currentFrameIndex{0};
        bool isFrameStarted{false};
    };
} // namespace feoo
