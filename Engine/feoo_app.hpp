#pragma once

#include "Window/feoo_window.hpp"
#include "Core/feoo_pipeline.hpp"
#include "Core/feoo_device.hpp"
#include "Core/feoo_game_object.hpp"
#include "Core/feoo_renderer.hpp"
#include "UI/feoo_imgui.hpp"

#include <memory>
#include <vector>

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

        void loadGameObjects();

        void update(float deltaTime);

        FeooWindow window{WIDTH, HEIGHT, "Feoo GameEngine Application"};
        FeooDevice device{window};
        FeooRenderer renderer{window, device};

        std::vector<FeooGameObject> gameobjects;
        std::unique_ptr<FeooImgui> imgui;
    };
}
