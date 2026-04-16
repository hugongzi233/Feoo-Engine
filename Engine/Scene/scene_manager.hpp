#pragma once

#include "../Scripting/feoo_script.hpp"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <array>
#include <vector>

namespace feoo {
    class SceneManager {
    public:
        using ScriptFactory = std::function<std::unique_ptr<FeooScript>(const std::string &)>;

        explicit SceneManager(std::filesystem::path sceneDirectory);

        void setScriptFactory(ScriptFactory factory);

        bool createNewScene(const std::string &sceneName, const ScriptContext &context);
        bool loadScene(const std::string &sceneName, const ScriptContext &context);
        bool saveScene(const std::string &sceneName) const;
        bool deleteScene(const std::string &sceneName);
        void unloadScene(const ScriptContext &context);

        void update(float deltaTime, const ScriptContext &context);
        void drawImGui(const ScriptContext &context);

        const std::string &getActiveSceneName() const { return activeSceneName; }

    private:
        struct SceneObjectEntry {
            ScriptGameObject object{};
            std::vector<std::unique_ptr<FeooScript>> scripts;
        };

        std::filesystem::path makeScenePath(const std::string &sceneName) const;
        std::string normalizeSceneName(const std::string &sceneName) const;
        void refreshSceneFiles();
        void drawSceneControlsWindow(const ScriptContext &context);
        void drawHierarchyWindow(const ScriptContext &context);
        void drawInspectorWindow(const ScriptContext &context);
        void createEmptyGameObject(const std::string &baseName = "EmptyGameObject");
        void deleteSceneObjectAt(int objectIndex, const ScriptContext *context = nullptr);

        void startCurrentScene(const ScriptContext &context);
        void stopCurrentScene(const ScriptContext &context);

        bool attachScriptByName(SceneObjectEntry &entry, const std::string &scriptTypeName);
        SceneObjectEntry *getSelectedSceneObject();
        const SceneObjectEntry *getSelectedSceneObject() const;

        std::filesystem::path sceneDirectory;
        ScriptFactory scriptFactory;

        std::vector<SceneObjectEntry> sceneObjects;
        std::vector<std::string> sceneFiles;
        std::vector<std::string> availableScriptTypes{
            "FuseeRotationScript",
            "CatgirlRotationScript",
            "SpinModelsScript",
            "MovementScript",
            "CameraScript"};

        std::string activeSceneName;
        std::string sceneNameDraft{"default"};
        int selectedSceneIndex{0};
        int selectedObjectIndex{-1};
        int renameObjectIndex{-1};
        std::array<char, 128> renameObjectBuffer{};
        bool sceneStarted{false};
    };
}
