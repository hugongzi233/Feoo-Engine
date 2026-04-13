#pragma once

#include "feoo_device.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace feoo {
	class FeooModel {
	public:

		struct Vertex {
			glm::vec3 position;
			glm::vec3 color;

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
		};

		FeooModel(FeooDevice& device, const std::vector<Vertex>& vertices);
		~FeooModel();

		FeooModel(const FeooModel&) = delete;
		FeooModel& operator=(const FeooModel&) = delete;

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);

	private:
		void createVertexBuffer(const std::vector<Vertex>& vertices);

		FeooDevice& device;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		uint32_t vertexCount;
	};
}