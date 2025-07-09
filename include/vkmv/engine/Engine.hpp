// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#ifndef VKMV_ENGINE_HPP
#define VKMV_ENGINE_HPP

#include <SDL3/SDL_events.h>

namespace vkmv {

class Engine {
public:
    Engine();
    ~Engine();

    void handleEvent(SDL_Event e);
    void update();

private:

};

} // namespace vkmv

#endif // VKMV_ENGINE_HPP