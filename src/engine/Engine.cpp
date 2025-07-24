// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#include "vkmv/engine/Engine.hpp"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

namespace vkmv {

Engine::Engine(const Renderer& renderer)
: renderer(renderer) {

}

Engine::~Engine() {

}

void Engine::handleEvent(SDL_Event e) {
    ImGui_ImplSDL3_ProcessEvent(&e);
}

void Engine::update(RenderableState& r) {
    newUIFrame();

    buildUI();
}

/**
 * @brief Must call this to refresh the UI state
 */
void Engine::newUIFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void Engine::buildUI() {
    ImVec2 viewport = ImGui::GetMainViewport()->WorkPos;
    ImVec2 viewportSize = ImGui::GetMainViewport()->WorkSize;

    ImGui::BeginMainMenuBar();
        if (ImGui::BeginMenu("File")) {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            ImGui::EndMenu();
        }

    ImGui::EndMainMenuBar();

/*
    ImGui::SetNextWindowPos();
    ImGui::SetNextWindowSize(ImVec2(panelWidth, viewportSize.y));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoTitleBar;

    ImGui::Begin("Right Panel", nullptr, flags);
*/

    ImGui::End();

}

} // namespace vkmv