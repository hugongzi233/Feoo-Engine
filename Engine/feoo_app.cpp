#include "feoo_app.hpp"
#include "Scripting/camera_script.hpp"
#include "Scripting/movement_script.hpp"
#include "Scripting/spin_models_script.hpp"

#include <imgui_impl_vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <chrono>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>

namespace feoo {
    FeooApp::FeooApp() = default;

    FeooApp::~FeooApp() {
        ScriptContext shutdownContext{
            device,
            window,
            renderer,
            camera,
            gameobjects,
            false,
            VK_NULL_HANDLE};
        sceneManager.unloadScene(shutdownContext);

        if (imgui) {
            vkDeviceWaitIdle(device.device());
            imgui->cleanup();
            imgui.reset();
        }
    }

    void FeooApp::run() {
        initImgui();
        RenderSystem renderSystem(device, renderer.getSwapChainRenderPass(), imgui.get());
        initSceneViewport();
        initScene(renderSystem);

        imgui->setCustomUiDrawCallback([this, &renderSystem]() {
            drawSceneManagerUi(renderSystem);
        });

        auto currentTime = std::chrono::high_resolution_clock::now();

        while (!window.shouldClose()) {
            glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime =
                    std::chrono::duration<float>(newTime - currentTime).count();
            currentTime = newTime;
            frameTime = glm::min(frameTime, 60.f / 1000.f);
            updateScene(frameTime, renderSystem);

            if (auto commandBuffer = renderer.beginFrame()) {
                imgui->newFrame();
                imgui->buildUI();

                renderSceneViewport(commandBuffer);
                renderer.beginSwapChainRenderPass(commandBuffer);
                imgui->render(commandBuffer);

                renderer.endSwapChainRenderPass(commandBuffer);
                renderer.endFrame();
            }
        }

        vkDeviceWaitIdle(device.device());
    }

    ScriptContext FeooApp::buildScriptContext(const RenderSystem &renderSystem) {
        return ScriptContext{
            device,
            window,
            renderer,
            camera,
            gameobjects,
            imgui ? imgui->isSceneViewportFocused() : false,
            renderSystem.getTextureSetLayout()};
    }

    void FeooApp::updateScene(float deltaTime, const RenderSystem &renderSystem) {
        imgui.get()->setDeltaTime(deltaTime);

        auto context = buildScriptContext(renderSystem);
        sceneManager.update(deltaTime, context);
    }

    void FeooApp::initSceneViewport() {
        const auto depthFormat = renderer.getSwapChain().findDepthFormat();
        const auto initialSize = imgui ? imgui->getSceneViewportSize() : ImVec2(900.0f, 600.0f);
        const VkExtent2D initialExtent{
            static_cast<uint32_t>(std::max(1.0f, initialSize.x)),
            static_cast<uint32_t>(std::max(1.0f, initialSize.y))};

        sceneViewportTarget = std::make_unique<SceneViewportTarget>(
            device,
            VK_FORMAT_R8G8B8A8_SRGB,
            depthFormat,
            initialExtent);

        sceneRenderSystem = std::make_unique<RenderSystem>(
            device,
            sceneViewportTarget->getRenderPass(),
            imgui.get());
    }

    void FeooApp::renderSceneViewport(VkCommandBuffer commandBuffer) {
        if (!sceneViewportTarget || !sceneRenderSystem) {
            return;
        }

        const ImVec2 requestedSize = imgui ? imgui->getSceneViewportSize() : ImVec2(900.0f, 600.0f);
        const VkExtent2D viewportExtent{
            static_cast<uint32_t>(std::max(1.0f, requestedSize.x)),
            static_cast<uint32_t>(std::max(1.0f, requestedSize.y))};

        sceneViewportTarget->resize(viewportExtent);
        sceneViewportTarget->beginRenderPass(commandBuffer);
        sceneRenderSystem->renderGameObjects(commandBuffer, gameobjects, camera);
        sceneViewportTarget->endRenderPass(commandBuffer);

        imgui->setSceneViewportTexture(static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(sceneViewportTarget->getDescriptorSet())));
        imgui->setSceneViewportSize(ImVec2(static_cast<float>(viewportExtent.width), static_cast<float>(viewportExtent.height)));
    }

    void FeooApp::initScene(const RenderSystem &renderSystem) {
        sceneManager.setScriptFactory([this](const std::string &scriptName) {
            return createScriptByName(scriptName);
        });

        auto context = buildScriptContext(renderSystem);

        if (!sceneManager.loadScene("default.scene", context)) {
            sceneManager.createNewScene("default.scene", context);
            sceneManager.saveScene("default.scene");
        }
    }

    void FeooApp::drawSceneManagerUi(const RenderSystem &renderSystem) {
        auto context = buildScriptContext(renderSystem);
        sceneManager.drawImGui(context);
    }

    std::unique_ptr<FeooScript> FeooApp::createScriptByName(const std::string &scriptName) const {
        if (scriptName == "FuseeRotationScript") {
            return std::make_unique<FuseeRotationScript>();
        }
        if (scriptName == "CatgirlRotationScript") {
            return std::make_unique<CatgirlRotationScript>();
        }
        if (scriptName == "SpinModelsScript") {
            return std::make_unique<SpinModelsScript>();
        }
        if (scriptName == "MovementScript") {
            return std::make_unique<MovementScript>();
        }
        if (scriptName == "CameraScript") {
            return std::make_unique<CameraScript>();
        }

        return nullptr;
    }

    void FeooApp::initImgui() {
        imgui = std::make_unique<FeooImgui>();
        imgui->init(device, renderer.getSwapChain(), window.getGLFWwindow());
    }
}
