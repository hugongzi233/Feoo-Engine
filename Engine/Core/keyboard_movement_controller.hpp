#pragma once

#include "feoo_game_object.hpp"
#include "../Window/feoo_window.hpp"

namespace feoo {
    class KeyboardMovementController {
    public:
        struct KeyMappings {
            int moveLeft = GLFW_KEY_A;
            int moveRight = GLFW_KEY_D;
            int moveForward = GLFW_KEY_W;
            int moveBackward = GLFW_KEY_S;
            int moveUp = GLFW_KEY_SPACE;
            int moveDown = GLFW_KEY_LEFT_CONTROL;
            int lookLeft = GLFW_KEY_LEFT;
            int lookRight = GLFW_KEY_RIGHT;
            int lookUp = GLFW_KEY_UP;
            int lookDown = GLFW_KEY_DOWN;
        };

        void moveInPlaneXZ(GLFWwindow *window, float dt, Transform &transform, bool enableInput = true);
        void moveInPlaneXZ(GLFWwindow *window, float dt, FeooGameObject &gameObject);

        KeyMappings keys{};
        float moveSpeed{3.f};

        // radians per pixel
        float lookSpeed{0.0025f};

    private:
        bool initialCaptureDone{false};
        bool cursorCaptured{false};
        bool hasLastMousePosition{false};
        bool escWasPressed{false};
        bool leftMouseWasPressed{false};
        double lastMouseX{0.0};
        double lastMouseY{0.0};
    };
} // namespace feoo
