//
// Created by b-boy on 05.10.2025.
//
module;
#include <iostream>
#include <optional>
#include <SDL3/SDL.h>

export module ufox_engine;

import ufox_lib;
import ufox_windowing;

export namespace ufox {

    class IDE {
    public:
        IDE() = default;
        ~IDE() = default;

        void Init() {
            using windowing::sdl::WindowFlag;
            _window.emplace("UFox", WindowFlag::Vulkan | WindowFlag::Resizable);

        }

        int Run() {
            try {

                bool isRunning = true;
                SDL_Event event;

                while (isRunning) {
                    while (SDL_PollEvent(&event)) {
                        switch (event.type) {
                            case SDL_EVENT_QUIT: {
                                isRunning = false;
                                break;
                            }
                            case SDL_EVENT_WINDOW_RESIZED:{
                                _window.value().UpdateRootPanel();
                                break;
                            }
                            default: {}
                        }
                    }
                }

                return 0;
            } catch (const windowing::sdl::SDLException& e) {
                std::cerr << e.what() << std::endl;
                return 1;
            }
        }


    private:
        std::optional<windowing::sdl::UFoxWindow> _window{};

    };

}