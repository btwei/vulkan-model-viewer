// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#ifndef VKMV_APP_HPP
#define VKMV_APP_HPP

#include "vkmv/app/Window.hpp"
#include "vkmv/engine/Engine.hpp"
#include "vkmv/renderer/Renderer.hpp"

namespace vkmv {

/**
 * @class App
 * @brief This class is the top level application for vkmv.
 * 
 * To run an instance of the model viewer, initialize 'App app', then call 'app.run()'.
 */
class App {
public:
    App(int argc, char* argv[]);
    ~App();

    /**
     * @brief Runs the application.
     */
    void run();

private:

};

} // namespace vkmv

#endif // VKMV_APP_HPP