#pragma once

#include "Window/gola_window.hpp"
#include "Core/gola_pipeline.hpp"
#include "Core/gola_device.hpp"
#include "Core/gola_game_object.hpp"
#include "Core/gola_renderer.hpp"
#include "UI/gola_imgui.hpp"

#include <memory>
#include <vector>

namespace gola {
    class GolaApp {
    public:
        static constexpr int WIDTH = 1280;
        static constexpr int HEIGHT = 960;

        GolaApp();

        ~GolaApp();

        GolaApp(const GolaApp &) = delete;

        GolaApp &operator=(const GolaApp &) = delete;

        void run();

    private:
        void initImgui();

        void loadGameObjects();

        void update(float deltaTime);

        GolaWindow window{WIDTH, HEIGHT, "Gola GameEngine Application"};
        GolaDevice device{window};
        GolaRenderer renderer{window, device};

        std::vector<GolaGameObject> gameobjects;
        std::unique_ptr<GolaImgui> imgui;
    };
}
