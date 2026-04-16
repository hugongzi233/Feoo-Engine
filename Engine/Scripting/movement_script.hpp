#pragma once

#include "feoo_script.hpp"
#include "../Core/keyboard_movement_controller.hpp"

namespace feoo {
    class MovementScript : public FeooScript {
    public:
        const char *getTypeName() const override { return "MovementScript"; }
        std::unique_ptr<FeooScript> clone() const override { return std::make_unique<MovementScript>(); }

        void onStart(ScriptGameObject &owner, const ScriptContext &context) override;
        void onUpdate(float deltaTime, ScriptGameObject &owner, const ScriptContext &context) override;

    private:
        KeyboardMovementController movementController{};
    };
}
