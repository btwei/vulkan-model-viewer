// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#ifndef VKMV_ENGINE_HPP
#define VKMV_ENGINE_HPP

#include <SDL3/SDL_events.h>

#include "vkmv/renderer/Renderer.hpp"

namespace vkmv {

/**
 * @class Engine
 * @brief Handles state updates. Pushes RenderableStates to the Renderer.
 * 
 * Construct this with a renderer reference. This class pulls inputs from the Renderer's
 * UI class, handles events from the window, and per frame generates a state to draw.
 */
class Engine {
public:
    Engine(const Renderer& r);
    ~Engine();

    void handleEvent(SDL_Event e);
    void update(RenderableState& r);

private:
    const Renderer& renderer;

    bool panel_open = true;
    float panel_width = 300.0f;
    float collapsed_width = 30.0f;
    float anim_progress = 1.0f;

    void newUIFrame();
    void buildUI();

};

} // namespace vkmv

#endif // VKMV_ENGINE_HPP