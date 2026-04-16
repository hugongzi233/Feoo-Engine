#include "camera_script.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace feoo {
    void CameraScript::onUpdate(float, ScriptGameObject &owner, const ScriptContext &context) {
        context.camera.setViewYXZ(owner.transform.translation, owner.transform.rotation);
        context.camera.setPerspectiveProjection(
            glm::radians(fovDegrees),
            context.renderer.getAspectRatio(),
            nearPlane,
            farPlane);
    }
}
