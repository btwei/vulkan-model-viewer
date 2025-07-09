// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#include <cstdlib>
#include <exception>
#include <iostream>

#include "vkmv/app/App.hpp"

int main(int argc, char* argv[]) {
    try {
        vkmv::App app(argc, argv);
        app.run();
    } catch(const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}