cmake_minimum_required(VERSION 3.31)
project(FeooGameEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GOLA_ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(GOLA_ENABLE_LTO "Enable interprocedural optimization in release-like builds" ON)
option(GOLA_ENABLE_SANITIZERS "Enable Address/Undefined sanitizers in Debug-like builds" OFF)

add_executable(FeooGameEngine
                main.cpp
                Engine/Core/feoo_device.cpp
                Engine/Core/feoo_model.cpp
                Engine/Core/feoo_pipeline.cpp
                Engine/Core/feoo_swap_chain.cpp
                Engine/Window/feoo_window.cpp
                Engine/feoo_app.cpp
                Engine/Core/feoo_renderer.cpp
                Engine/UI/feoo_imgui.cpp
                Engine/Core/feoo_game_object.hpp
                Engine/Core/render_system.cpp
                Engine/Core/feoo_camera.cpp
                Engine/Core/keyboard_movement_controller.cpp
                Engine/Scripting/spin_models_script.cpp
                Engine/Scripting/movement_script.cpp
                Engine/Scripting/camera_script.cpp
                Engine/Scene/scene_manager.cpp
                Engine/Core/scene_viewport_target.cpp
)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(unofficial-openfbx CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

function(feoo_link_first_available_target consumer)
        foreach(dep IN LISTS ARGN)
                if (TARGET ${dep})
                        target_link_libraries(${consumer} PRIVATE ${dep})
                        return()
                endif ()
        endforeach()

        message(FATAL_ERROR "Could not find any of these targets: ${ARGN}")
endfunction()

target_link_libraries(FeooGameEngine
                PRIVATE
                Vulkan::Vulkan
                glm::glm
                tinyobjloader::tinyobjloader
                vk-bootstrap::vk-bootstrap
)

target_include_directories(FeooGameEngine PRIVATE ${Stb_INCLUDE_DIR})

feoo_link_first_available_target(FeooGameEngine glfw glfw3::glfw glfw3)
feoo_link_first_available_target(FeooGameEngine imgui::imgui imgui)
feoo_link_first_available_target(FeooGameEngine unoffical::openfbx::openfbx unofficial::openfbx::openfbx openfbx)

if (GOLA_ENABLE_WARNINGS)
        if (MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
                target_compile_options(FeooGameEngine PRIVATE /W4 /permissive-)
        elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
                target_compile_options(FeooGameEngine PRIVATE -Wall -Wextra -Wpedantic)
        endif ()
endif ()

if (GOLA_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE MATCHES "Debug|RelWithDebInfo")
        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU" AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
                target_compile_options(FeooGameEngine PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
                target_link_options(FeooGameEngine PRIVATE -fsanitize=address,undefined)
        else ()
                message(STATUS "Sanitizers are enabled but skipped for this compiler frontend")
        endif ()
endif ()

if (GOLA_ENABLE_LTO)
        include(CheckIPOSupported)
        check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
        if (ipo_supported)
                set_property(TARGET FeooGameEngine PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
                set_property(TARGET FeooGameEngine PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
        else ()
                message(WARNING "LTO disabled: ${ipo_error}")
        endif ()
endif ()