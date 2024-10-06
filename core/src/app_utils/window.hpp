#pragma once

#include <GLFW/glfw3.h>

namespace window {

void GlwfErrorCallback(int error, const char *description);
GLFWwindow *OpenWindow();
void InitImgui(GLFWwindow *window);
void Dockspace(float width, float height);

} // namespace window
