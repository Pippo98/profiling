#pragma once

#include "imgui.hpp"

namespace render {
void NewFrame();
void Render(GLFWwindow *window, ImVec4 clearColor);
} // namespace render
