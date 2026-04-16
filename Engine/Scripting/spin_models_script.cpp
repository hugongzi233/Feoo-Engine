#include "spin_models_script.hpp"

#include "../Core/feoo_model.hpp"

#include <algorithm>
#include <optional>

namespace feoo {
    namespace {
        std::filesystem::path resolveAssetPath(const std::filesystem::path &relativePath) {
            const std::filesystem::path current = std::filesystem::current_path();
            for (auto searchRoot = current; !searchRoot.empty(); searchRoot = searchRoot.parent_path()) {
                const auto candidate = searchRoot / relativePath;
                if (std::filesystem::exists(candidate)) {
                    return candidate;
                }
            }
            return current / relativePath;
        }

        FeooGameObject *findGameObject(std::vector<FeooGameObject> &gameObjects, FeooGameObject::id_t id) {
            const auto it = std::find_if(gameObjects.begin(), gameObjects.end(), [id](const FeooGameObject &object) {
                return object.getId() == id;
            });
            return it == gameObjects.end() ? nullptr : &(*it);
        }

        void spawnRotatingModel(const std::filesystem::path &modelPath,
                                const glm::vec3 &translation,
                                const glm::vec3 &scale,
                                const ScriptContext &context,
                                std::optional<FeooGameObject::id_t> &spawnedObjectId) {
            if (spawnedObjectId.has_value()) {
                return;
            }

            auto model = FeooModel::createModelFromFile(context.device, modelPath, context.textureSetLayout);

            auto gameObject = FeooGameObject::createGameObject();
            gameObject.model = std::move(model);
            gameObject.transform.translation = translation;
            gameObject.transform.scale = scale;
            gameObject.color = glm::vec3(1.0f, 1.0f, 1.0f);

            spawnedObjectId = gameObject.getId();
            context.gameObjects.push_back(std::move(gameObject));
        }

        void spinRotatingModel(float deltaTime,
                               const ScriptContext &context,
                               std::optional<FeooGameObject::id_t> &spawnedObjectId,
                               float spinSpeed) {
            if (!spawnedObjectId.has_value()) {
                return;
            }

            if (auto *gameObject = findGameObject(context.gameObjects, *spawnedObjectId)) {
                gameObject->transform.rotation.y += deltaTime * spinSpeed;
            }
        }

        void destroyRotatingModel(const ScriptContext &context, std::optional<FeooGameObject::id_t> &spawnedObjectId) {
            if (!spawnedObjectId.has_value()) {
                return;
            }

            if (auto *gameObject = findGameObject(context.gameObjects, *spawnedObjectId)) {
                gameObject->model.reset();
            }

            spawnedObjectId.reset();
        }
    }

    void SpinModelsScript::onStart(ScriptGameObject &, const ScriptContext &context) {
        rotatingObjectIds.clear();
        spawnedObjectIds.clear();

        const auto fuseePath = resolveAssetPath("Engine/Resources/Models/fusee1.fbx");
        const auto catgirlPath = resolveAssetPath("Engine/Resources/Models/catgirl.obj");

        auto fuseeModel = FeooModel::createModelFromFile(context.device, fuseePath, context.textureSetLayout);
        auto catgirlModel = FeooModel::createModelFromFile(context.device, catgirlPath, context.textureSetLayout);

        auto fuseeObject = FeooGameObject::createGameObject();
        fuseeObject.model = std::move(fuseeModel);
        fuseeObject.transform.translation = glm::vec3(-5.0f, 0.0f, 0.0f);
        fuseeObject.transform.scale = glm::vec3(1.75f, 1.75f, 1.75f);
        fuseeObject.color = glm::vec3(1.0f, 1.0f, 1.0f);
        rotatingObjectIds.push_back(fuseeObject.getId());
        spawnedObjectIds.push_back(fuseeObject.getId());
        context.gameObjects.push_back(std::move(fuseeObject));

        auto catgirlObject = FeooGameObject::createGameObject();
        catgirlObject.model = std::move(catgirlModel);
        catgirlObject.transform.translation = glm::vec3(5.0f, 0.0f, 0.0f);
        catgirlObject.transform.scale = glm::vec3(1.75f, 1.75f, 1.75f);
        catgirlObject.color = glm::vec3(1.0f, 1.0f, 1.0f);
        rotatingObjectIds.push_back(catgirlObject.getId());
        spawnedObjectIds.push_back(catgirlObject.getId());
        context.gameObjects.push_back(std::move(catgirlObject));
    }

    void SpinModelsScript::onUpdate(float deltaTime, ScriptGameObject &, const ScriptContext &context) {
        for (auto &object : context.gameObjects) {
            if (std::find(rotatingObjectIds.begin(), rotatingObjectIds.end(), object.getId()) != rotatingObjectIds.end()) {
                object.transform.rotation.y += deltaTime * 0.75f;
            }
        }
    }

    void SpinModelsScript::onDestroy(ScriptGameObject &, const ScriptContext &context) {
        for (auto &obj : context.gameObjects) {
            if (std::find(spawnedObjectIds.begin(), spawnedObjectIds.end(), obj.getId()) != spawnedObjectIds.end()) {
                obj.model.reset();
            }
        }

        rotatingObjectIds.clear();
        spawnedObjectIds.clear();
    }

    void FuseeRotationScript::onStart(ScriptGameObject &, const ScriptContext &context) {
        if (spawnedObjectId.has_value()) {
            return;
        }

        const auto fuseePath = resolveAssetPath("Engine/Resources/Models/fusee1.fbx");
        spawnRotatingModel(fuseePath, glm::vec3(-5.0f, 0.0f, 0.0f), glm::vec3(1.75f, 1.75f, 1.75f), context, spawnedObjectId);
    }

    void FuseeRotationScript::onUpdate(float deltaTime, ScriptGameObject &, const ScriptContext &context) {
        spinRotatingModel(deltaTime, context, spawnedObjectId, 0.75f);
    }

    void FuseeRotationScript::onDestroy(ScriptGameObject &, const ScriptContext &context) {
        destroyRotatingModel(context, spawnedObjectId);
    }

    void CatgirlRotationScript::onStart(ScriptGameObject &, const ScriptContext &context) {
        if (spawnedObjectId.has_value()) {
            return;
        }

        const auto catgirlPath = resolveAssetPath("Engine/Resources/Models/catgirl.obj");
        spawnRotatingModel(catgirlPath, glm::vec3(5.0f, 0.0f, 0.0f), glm::vec3(1.75f, 1.75f, 1.75f), context, spawnedObjectId);
    }

    void CatgirlRotationScript::onUpdate(float deltaTime, ScriptGameObject &, const ScriptContext &context) {
        spinRotatingModel(deltaTime, context, spawnedObjectId, 0.75f);
    }

    void CatgirlRotationScript::onDestroy(ScriptGameObject &, const ScriptContext &context) {
        destroyRotatingModel(context, spawnedObjectId);
    }
}
