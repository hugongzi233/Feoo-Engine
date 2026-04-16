#pragma once

#include "../Core/feoo_camera.hpp"
#include "../Core/feoo_device.hpp"
#include "../Core/feoo_game_object.hpp"
#include "../Core/feoo_renderer.hpp"
#include "../Window/feoo_window.hpp"

#include <memory>
#include <string>
#include <vector>

namespace feoo {
    struct ScriptGameObject {
        std::string name{"GameObject"};
        Transform transform{};
        bool enabled{true};
    };

    struct ScriptContext {
        FeooDevice &device;
        FeooWindow &window;
        FeooRenderer &renderer;
        FeooCamera &camera;
        std::vector<FeooGameObject> &gameObjects;
        bool sceneInputEnabled{false};
        VkDescriptorSetLayout textureSetLayout = VK_NULL_HANDLE;
    };

    class FeooScript {
    public:
        virtual ~FeooScript() = default;

        virtual const char *getTypeName() const = 0;
        virtual std::unique_ptr<FeooScript> clone() const = 0;

        virtual void onStart(ScriptGameObject &, const ScriptContext &) {}
        virtual void onUpdate(float, ScriptGameObject &, const ScriptContext &) {}
        virtual void onDestroy(ScriptGameObject &, const ScriptContext &) {}
    };
}
