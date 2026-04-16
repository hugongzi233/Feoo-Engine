#pragma once

#include "feoo_script.hpp"

#include <filesystem>
#include <optional>
#include <vector>

namespace feoo {
    class SpinModelsScript : public FeooScript {
    public:
        const char *getTypeName() const override { return "SpinModelsScript"; }
        std::unique_ptr<FeooScript> clone() const override { return std::make_unique<SpinModelsScript>(); }

        void onStart(ScriptGameObject &owner, const ScriptContext &context) override;
        void onUpdate(float deltaTime, ScriptGameObject &owner, const ScriptContext &context) override;
        void onDestroy(ScriptGameObject &owner, const ScriptContext &context) override;

    private:
        std::vector<FeooGameObject::id_t> rotatingObjectIds;
        std::vector<FeooGameObject::id_t> spawnedObjectIds;
    };

    class FuseeRotationScript : public FeooScript {
    public:
        const char *getTypeName() const override { return "FuseeRotationScript"; }
        std::unique_ptr<FeooScript> clone() const override { return std::make_unique<FuseeRotationScript>(); }

        void onStart(ScriptGameObject &owner, const ScriptContext &context) override;
        void onUpdate(float deltaTime, ScriptGameObject &owner, const ScriptContext &context) override;
        void onDestroy(ScriptGameObject &owner, const ScriptContext &context) override;

    private:
        std::optional<FeooGameObject::id_t> spawnedObjectId;
    };

    class CatgirlRotationScript : public FeooScript {
    public:
        const char *getTypeName() const override { return "CatgirlRotationScript"; }
        std::unique_ptr<FeooScript> clone() const override { return std::make_unique<CatgirlRotationScript>(); }

        void onStart(ScriptGameObject &owner, const ScriptContext &context) override;
        void onUpdate(float deltaTime, ScriptGameObject &owner, const ScriptContext &context) override;
        void onDestroy(ScriptGameObject &owner, const ScriptContext &context) override;

    private:
        std::optional<FeooGameObject::id_t> spawnedObjectId;
    };
}
