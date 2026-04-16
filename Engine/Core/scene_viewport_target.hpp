#pragma once

#include "feoo_device.hpp"

#include <vulkan/vulkan.h>

namespace feoo {
    class SceneViewportTarget {
    public:
        SceneViewportTarget(FeooDevice &device, VkFormat colorFormat, VkFormat depthFormat, VkExtent2D extent);
        ~SceneViewportTarget();

        SceneViewportTarget(const SceneViewportTarget &) = delete;
        SceneViewportTarget &operator=(const SceneViewportTarget &) = delete;

        void resize(VkExtent2D extent);

        VkRenderPass getRenderPass() const { return renderPass; }
        VkFramebuffer getFramebuffer() const { return framebuffer; }
        VkExtent2D getExtent() const { return extent; }
        VkImageView getColorImageView() const { return colorImageView; }
        VkSampler getSampler() const { return sampler; }
        VkDescriptorSet getDescriptorSet() const { return descriptorSet; }

        void beginRenderPass(VkCommandBuffer commandBuffer);
        void endRenderPass(VkCommandBuffer commandBuffer);

    private:
        void createRenderPass();
        void createResources(VkExtent2D newExtent);
        void destroyResources();
        void createDescriptorResources();
        void updateDescriptorSet();
        void createColorResources(VkExtent2D newExtent);
        void createDepthResources(VkExtent2D newExtent);
        void createFramebuffer();
        VkSampler createSampler();

        FeooDevice &device;
        VkFormat colorFormat;
        VkFormat depthFormat;
        VkExtent2D extent{};

        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;

        VkImage colorImage = VK_NULL_HANDLE;
        VkDeviceMemory colorImageMemory = VK_NULL_HANDLE;
        VkImageView colorImageView = VK_NULL_HANDLE;

        VkImage depthImage = VK_NULL_HANDLE;
        VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
        VkImageView depthImageView = VK_NULL_HANDLE;

        VkSampler sampler = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    };
}
