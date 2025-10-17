//
// Created by Puwiwad on 05.10.2025.
//
module;
#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <GLFW/glfw3.h>
#include <string>
#include <stdexcept>
#include <memory>
#include <optional>
#include <vulkan/vulkan_raii.hpp>

export module ufox_windowing;

import glm;
import ufox_lib;



export namespace ufox::windowing::sdl {

    // void UpdatePanel(WindowResource& resource) {
    //     SDL_GetWindowSize(resource.window.get(), &resource.panel.width, &resource.panel.height);
    //     resource.panel.print();
    // }
    //
    // void Show(const WindowResource& resource) {
    //     SDL_ShowWindow(resource.window.get());
    // }
    //
    // void Hide(const WindowResource& resource) {
    //     SDL_HideWindow(resource.window.get());
    // }
    //
    // WindowResource CreateWindowResource(const UFoxWindowCreateInfo& info) {
    //     WindowResource resource{};
    //
    //     if (!SDL_Init(SDL_INIT_VIDEO)) throw SDLException("Failed to initialize SDL");
    //
    //     SDL_DisplayID primary = SDL_GetPrimaryDisplay();
    //     SDL_Rect usableBounds{};
    //     SDL_GetDisplayUsableBounds(primary, &usableBounds);
    //
    //     // Create window while ensuring proper cleanup on failure
    //     auto flags = static_cast<Uint32>(info.flags);
    //     SDL_Window *rawWindow = SDL_CreateWindow(info.title.c_str(),
    //         usableBounds.w - 4, usableBounds.h - 34, flags);
    //
    //     if (!rawWindow) throw SDLException("Failed to create window");
    //
    //     resource.window.reset(rawWindow);
    //
    //     SDL_SetWindowPosition(resource.window.get(), 2, 32);
    //
    //     UpdatePanel(resource);
    //
    //     if (flags & SDL_WINDOW_VULKAN && !SDL_Vulkan_LoadLibrary(nullptr)) throw SDLException("Failed to load Vulkan library");
    //     return resource;
    // }

}
