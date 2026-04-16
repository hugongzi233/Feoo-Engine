#include "keyboard_movement_controller.hpp"

// std
#include <limits>

namespace feoo {
    void KeyboardMovementController::moveInPlaneXZ(GLFWwindow *window, float dt, FeooGameObject &gameObject) {
        moveInPlaneXZ(window, dt, gameObject.transform, true);
    }

    void KeyboardMovementController::moveInPlaneXZ(GLFWwindow *window, float dt, Transform &transform, bool enableInput) {
        if (!enableInput) {
            if (cursorCaptured) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                if (glfwRawMouseMotionSupported()) {
                    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
                }
                cursorCaptured = false;
                hasLastMousePosition = false;
            }
            escWasPressed = false;
            leftMouseWasPressed = false;
            return;
        }

        if (!initialCaptureDone) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            if (glfwRawMouseMotionSupported()) {
                glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            }
            initialCaptureDone = true;
            cursorCaptured = true;
            hasLastMousePosition = false;
        }

        const bool escPressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        if (escPressed && !escWasPressed && cursorCaptured) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            if (glfwRawMouseMotionSupported()) {
                glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
            }
            cursorCaptured = false;
            hasLastMousePosition = false;
        }
        escWasPressed = escPressed;

        const bool leftMousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (leftMousePressed && !leftMouseWasPressed && !cursorCaptured) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            if (glfwRawMouseMotionSupported()) {
                glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            }
            cursorCaptured = true;
            hasLastMousePosition = false;
        }
        leftMouseWasPressed = leftMousePressed;

        if (cursorCaptured) {
            double mouseX = 0.0;
            double mouseY = 0.0;
            glfwGetCursorPos(window, &mouseX, &mouseY);

            if (hasLastMousePosition) {
                const float deltaX = static_cast<float>(mouseX - lastMouseX);
                const float deltaY = static_cast<float>(mouseY - lastMouseY);

                transform.rotation.y += lookSpeed * deltaX;
                transform.rotation.x -= lookSpeed * deltaY;
            }

            lastMouseX = mouseX;
            lastMouseY = mouseY;
            hasLastMousePosition = true;
        }

        // limit pitch values between about +/- 85ish degrees
        transform.rotation.x = glm::clamp(transform.rotation.x, -1.5f, 1.5f);
        transform.rotation.y = glm::mod(transform.rotation.y, glm::two_pi<float>());

        float yaw = transform.rotation.y;
        const glm::vec3 forwardDir{sin(yaw), 0.f, cos(yaw)};
        const glm::vec3 rightDir{forwardDir.z, 0.f, -forwardDir.x};
        const glm::vec3 upDir{0.f, -1.f, 0.f};

        glm::vec3 moveDir{0.f};
        if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
        if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
        if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
        if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
        if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
        if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;

        if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
            transform.translation += moveSpeed * dt * glm::normalize(moveDir);
        }
    }
} // namespace feoo
