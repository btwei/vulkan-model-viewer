// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#include <SDL3/SDL_events.h>

#include "vkmv/app/App.hpp"

namespace vkmv
{

App::App(int argc, char* argv[]) {

}

App::~App() {

}

void App::run() {
    RenderableState state;

    Window w;
    Renderer renderer;
    Engine engine;

    while(!w.shouldClose()) {
        SDL_Event e;
        while(SDL_PollEvent(&e) != false) {
            w.handleEvent(e);
            engine.handleEvent(e);
            renderer.handleEvent(e);
        }

        engine.update(state);
        renderer.drawFrame(state);
    }
}

} // namespace vkmv
