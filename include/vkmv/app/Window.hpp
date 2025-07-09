// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#ifndef VKMV_WINDOW_HPP
#define VKMV_WINDOW_HPP

#include <stdexcept>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_main.h>
#include <vulkan/vulkan.h>

namespace vkmv {

/**
 * @class Window
 * @brief Creates a window across a variety of platforms via SDL3.
 */
class Window {
public:
    Window();
    ~Window();

    bool shouldClose() const { return quit; }

    void handleEvent(SDL_Event event);

private:
    SDL_Window* window;
    bool quit = false;

    void initWindow();
};

} // namespace vkmv

#endif // VKMV_WINDOW_HPP