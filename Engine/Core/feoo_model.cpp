#include "feoo_model.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <ofbx.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <unordered_map>

namespace {
    struct LoadedPixels {
        int width = 0;
        int height = 0;
        std::vector<unsigned char> pixels;
    };

    struct ImportedMeshData {
        std::vector<feoo::FeooModel::Vertex> vertices;
        std::vector<uint32_t> indices;
        std::filesystem::path texturePath;
        std::vector<unsigned char> embeddedTextureData;
        std::string embeddedTextureName;
    };

    std::string toString(const ofbx::DataView &view) {
        if (view.begin == nullptr || view.end == nullptr || view.end <= view.begin) {
            return {};
        }
        return std::string(reinterpret_cast<const char *>(view.begin), reinterpret_cast<const char *>(view.end));
    }

    std::vector<unsigned char> readBinaryFile(const std::filesystem::path &path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open file: " + path.string());
        }

        const std::streamsize size = file.tellg();
        if (size <= 0) {
            return {};
        }

        std::vector<unsigned char> buffer(static_cast<size_t>(size));
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char *>(buffer.data()), size);
        return buffer;
    }

    std::filesystem::path resolveTexturePath(const std::filesystem::path &modelDir, std::string textureName) {
        if (textureName.empty()) {
            return {};
        }

        std::replace(textureName.begin(), textureName.end(), '\\', '/');
        std::filesystem::path texturePath = textureName;

        if (texturePath.is_absolute() && std::filesystem::exists(texturePath)) {
            return texturePath;
        }

        const auto relativeCandidate = modelDir / texturePath;
        if (std::filesystem::exists(relativeCandidate)) {
            return relativeCandidate;
        }

        const auto fileNameCandidate = modelDir / texturePath.filename();
        if (std::filesystem::exists(fileNameCandidate)) {
            return fileNameCandidate;
        }

        return {};
    }

    LoadedPixels loadPixelsFromFile(const std::filesystem::path &texturePath) {
        if (!std::filesystem::exists(texturePath)) {
            throw std::runtime_error("texture file not found: " + texturePath.string());
        }

        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_set_flip_vertically_on_load(true);
        unsigned char *pixels = stbi_load(texturePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        stbi_set_flip_vertically_on_load(false);

        if (!pixels) {
            throw std::runtime_error("failed to load texture: " + texturePath.string());
        }

        LoadedPixels loaded{};
        loaded.width = width;
        loaded.height = height;
        loaded.pixels.assign(pixels, pixels + static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
        stbi_image_free(pixels);
        return loaded;
    }

    LoadedPixels loadPixelsFromMemory(const unsigned char *data, int size) {
        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_set_flip_vertically_on_load(true);
        unsigned char *pixels = stbi_load_from_memory(data, size, &width, &height, &channels, STBI_rgb_alpha);
        stbi_set_flip_vertically_on_load(false);

        if (!pixels) {
            throw std::runtime_error("failed to load embedded texture from memory");
        }

        LoadedPixels loaded{};
        loaded.width = width;
        loaded.height = height;
        loaded.pixels.assign(pixels, pixels + static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
        stbi_image_free(pixels);
        return loaded;
    }

    void normalizeMeshes(std::vector<ImportedMeshData> &meshes) {
        glm::vec3 minPosition{FLT_MAX, FLT_MAX, FLT_MAX};
        glm::vec3 maxPosition{-FLT_MAX, -FLT_MAX, -FLT_MAX};

        bool hasVertices = false;
        for (const auto &mesh : meshes) {
            for (const auto &vertex : mesh.vertices) {
                minPosition = glm::min(minPosition, vertex.position);
                maxPosition = glm::max(maxPosition, vertex.position);
                hasVertices = true;
            }
        }

        if (!hasVertices) {
            return;
        }

        const glm::vec3 extents = maxPosition - minPosition;
        const float maxExtent = std::max({extents.x, extents.y, extents.z});
        if (maxExtent <= 0.0f) {
            return;
        }

        const glm::vec3 center = (minPosition + maxPosition) * 0.5f;
        const float scale = 2.0f / maxExtent;

        for (auto &mesh : meshes) {
            for (auto &vertex : mesh.vertices) {
                vertex.position = (vertex.position - center) * scale;
            }
        }
    }

    std::vector<ImportedMeshData> importObjMeshes(const std::filesystem::path &modelPath) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn;
        std::string err;

        const std::string modelDir = modelPath.parent_path().string();
        const bool loaded = tinyobj::LoadObj(
            &attrib,
            &shapes,
            &materials,
            &warn,
            &err,
            modelPath.string().c_str(),
            modelDir.c_str(),
            true,
            true);

        if (!loaded) {
            throw std::runtime_error("failed to load obj: " + modelPath.string() + " - " + err);
        }

        std::filesystem::path fallbackTexturePath{};
        for (const auto &material : materials) {
            auto candidate = resolveTexturePath(modelPath.parent_path(), material.diffuse_texname);
            if (!candidate.empty()) {
                fallbackTexturePath = std::move(candidate);
                break;
            }
        }

        std::unordered_map<int, ImportedMeshData> meshesByMaterial;

        for (const auto &shape : shapes) {
            size_t indexOffset = 0;
            for (size_t face = 0; face < shape.mesh.num_face_vertices.size(); ++face) {
                const int fv = shape.mesh.num_face_vertices[face];
                if (fv < 3) {
                    indexOffset += static_cast<size_t>(fv);
                    continue;
                }

                const int materialId = face < shape.mesh.material_ids.size() ? shape.mesh.material_ids[face] : -1;
                auto &meshData = meshesByMaterial[materialId];

                std::vector<uint32_t> faceVertexIndices;
                faceVertexIndices.reserve(static_cast<size_t>(fv));

                for (int v = 0; v < fv; ++v) {
                    const tinyobj::index_t idx = shape.mesh.indices[indexOffset + static_cast<size_t>(v)];

                    feoo::FeooModel::Vertex vertex{};
                    vertex.color = {1.0f, 1.0f, 1.0f};

                    if (idx.vertex_index >= 0) {
                        const size_t positionIndex = static_cast<size_t>(idx.vertex_index) * 3;
                        vertex.position = {
                            attrib.vertices[positionIndex + 0],
                            attrib.vertices[positionIndex + 1],
                            attrib.vertices[positionIndex + 2]};
                    }

                    if (idx.texcoord_index >= 0) {
                        const size_t uvIndex = static_cast<size_t>(idx.texcoord_index) * 2;
                        vertex.uv = {
                            attrib.texcoords[uvIndex + 0],
                            attrib.texcoords[uvIndex + 1]};
                    } else {
                        vertex.uv = {0.0f, 0.0f};
                    }

                    meshData.vertices.push_back(vertex);
                    faceVertexIndices.push_back(static_cast<uint32_t>(meshData.vertices.size() - 1));
                }

                for (int tri = 1; tri < fv - 1; ++tri) {
                    meshData.indices.push_back(faceVertexIndices[0]);
                    meshData.indices.push_back(faceVertexIndices[tri]);
                    meshData.indices.push_back(faceVertexIndices[tri + 1]);
                }

                indexOffset += static_cast<size_t>(fv);
            }
        }

        std::vector<ImportedMeshData> meshes;
        meshes.reserve(meshesByMaterial.size());

        for (auto &[materialId, meshData] : meshesByMaterial) {
            if (materialId >= 0 && materialId < static_cast<int>(materials.size())) {
                const auto &material = materials[materialId];
                meshData.texturePath = resolveTexturePath(modelPath.parent_path(), material.diffuse_texname);
            }

            if (meshData.texturePath.empty() && !fallbackTexturePath.empty()) {
                meshData.texturePath = fallbackTexturePath;
            }

            meshes.push_back(std::move(meshData));
        }

        normalizeMeshes(meshes);
        return meshes;
    }

    std::vector<ImportedMeshData> importFbxMeshes(const std::filesystem::path &modelPath) {
        const std::vector<unsigned char> fileBytes = readBinaryFile(modelPath);
        if (fileBytes.empty()) {
            throw std::runtime_error("fbx file is empty: " + modelPath.string());
        }

        const auto flags = static_cast<ofbx::u16>(
            ofbx::LoadFlags::IGNORE_BLEND_SHAPES |
            ofbx::LoadFlags::IGNORE_CAMERAS |
            ofbx::LoadFlags::IGNORE_LIGHTS |
            ofbx::LoadFlags::IGNORE_SKIN |
            ofbx::LoadFlags::IGNORE_BONES |
            ofbx::LoadFlags::IGNORE_PIVOTS |
            ofbx::LoadFlags::IGNORE_ANIMATIONS |
            ofbx::LoadFlags::IGNORE_POSES |
            ofbx::LoadFlags::IGNORE_LIMBS);

        ofbx::IScene *scene = ofbx::load(fileBytes.data(), fileBytes.size(), flags);
        if (scene == nullptr) {
            throw std::runtime_error("failed to load fbx: " + modelPath.string() + " - " + ofbx::getError());
        }

        std::vector<ImportedMeshData> meshes;

        const int meshCount = scene->getMeshCount();
        for (int meshIndex = 0; meshIndex < meshCount; ++meshIndex) {
            const ofbx::Mesh *mesh = scene->getMesh(meshIndex);
            if (mesh == nullptr) {
                continue;
            }

            const ofbx::GeometryData &geometry = mesh->getGeometryData();
            if (!geometry.hasVertices()) {
                continue;
            }

            const auto positions = geometry.getPositions();
            const auto uvs = geometry.getUVs();

            const int partitionCount = geometry.getPartitionCount();
            for (int partitionIndex = 0; partitionIndex < partitionCount; ++partitionIndex) {
                const auto partition = geometry.getPartition(partitionIndex);
                if (partition.polygon_count <= 0) {
                    continue;
                }

                ImportedMeshData meshData{};
                std::vector<int> triangulatedIndices(static_cast<size_t>(partition.max_polygon_triangles) * 3);

                for (int polygonIndex = 0; polygonIndex < partition.polygon_count; ++polygonIndex) {
                    const auto polygon = partition.polygons[polygonIndex];
                    const uint32_t triCount = ofbx::triangulate(geometry, polygon, triangulatedIndices.data());

                    for (uint32_t i = 0; i < triCount; ++i) {
                        const int srcIndex = triangulatedIndices[i];
                        feoo::FeooModel::Vertex vertex{};
                        vertex.color = {1.0f, 1.0f, 1.0f};

                        const auto position = positions.get(srcIndex);
                        vertex.position = {position.x, position.y, position.z};

                        if (uvs.values != nullptr && srcIndex < uvs.count) {
                            const auto uv = uvs.get(srcIndex);
                            vertex.uv = {uv.x, uv.y};
                        } else {
                            vertex.uv = {0.0f, 0.0f};
                        }

                        meshData.vertices.push_back(vertex);
                        meshData.indices.push_back(static_cast<uint32_t>(meshData.vertices.size() - 1));
                    }
                }

                if (partitionIndex < mesh->getMaterialCount()) {
                    const ofbx::Material *material = mesh->getMaterial(partitionIndex);
                    if (material != nullptr) {
                        const ofbx::Texture *texture = material->getTexture(ofbx::Texture::DIFFUSE);
                        if (texture != nullptr) {
                            const auto embeddedData = texture->getEmbeddedData();
                            if (embeddedData.begin != nullptr && embeddedData.end != nullptr && embeddedData.end > embeddedData.begin) {
                                meshData.embeddedTextureData.assign(embeddedData.begin, embeddedData.end);
                                meshData.embeddedTextureName = toString(texture->getRelativeFileName());
                            } else {
                                std::string textureName = toString(texture->getRelativeFileName());
                                if (textureName.empty()) {
                                    textureName = toString(texture->getFileName());
                                }
                                meshData.texturePath = resolveTexturePath(modelPath.parent_path(), textureName);
                            }
                        }
                    }
                }

                if (!meshData.indices.empty()) {
                    meshes.push_back(std::move(meshData));
                }
            }
        }

        scene->destroy();
        normalizeMeshes(meshes);
        return meshes;
    }
}

namespace feoo {
    FeooModel::FeooModel(FeooDevice &device, const std::vector<Vertex> &vertices)
        : device{device} {
        Mesh mesh{};
        createVertexBuffer(mesh, vertices);

        std::vector<uint32_t> indices(vertices.size());
        for (uint32_t i = 0; i < indices.size(); ++i) {
            indices[i] = i;
        }

        createIndexBuffer(mesh, indices);
        mesh.indexCount = static_cast<uint32_t>(indices.size());
        mesh.texture = createWhiteTexture();
        meshes.push_back(std::move(mesh));
    }

    FeooModel::FeooModel(FeooDevice &device, std::vector<Mesh> meshes)
        : device{device}, meshes{std::move(meshes)} {
    }

    std::shared_ptr<FeooModel> FeooModel::createModelFromFile(
        FeooDevice &device,
        const std::filesystem::path &modelPath,
        VkDescriptorSetLayout textureSetLayout) {
        if (!std::filesystem::exists(modelPath)) {
            throw std::runtime_error("model file not found: " + modelPath.string());
        }

        std::vector<ImportedMeshData> importedMeshes;
        const std::string extension = modelPath.extension().string();
        if (extension == ".obj" || extension == ".OBJ") {
            importedMeshes = importObjMeshes(modelPath);
        } else if (extension == ".fbx" || extension == ".FBX") {
            importedMeshes = importFbxMeshes(modelPath);
        } else {
            throw std::runtime_error("unsupported model format: " + modelPath.string());
        }

        auto model = std::shared_ptr<FeooModel>(new FeooModel(device, std::vector<Mesh>{}));
        model->meshes.reserve(importedMeshes.size());

        for (auto &importedMesh : importedMeshes) {
            Mesh mesh{};
            model->createVertexBuffer(mesh, importedMesh.vertices);
            model->createIndexBuffer(mesh, importedMesh.indices);
            mesh.indexCount = static_cast<uint32_t>(importedMesh.indices.size());

            if (!importedMesh.embeddedTextureData.empty()) {
                mesh.texture = model->createTextureFromMemory(
                    importedMesh.embeddedTextureData.data(),
                    importedMesh.embeddedTextureData.size(),
                    importedMesh.embeddedTextureName.empty() ? std::string("embedded") : importedMesh.embeddedTextureName);
            } else if (!importedMesh.texturePath.empty()) {
                mesh.texture = model->createTextureFromFile(importedMesh.texturePath);
            } else {
                mesh.texture = model->createWhiteTexture();
            }

            model->meshes.push_back(std::move(mesh));
        }

        model->createDescriptorPoolAndSets(textureSetLayout);
        return model;
    }

    FeooModel::~FeooModel() {
        for (auto &mesh: meshes) {
            if (mesh.texture.sampler != VK_NULL_HANDLE) {
                vkDestroySampler(device.device(), mesh.texture.sampler, nullptr);
            }
            if (mesh.texture.imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device.device(), mesh.texture.imageView, nullptr);
            }
            if (mesh.texture.image != VK_NULL_HANDLE) {
                vkDestroyImage(device.device(), mesh.texture.image, nullptr);
            }
            if (mesh.texture.memory != VK_NULL_HANDLE) {
                vkFreeMemory(device.device(), mesh.texture.memory, nullptr);
            }
            if (mesh.vertexBuffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(device.device(), mesh.vertexBuffer, nullptr);
            }
            if (mesh.vertexBufferMemory != VK_NULL_HANDLE) {
                vkFreeMemory(device.device(), mesh.vertexBufferMemory, nullptr);
            }
            if (mesh.indexBuffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(device.device(), mesh.indexBuffer, nullptr);
            }
            if (mesh.indexBufferMemory != VK_NULL_HANDLE) {
                vkFreeMemory(device.device(), mesh.indexBufferMemory, nullptr);
            }
        }

        if (descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device.device(), descriptorPool, nullptr);
        }
    }

    void FeooModel::createVertexBuffer(Mesh &mesh, const std::vector<Vertex> &vertices) {
        assert(!vertices.empty() && "vertex count must be greater than 0");
        const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        device.createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            mesh.vertexBuffer,
            mesh.vertexBufferMemory);

        void *data = nullptr;
        vkMapMemory(device.device(), mesh.vertexBufferMemory, 0, bufferSize, 0, &data);
        std::memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(device.device(), mesh.vertexBufferMemory);
    }

    void FeooModel::createIndexBuffer(Mesh &mesh, const std::vector<uint32_t> &indices) {
        assert(!indices.empty() && "index count must be greater than 0");
        const VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        device.createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            mesh.indexBuffer,
            mesh.indexBufferMemory);

        void *data = nullptr;
        vkMapMemory(device.device(), mesh.indexBufferMemory, 0, bufferSize, 0, &data);
        std::memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(device.device(), mesh.indexBufferMemory);
    }

    FeooModel::TextureResource FeooModel::createWhiteTexture() {
        const std::vector<unsigned char> whitePixel{255, 255, 255, 255};
        return createTextureFromPixels(1, 1, whitePixel);
    }

    FeooModel::TextureResource FeooModel::createTextureFromFile(const std::filesystem::path &texturePath) {
        LoadedPixels pixels = loadPixelsFromFile(texturePath);
        return createTextureFromPixels(pixels.width, pixels.height, std::move(pixels.pixels));
    }

    FeooModel::TextureResource FeooModel::createTextureFromMemory(
        const unsigned char *data,
        size_t size,
        const std::string &sourceName) {
        if (data == nullptr || size == 0) {
            throw std::runtime_error("invalid texture memory: " + sourceName);
        }

        LoadedPixels pixels = loadPixelsFromMemory(data, static_cast<int>(size));
        return createTextureFromPixels(pixels.width, pixels.height, std::move(pixels.pixels));
    }

    FeooModel::TextureResource FeooModel::createTextureFromPixels(
        int width,
        int height,
        std::vector<unsigned char> pixels) {
        if (width <= 0 || height <= 0 || pixels.empty()) {
            throw std::runtime_error("invalid texture pixels");
        }

        TextureResource texture{};

        const VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * 4;
        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

        device.createBuffer(
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory);

        void *data = nullptr;
        vkMapMemory(device.device(), stagingBufferMemory, 0, imageSize, 0, &data);
        std::memcpy(data, pixels.data(), static_cast<size_t>(imageSize));
        vkUnmapMemory(device.device(), stagingBufferMemory);

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(width);
        imageInfo.extent.height = static_cast<uint32_t>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture.image, texture.memory);

        transitionImageLayout(texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        device.copyBufferToImage(stagingBuffer, texture.image, static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1);
        transitionImageLayout(texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
        vkFreeMemory(device.device(), stagingBufferMemory, nullptr);

        texture.imageView = createImageView(texture.image);
        texture.sampler = createSampler();
        return texture;
    }

    VkImageView FeooModel::createImageView(VkImage image) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView = VK_NULL_HANDLE;
        if (vkCreateImageView(device.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view");
        }
        return imageView;
    }

    VkSampler FeooModel::createSampler() {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        VkSampler sampler = VK_NULL_HANDLE;
        if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler");
        }
        return sampler;
    }

    void FeooModel::transitionImageLayout(
        VkImage image,
        VkFormat,
        VkImageLayout oldLayout,
        VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::runtime_error("unsupported texture layout transition");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage,
            destinationStage,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);

        device.endSingleTimeCommands(commandBuffer);
    }

    void FeooModel::createDescriptorPoolAndSets(VkDescriptorSetLayout textureSetLayout) {
        if (meshes.empty()) {
            return;
        }

        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = static_cast<uint32_t>(meshes.size());

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(meshes.size());

        if (vkCreateDescriptorPool(device.device(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create model descriptor pool");
        }

        std::vector<VkDescriptorSetLayout> layouts(meshes.size(), textureSetLayout);
        std::vector<VkDescriptorSet> descriptorSets(meshes.size());

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(device.device(), &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate model descriptor sets");
        }

        for (size_t i = 0; i < meshes.size(); ++i) {
            meshes[i].descriptorSet = descriptorSets[i];

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = meshes[i].texture.imageView;
            imageInfo.sampler = meshes[i].texture.sampler;

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = meshes[i].descriptorSet;
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(device.device(), 1, &descriptorWrite, 0, nullptr);
        }
    }

    void FeooModel::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
        for (const auto &mesh: meshes) {
            VkBuffer vertexBuffers[] = {mesh.vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout,
                0,
                1,
                &mesh.descriptorSet,
                0,
                nullptr);
            vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, 0, 0, 0);
        }
    }

    std::vector<VkVertexInputBindingDescription> FeooModel::Vertex::getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescriptions;
    }

    std::vector<VkVertexInputAttributeDescription> FeooModel::Vertex::getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, uv);
        return attributeDescriptions;
    }
}
