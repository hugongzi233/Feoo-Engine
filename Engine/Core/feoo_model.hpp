#pragma once

#include "feoo_device.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace feoo {
	class FeooModel {
	public:
		struct TextureResource {
			VkImage image = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
			VkImageView imageView = VK_NULL_HANDLE;
			VkSampler sampler = VK_NULL_HANDLE;
		};

		struct Mesh {
			VkBuffer vertexBuffer = VK_NULL_HANDLE;
			VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
			VkBuffer indexBuffer = VK_NULL_HANDLE;
			VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
			uint32_t indexCount = 0;
			TextureResource texture;
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		};

		struct Vertex {
			glm::vec3 position;
			glm::vec3 color;
			glm::vec2 uv;

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
		};

		FeooModel(FeooDevice& device, const std::vector<Vertex>& vertices);
		static std::shared_ptr<FeooModel> createModelFromFile(
			FeooDevice& device,
			const std::filesystem::path& modelPath,
			VkDescriptorSetLayout textureSetLayout);
		~FeooModel();

		FeooModel(const FeooModel&) = delete;
		FeooModel& operator=(const FeooModel&) = delete;

		void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

	private:
		FeooModel(FeooDevice& device, std::vector<Mesh> meshes);

		void createVertexBuffer(Mesh& mesh, const std::vector<Vertex>& vertices);
		void createIndexBuffer(Mesh& mesh, const std::vector<uint32_t>& indices);
		TextureResource createWhiteTexture();
		TextureResource createTextureFromFile(const std::filesystem::path& texturePath);
		TextureResource createTextureFromMemory(const unsigned char* data, size_t size, const std::string& sourceName);
		TextureResource createTextureFromPixels(int width, int height, std::vector<unsigned char> pixels);
		VkImageView createImageView(VkImage image);
		VkSampler createSampler();
		void transitionImageLayout(
			VkImage image,
			VkFormat format,
			VkImageLayout oldLayout,
			VkImageLayout newLayout);
		void createDescriptorPoolAndSets(VkDescriptorSetLayout textureSetLayout);

		FeooDevice& device;
		std::vector<Mesh> meshes;
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	};
}