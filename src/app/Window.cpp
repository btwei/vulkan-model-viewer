// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#include "vkmv/app/Window.hpp"

namespace vkmv {

Window::Window() {
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow("Vulkan Model Viewer", 640, 480, SDL_WINDOW_VULKAN);

    if(!window) throw std::runtime_error("Failed to create SDL3 window!"); 
}

Window::~Window() {
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Window::handleEvent(SDL_Event event) {
    if(event.type == SDL_EVENT_QUIT) quit = true;
    if(event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)) quit = true;
}

void Window::initWindow() {

}

} // namespace vkmv