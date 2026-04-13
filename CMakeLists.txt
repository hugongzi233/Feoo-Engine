cmake_minimum_required(VERSION 3.31)
project(GolaGameEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GOLA_ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(GOLA_ENABLE_LTO "Enable interprocedural optimization in release-like builds" ON)
option(GOLA_ENABLE_SANITIZERS "Enable Address/Undefined sanitizers in Debug-like builds" OFF)

add_executable(GolaGameEngine
                main.cpp
                Engine/Core/gola_device.cpp
                Engine/Core/gola_model.cpp
                Engine/Core/gola_pipeline.cpp
                Engine/Core/gola_swap_chain.cpp
                Engine/Window/gola_window.cpp
                Engine/gola_app.cpp
                Engine/Core/gola_renderer.cpp
                Engine/UI/gola_imgui.cpp
                Engine/Core/gola_game_object.hpp
                Engine/Core/render_system.cpp
                Engine/Core/gola_camera.cpp
                Engine/Core/keyboard_movement_controller.cpp
)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

function(gola_link_first_available_target consumer)
        foreach(dep IN LISTS ARGN)
                if (TARGET ${dep})
                        target_link_libraries(${consumer} PRIVATE ${dep})
                        return()
                endif ()
        endforeach()

        message(FATAL_ERROR "Could not find any of these targets: ${ARGN}")
endfunction()

target_link_libraries(GolaGameEngine
                PRIVATE
                Vulkan::Vulkan
                glm::glm
                vk-bootstrap::vk-bootstrap
)

gola_link_first_available_target(GolaGameEngine glfw glfw3::glfw glfw3)
gola_link_first_available_target(GolaGameEngine imgui::imgui imgui)

if (GOLA_ENABLE_WARNINGS)
        if (MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
                target_compile_options(GolaGameEngine PRIVATE /W4 /permissive-)
        elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
                target_compile_options(GolaGameEngine PRIVATE -Wall -Wextra -Wpedantic)
        endif ()
endif ()

if (GOLA_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE MATCHES "Debug|RelWithDebInfo")
        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU" AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
                target_compile_options(GolaGameEngine PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
                target_link_options(GolaGameEngine PRIVATE -fsanitize=address,undefined)
        else ()
                message(STATUS "Sanitizers are enabled but skipped for this compiler frontend")
        endif ()
endif ()

if (GOLA_ENABLE_LTO)
        include(CheckIPOSupported)
        check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
        if (ipo_supported)
                set_property(TARGET GolaGameEngine PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
                set_property(TARGET GolaGameEngine PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
        else ()
                message(WARNING "LTO disabled: ${ipo_error}")
        endif ()
endif ()