#include "scene_manager.hpp"

#include "imgui.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cstdio>

namespace feoo {
    namespace {
        std::string trim(const std::string &value) {
            size_t begin = 0;
            while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
                ++begin;
            }

            size_t end = value.size();
            while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
                --end;
            }

            return value.substr(begin, end - begin);
        }

        void copyStringToBuffer(std::array<char, 128> &buffer, const std::string &value) {
            std::snprintf(buffer.data(), buffer.size(), "%s", value.c_str());
        }

        void waitForGpuIdle(const ScriptContext &context) {
            vkDeviceWaitIdle(context.device.device());
        }
    }

    SceneManager::SceneManager(std::filesystem::path sceneDirectory)
        : sceneDirectory(std::move(sceneDirectory)) {
        std::filesystem::create_directories(this->sceneDirectory);
        refreshSceneFiles();
    }

    void SceneManager::setScriptFactory(ScriptFactory factory) {
        scriptFactory = std::move(factory);
    }

    std::string SceneManager::normalizeSceneName(const std::string &sceneName) const {
        const std::string cleaned = trim(sceneName);
        if (cleaned.empty()) {
            return "untitled.scene";
        }

        if (cleaned.ends_with(".scene")) {
            return cleaned;
        }
        return cleaned + ".scene";
    }

    std::filesystem::path SceneManager::makeScenePath(const std::string &sceneName) const {
        return sceneDirectory / normalizeSceneName(sceneName);
    }

    void SceneManager::refreshSceneFiles() {
        sceneFiles.clear();

        if (!std::filesystem::exists(sceneDirectory)) {
            selectedSceneIndex = 0;
            return;
        }

        for (const auto &entry : std::filesystem::directory_iterator(sceneDirectory)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            if (entry.path().extension() == ".scene") {
                sceneFiles.push_back(entry.path().filename().string());
            }
        }

        std::sort(sceneFiles.begin(), sceneFiles.end());
        if (sceneFiles.empty()) {
            selectedSceneIndex = 0;
        } else {
            selectedSceneIndex = std::clamp(selectedSceneIndex, 0, static_cast<int>(sceneFiles.size()) - 1);
        }
    }

    bool SceneManager::attachScriptByName(SceneObjectEntry &entry, const std::string &scriptTypeName) {
        if (!scriptFactory) {
            return false;
        }

        auto script = scriptFactory(scriptTypeName);
        if (!script) {
            return false;
        }

        entry.scripts.push_back(std::move(script));
        return true;
    }

    SceneManager::SceneObjectEntry *SceneManager::getSelectedSceneObject() {
        if (selectedObjectIndex < 0 || selectedObjectIndex >= static_cast<int>(sceneObjects.size())) {
            return nullptr;
        }
        return &sceneObjects[static_cast<size_t>(selectedObjectIndex)];
    }

    const SceneManager::SceneObjectEntry *SceneManager::getSelectedSceneObject() const {
        if (selectedObjectIndex < 0 || selectedObjectIndex >= static_cast<int>(sceneObjects.size())) {
            return nullptr;
        }
        return &sceneObjects[static_cast<size_t>(selectedObjectIndex)];
    }

    void SceneManager::startCurrentScene(const ScriptContext &context) {
        if (sceneStarted) {
            return;
        }

        for (auto &entry : sceneObjects) {
            for (auto &script : entry.scripts) {
                script->onStart(entry.object, context);
            }
        }

        sceneStarted = true;
    }

    void SceneManager::stopCurrentScene(const ScriptContext &context) {
        if (!sceneStarted) {
            return;
        }

        waitForGpuIdle(context);

        for (auto &entry : sceneObjects) {
            for (auto &script : entry.scripts) {
                script->onDestroy(entry.object, context);
            }
        }

        sceneStarted = false;
    }

    bool SceneManager::createNewScene(const std::string &sceneName, const ScriptContext &context) {
        if (!scriptFactory) {
            return false;
        }

        stopCurrentScene(context);

        sceneObjects.clear();
        activeSceneName = normalizeSceneName(sceneName);
        sceneNameDraft = activeSceneName;
        selectedObjectIndex = -1;

        SceneObjectEntry fuseeObject{};
        fuseeObject.object.name = "FuseeModel";
        attachScriptByName(fuseeObject, "FuseeRotationScript");
        sceneObjects.push_back(std::move(fuseeObject));

        SceneObjectEntry catgirlObject{};
        catgirlObject.object.name = "CatgirlModel";
        attachScriptByName(catgirlObject, "CatgirlRotationScript");
        sceneObjects.push_back(std::move(catgirlObject));

        SceneObjectEntry playerController{};
        playerController.object.name = "PlayerController";
        playerController.object.transform.translation = glm::vec3(0.0f, 0.0f, -12.0f);
        attachScriptByName(playerController, "MovementScript");
        attachScriptByName(playerController, "CameraScript");
        sceneObjects.push_back(std::move(playerController));
        selectedObjectIndex = 0;

        startCurrentScene(context);
        refreshSceneFiles();
        return true;
    }

    bool SceneManager::loadScene(const std::string &sceneName, const ScriptContext &context) {
        if (!scriptFactory) {
            return false;
        }

        const auto scenePath = makeScenePath(sceneName);
        if (!std::filesystem::exists(scenePath)) {
            return false;
        }

        std::ifstream input(scenePath);
        if (!input.is_open()) {
            return false;
        }

        std::vector<SceneObjectEntry> loadedObjects;
        std::string token;

        while (input >> token) {
            if (token == "object") {
                SceneObjectEntry entry{};
                input >> std::quoted(entry.object.name);

                while (input >> token) {
                    if (token == "end_object") {
                        break;
                    }
                    if (token == "enabled") {
                        int enabled = 1;
                        input >> enabled;
                        entry.object.enabled = enabled != 0;
                    } else if (token == "translation") {
                        input >> entry.object.transform.translation.x
                              >> entry.object.transform.translation.y
                              >> entry.object.transform.translation.z;
                    } else if (token == "rotation") {
                        input >> entry.object.transform.rotation.x
                              >> entry.object.transform.rotation.y
                              >> entry.object.transform.rotation.z;
                    } else if (token == "scale") {
                        input >> entry.object.transform.scale.x
                              >> entry.object.transform.scale.y
                              >> entry.object.transform.scale.z;
                    } else if (token == "script") {
                        std::string scriptTypeName;
                        input >> std::quoted(scriptTypeName);
                        attachScriptByName(entry, scriptTypeName);
                    }
                }

                loadedObjects.push_back(std::move(entry));
            }
        }

        stopCurrentScene(context);
        sceneObjects = std::move(loadedObjects);
        activeSceneName = normalizeSceneName(sceneName);
        sceneNameDraft = activeSceneName;
        selectedObjectIndex = sceneObjects.empty() ? -1 : 0;
        startCurrentScene(context);

        refreshSceneFiles();
        return true;
    }

    bool SceneManager::saveScene(const std::string &sceneName) const {
        const auto scenePath = makeScenePath(sceneName);
        std::ofstream output(scenePath, std::ios::trunc);
        if (!output.is_open()) {
            return false;
        }

        for (const auto &entry : sceneObjects) {
            output << "object " << std::quoted(entry.object.name) << "\n";
            output << "enabled " << (entry.object.enabled ? 1 : 0) << "\n";
            output << "translation "
                   << entry.object.transform.translation.x << " "
                   << entry.object.transform.translation.y << " "
                   << entry.object.transform.translation.z << "\n";
            output << "rotation "
                   << entry.object.transform.rotation.x << " "
                   << entry.object.transform.rotation.y << " "
                   << entry.object.transform.rotation.z << "\n";
            output << "scale "
                   << entry.object.transform.scale.x << " "
                   << entry.object.transform.scale.y << " "
                   << entry.object.transform.scale.z << "\n";

            for (const auto &script : entry.scripts) {
                output << "script " << std::quoted(script->getTypeName()) << "\n";
            }
            output << "end_object\n";
        }

        return true;
    }

    bool SceneManager::deleteScene(const std::string &sceneName) {
        const auto scenePath = makeScenePath(sceneName);
        if (!std::filesystem::exists(scenePath)) {
            return false;
        }

        const bool removed = std::filesystem::remove(scenePath);
        refreshSceneFiles();
        return removed;
    }

    void SceneManager::unloadScene(const ScriptContext &context) {
        stopCurrentScene(context);
        sceneObjects.clear();
        activeSceneName.clear();
        selectedObjectIndex = -1;
    }

    void SceneManager::update(float deltaTime, const ScriptContext &context) {
        if (!sceneStarted) {
            return;
        }

        for (auto &entry : sceneObjects) {
            if (!entry.object.enabled) {
                continue;
            }
            for (auto &script : entry.scripts) {
                script->onUpdate(deltaTime, entry.object, context);
            }
        }
    }

    void SceneManager::drawImGui(const ScriptContext &context) {
        refreshSceneFiles();

        drawSceneControlsWindow(context);
        drawHierarchyWindow(context);
        drawInspectorWindow(context);
    }

    void SceneManager::drawSceneControlsWindow(const ScriptContext &context) {
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 12.0f, viewport->WorkPos.y + 40.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(320.0f, 260.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Scene Manager");

        std::array<char, 128> sceneNameBuffer{};
        copyStringToBuffer(sceneNameBuffer, sceneNameDraft);
        if (ImGui::InputText("Scene Name", sceneNameBuffer.data(), sceneNameBuffer.size())) {
            sceneNameDraft = sceneNameBuffer.data();
        }

        if (ImGui::Button("New Scene")) {
            createNewScene(sceneNameDraft, context);
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Scene")) {
            const std::string name = trim(sceneNameDraft);
            if (!name.empty()) {
                activeSceneName = normalizeSceneName(name);
                sceneNameDraft = activeSceneName;
                saveScene(activeSceneName);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Unload Scene")) {
            unloadScene(context);
        }

        if (!sceneFiles.empty()) {
            if (selectedSceneIndex < 0 || selectedSceneIndex >= static_cast<int>(sceneFiles.size())) {
                selectedSceneIndex = 0;
            }

            const char *currentScene = sceneFiles[selectedSceneIndex].c_str();
            if (ImGui::BeginCombo("Saved Scenes", currentScene)) {
                for (int i = 0; i < static_cast<int>(sceneFiles.size()); ++i) {
                    const bool selected = selectedSceneIndex == i;
                    if (ImGui::Selectable(sceneFiles[i].c_str(), selected)) {
                        selectedSceneIndex = i;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            if (ImGui::Button("Load Selected")) {
                loadScene(sceneFiles[selectedSceneIndex], context);
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete Selected")) {
                deleteScene(sceneFiles[selectedSceneIndex]);
            }
        }

        ImGui::End();
    }

    void SceneManager::createEmptyGameObject(const std::string &baseName) {
        SceneObjectEntry entry{};
        entry.object.name = baseName;
        sceneObjects.push_back(std::move(entry));
        selectedObjectIndex = static_cast<int>(sceneObjects.size()) - 1;
    }

    void SceneManager::deleteSceneObjectAt(int objectIndex, const ScriptContext *context) {
        if (objectIndex < 0 || objectIndex >= static_cast<int>(sceneObjects.size())) {
            return;
        }

        if (context != nullptr) {
            waitForGpuIdle(*context);
            for (auto &script : sceneObjects[static_cast<size_t>(objectIndex)].scripts) {
                script->onDestroy(sceneObjects[static_cast<size_t>(objectIndex)].object, *context);
            }
        }

        sceneObjects.erase(sceneObjects.begin() + objectIndex);
        if (sceneObjects.empty()) {
            selectedObjectIndex = -1;
        } else if (selectedObjectIndex >= static_cast<int>(sceneObjects.size())) {
            selectedObjectIndex = static_cast<int>(sceneObjects.size()) - 1;
        }
    }

    void SceneManager::drawHierarchyWindow(const ScriptContext &context) {
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 12.0f, viewport->WorkPos.y + 320.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(320.0f, 360.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Hierarchy");

        if (ImGui::Button("Create Empty")) {
            createEmptyGameObject();
        }

        ImGui::SeparatorText("Scene");
        const ImGuiTreeNodeFlags sceneFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
        const bool sceneTreeOpen = ImGui::TreeNodeEx("SceneRoot", sceneFlags, "Scene");

        if (ImGui::BeginPopupContextItem("SceneRootContext")) {
            if (ImGui::MenuItem("Create Empty")) {
                createEmptyGameObject();
            }
            ImGui::EndPopup();
        }

        if (sceneTreeOpen) {
            for (int i = 0; i < static_cast<int>(sceneObjects.size()); ++i) {
                ImGui::PushID(i);
                const bool selected = selectedObjectIndex == i;
                ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf |
                                               ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                               ImGuiTreeNodeFlags_SpanAvailWidth;
                if (selected) {
                    nodeFlags |= ImGuiTreeNodeFlags_Selected;
                }

                ImGui::TreeNodeEx("GameObject", nodeFlags, "%s", sceneObjects[static_cast<size_t>(i)].object.name.c_str());
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    selectedObjectIndex = i;
                }

                if (ImGui::BeginPopupContextItem("GameObjectContext")) {
                    if (ImGui::MenuItem("Create Empty")) {
                        createEmptyGameObject();
                    }
                    if (ImGui::MenuItem("Rename")) {
                        renameObjectIndex = i;
                        copyStringToBuffer(renameObjectBuffer, sceneObjects[static_cast<size_t>(i)].object.name);
                        ImGui::OpenPopup("Rename GameObject");
                    }
                    if (ImGui::MenuItem("Delete")) {
                        deleteSceneObjectAt(i, &context);
                        ImGui::EndPopup();
                        ImGui::PopID();
                        break;
                    }
                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }
            ImGui::TreePop();
        }

        if (ImGui::BeginPopupModal("Rename GameObject", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("Name", renameObjectBuffer.data(), renameObjectBuffer.size());
            if (ImGui::Button("OK")) {
                if (renameObjectIndex >= 0 && renameObjectIndex < static_cast<int>(sceneObjects.size())) {
                    const std::string newName = trim(renameObjectBuffer.data());
                    if (!newName.empty()) {
                        sceneObjects[static_cast<size_t>(renameObjectIndex)].object.name = newName;
                    }
                }
                renameObjectIndex = -1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                renameObjectIndex = -1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupContextWindow("HierarchyEmptyContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
            if (ImGui::MenuItem("Create Empty")) {
                createEmptyGameObject();
            }
            ImGui::EndPopup();
        }

        ImGui::End();
    }

    void SceneManager::drawInspectorWindow(const ScriptContext &context) {
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - 360.0f, viewport->WorkPos.y + 40.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340.0f, 640.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Inspector");

        SceneObjectEntry *selected = getSelectedSceneObject();
        if (!selected) {
            ImGui::TextUnformatted("No GameObject selected");
            ImGui::End();
            return;
        }

        std::array<char, 128> objectNameBuffer{};
        copyStringToBuffer(objectNameBuffer, selected->object.name);

        ImGui::SeparatorText("GameObject");
        if (ImGui::InputText("Name", objectNameBuffer.data(), objectNameBuffer.size())) {
            selected->object.name = objectNameBuffer.data();
        }
        ImGui::Checkbox("Enabled", &selected->object.enabled);

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Position", &selected->object.transform.translation.x, 0.1f);
            ImGui::DragFloat3("Rotation", &selected->object.transform.rotation.x, 0.01f);
            ImGui::DragFloat3("Scale", &selected->object.transform.scale.x, 0.05f, 0.01f, 100.0f);
        }

        ImGui::SeparatorText("Scripts");
        for (int s = 0; s < static_cast<int>(selected->scripts.size()); ++s) {
            ImGui::PushID(s);
            const char *scriptTypeName = selected->scripts[static_cast<size_t>(s)]->getTypeName();
            if (ImGui::CollapsingHeader(scriptTypeName, ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Type: %s", scriptTypeName);
                if (ImGui::Button("Remove Script")) {
                    waitForGpuIdle(context);
                    selected->scripts[static_cast<size_t>(s)]->onDestroy(selected->object, context);
                    selected->scripts.erase(selected->scripts.begin() + s);
                    ImGui::PopID();
                    break;
                }
            }
            ImGui::PopID();
        }

        if (ImGui::BeginCombo("Add Script", "Select...")) {
            for (const auto &scriptType : availableScriptTypes) {
                if (ImGui::Selectable(scriptType.c_str())) {
                    if (attachScriptByName(*selected, scriptType) && sceneStarted) {
                        selected->scripts.back()->onStart(selected->object, context);
                    }
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::Button("Delete GameObject")) {
            deleteSceneObjectAt(selectedObjectIndex, &context);
        }

        ImGui::End();
    }
}
