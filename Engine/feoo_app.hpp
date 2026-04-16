#pragma once

#include "Core/render_system.hpp"
#include "Core/feoo_camera.hpp"
#include "Core/scene_viewport_target.hpp"
#include "Window/feoo_window.hpp"
#include "Core/feoo_pipeline.hpp"
#include "Core/feoo_device.hpp"
#include "Core/feoo_game_object.hpp"
#include "Core/render_system.hpp"
#include "Core/feoo_renderer.hpp"
#include "UI/feoo_imgui.hpp"
#include "Scripting/feoo_script.hpp"
#include "Scene/scene_manager.hpp"

#include <memory>
#include <vector>
#include <filesystem>

namespace feoo {
    class FeooApp {
    public:
        static constexpr int WIDTH = 1280;
        static constexpr int HEIGHT = 960;

        FeooApp();

        ~FeooApp();

        FeooApp(const FeooApp &) = delete;

        FeooApp &operator=(const FeooApp &) = delete;

        void run();

    private:
        void initImgui();
        void initSceneViewport();
        ScriptContext buildScriptContext(const RenderSystem &renderSystem);
        void initScene(const RenderSystem &renderSystem);
        void updateScene(float deltaTime, const RenderSystem &renderSystem);
        void renderSceneViewport(VkCommandBuffer commandBuffer);
        void drawSceneManagerUi(const RenderSystem &renderSystem);
        std::unique_ptr<FeooScript> createScriptByName(const std::string &scriptName) const;

        FeooWindow window{WIDTH, HEIGHT, "Feoo GameEngine Application"};
        FeooDevice device{window};
        FeooRenderer renderer{window, device};
        FeooCamera camera{};
        std::unique_ptr<SceneViewportTarget> sceneViewportTarget;
        std::unique_ptr<RenderSystem> sceneRenderSystem;

        std::vector<FeooGameObject> gameobjects;
        std::unique_ptr<FeooImgui> imgui;
        SceneManager sceneManager{"Engine/Scene/Scenes"};
    };
}
