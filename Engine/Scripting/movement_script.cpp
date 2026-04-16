#include "movement_script.hpp"

namespace feoo {
    void MovementScript::onStart(ScriptGameObject &owner, const ScriptContext &) {
        owner.transform.translation = glm::vec3(0.0f, 0.0f, -12.0f);
    }

    void MovementScript::onUpdate(float deltaTime, ScriptGameObject &owner, const ScriptContext &context) {
        movementController.moveInPlaneXZ(
            context.window.getGLFWwindow(),
            deltaTime,
            owner.transform,
            context.sceneInputEnabled);
    }
}
