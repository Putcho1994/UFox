module;

#include <chrono>
#include <memory>
#include <SDL3/SDL_mouse.h>
#include <glm/glm.hpp>

#include <SDL3/SDL_events.h>

#include <vector>

#include "GLFW/glfw3.h"

export module ufox_input;

import ufox_lib;  // For GenerateUniqueID

export  namespace ufox::input {

    void RefreshResources(InputResource& res) {
        res.refresh();
    }

    void UpdateGlobalMousePosition(const windowing::WindowResource& window, InputResource& res) {
        switch (res.mouseMotionState) {
            case MouseMotionState::Moving: {
                res.onMouseMove();
                res.mouseMotionState = MouseMotionState::Stopping;
            }
            case MouseMotionState::Stopping: {
                res.onMouseStop();
                res.mouseDelta = glm::ivec2{0,0};
                res.mouseMotionState = MouseMotionState::Idle;
            }
            case MouseMotionState::Idle: {
#ifdef USE_SDL
                float x,y;
                SDL_GetGlobalMouseState(&x,&y);
                x = x - static_cast<float>(window.position.x);
                y = y - static_cast<float>(window.position.y);
#else
                double x,y;
                glfwGetCursorPos(window.getHandle(), &x, &y);
#endif
                glm::ivec2 pos = {static_cast<int>(x), static_cast<int>(y)};
                res.updateMouseDelta(pos);

                if (res.getMouseDeltaMagnitude() > 0.0f) {
                    res.mousePosition = pos;
                    res.mouseMotionState = MouseMotionState::Moving;
                }
            }
            default: ;
        }
    }

    void UpdateMouseButtonAction(InputResource& res) {
        if (res.leftMouseButtonAction.phase != ActionPhase::eSleep) {
            res.onLeftMouseButton();
        }

        if (res.rightMouseButtonAction.phase != ActionPhase::eSleep) {
            res.onRightMouseButton();
        }

        if (res.middleMouseButtonAction.phase != ActionPhase::eSleep) {
            res.onMiddleMouseButton();
        }
    }

    void CatchMouseButton(InputResource& res, MouseButton button, ActionPhase phase, const float& value1, const float& value2) {
        if (button == MouseButton::eLeft) {
            res.leftMouseButtonAction.phase = phase;
            res.leftMouseButtonAction.value1 = value1;
            res.leftMouseButtonAction.value2 = value2;
            res.leftMouseButtonAction.startTime = std::chrono::steady_clock::now();
        }else if (button == MouseButton::eRight) {
            res.rightMouseButtonAction.phase = phase;
            res.rightMouseButtonAction.value1 = value1;
            res.rightMouseButtonAction.value2 = value2;
            res.rightMouseButtonAction.startTime = std::chrono::steady_clock::now();
        }else if (button == MouseButton::eMiddle) {
            res.middleMouseButtonAction.phase = phase;
            res.middleMouseButtonAction.value1 = value1;
            res.middleMouseButtonAction.value2 = value2;
            res.middleMouseButtonAction.startTime = std::chrono::steady_clock::now();
        }
    }

    }




