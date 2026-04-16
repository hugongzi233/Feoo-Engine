#pragma once

#include "feoo_script.hpp"

namespace feoo {
    class CameraScript : public FeooScript {
    public:
        const char *getTypeName() const override { return "CameraScript"; }
        std::unique_ptr<FeooScript> clone() const override { return std::make_unique<CameraScript>(); }

        void onUpdate(float deltaTime, ScriptGameObject &owner, const ScriptContext &context) override;

    private:
        float fovDegrees{50.0f};
        float nearPlane{0.1f};
        float farPlane{10000.0f};
    };
}
