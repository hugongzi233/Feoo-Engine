#include "feoo_app.hpp"
#include "Core/feoo_game_object.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <chrono>
#include <iostream>
#include <thread>

#include "Core/render_system.hpp"
#include "Core/feoo_camera.hpp"
#include "Core/keyboard_movement_controller.hpp"

namespace feoo {
    FeooApp::FeooApp() {
        
    }

    FeooApp::~FeooApp() {
    }

    void FeooApp::run() {
        loadGameObjects();
        initImgui();
        RenderSystem renderSystem(device, renderer.getSwapChainRenderPass(), imgui.get());

        FeooCamera camera{};
        auto viewObject = FeooGameObject::createGameObject();
        KeyboardMovementController cameraController{};

        auto currentTime = std::chrono::high_resolution_clock::now();

        // 主循环逻辑
        while (!window.shouldClose()) {
            glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime =
                    std::chrono::duration<float>(newTime - currentTime).count();
            currentTime = newTime;
            frameTime = glm::min(frameTime, 60.f / 1000.f);
            update(frameTime);
            
            cameraController.moveInPlaneXZ(window.getGLFWwindow(), frameTime, viewObject);
            camera.setViewYXZ(viewObject.transform.translation,viewObject.transform.rotation);

            float aspect = renderer.getAspectRatio();
            // camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
            camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10000.f);

            if (auto commandBuffer = renderer.beginFrame()) {
                renderer.beginSwapChainRenderPass(commandBuffer);
                renderSystem.renderGameObjects(commandBuffer, gameobjects, camera);
                renderSystem.renderImgui(commandBuffer);

                renderer.endSwapChainRenderPass(commandBuffer);
                renderer.endFrame();
            }
        }

        vkDeviceWaitIdle(device.device());
    }

    void FeooApp::update(float deltaTime) {
        float offset = 1.5f;
        for (auto &obj: gameobjects) {
            obj.transform.rotation.z += deltaTime * offset;
            offset+=1.0f;
            obj.transform.rotation.x += deltaTime * (offset-1.0f);
        }
    }

    std::unique_ptr<FeooModel> createCubeModel(FeooDevice &device, glm::vec3 offset) {
        std::vector<FeooModel::Vertex> vertices = {
            // left face (white)
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
            {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
            {{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
            {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
            {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
            // right face (yellow)
            {{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
            {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},
            {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
            {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},
            {{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
            {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},
            // top face (blue)
            {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
            {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
            {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
            {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
            // bottom face (green)
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
            {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
            {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            // front face (red)
            {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
            {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
            {{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
            // back face (cyan)
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
            {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
            {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
            {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
            {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
        };
        for (auto &v: vertices) {
            v.position += offset;
        }

        return std::make_unique<FeooModel>(device, vertices);
    }

    void FeooApp::loadGameObjects() {
        std::shared_ptr<FeooModel> cubeModel = createCubeModel(device, glm::vec3(0.0f, 0.0f, 0.0f));

        for (int i = 0; i < 100; i++) {
            auto gameObject = FeooGameObject::createGameObject();
            gameObject.model = cubeModel;
            gameObject.transform.translation = glm::vec3(static_cast<float>(i), 0.0f, 2.5f);
            gameObject.transform.scale = glm::vec3(0.5f, 0.5f, 0.5f);
            gameobjects.push_back(std::move(gameObject));
        }
    }

    void FeooApp::initImgui() {
        imgui = std::make_unique<FeooImgui>();
        imgui->init(device, renderer.getSwapChain(), window.getGLFWwindow());
    }
}
