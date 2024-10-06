#include "app.hpp"
#include "render.hpp"
#include "window.hpp"

void App::SetTitle(const std::string &_title) { title = _title; }
bool App::Init() {
  window = window::OpenWindow();
  if (window == nullptr) {
    return false;
  }
  window::InitImgui(window);
  return true;
}
void App::Run() {
  while (!glfwWindowShouldClose(window)) {
    render::NewFrame();
    Draw();
    Render();
  }
}
void App::Shutdown() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
}
void App::Render() {
  render::Render(window, ImVec4(0.06f, 0.06f, 0.06f, 0.94f));
}
void App::Draw() {
  ImGui::Begin("Hello, world!");
  ImGui::Text("This is some useful text.");
  ImGui::End();
}

namespace render {

void NewFrame() {
  glfwPollEvents();
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  window::Dockspace(ImGui::GetMainViewport()->WorkSize.x,
                    ImGui::GetMainViewport()->WorkSize.y);
}

void Render(GLFWwindow *window, ImVec4 clearColor) {
  int display_w, display_h;

  ImGui::Render();
  glfwGetFramebufferSize(window, &display_w, &display_h);

  glViewport(0, 0, display_w, display_h);
  glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w,
               clearColor.z * clearColor.w, clearColor.w);
  glClear(GL_COLOR_BUFFER_BIT);

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }
  glfwMakeContextCurrent(window);

  glfwSwapBuffers(window);
}

} // namespace render

namespace window {

void GlwfErrorCallback(int error, const char *description) {
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

GLFWwindow *OpenWindow() {
  // Setup window
  glfwSetErrorCallback(GlwfErrorCallback);
  glewExperimental = GL_TRUE;
  if (!glfwInit()) {
    return nullptr;
  }

  GLFWmonitor *monitor = glfwGetPrimaryMonitor();

  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
#ifndef __APPLE__
  glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
#endif

  int x, y, w, h;
  glfwGetMonitorWorkarea(monitor, &x, &y, &w, &h);

  glfwWindowHint(GLFW_MAXIMIZED, GL_TRUE);
  GLFWwindow *window = glfwCreateWindow(w, h, "example_1", nullptr, nullptr);

  if (window == nullptr) {
    return nullptr;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync
  glEnable(GL_MULTISAMPLE);

  if (glewInit() != GLEW_OK) {
    return nullptr;
  }

  glfwShowWindow(window);

  return window;
}

void InitImgui(GLFWwindow *window) {
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGuiContext *ctx = ImGui::CreateContext();
  ImGui::SetCurrentContext(ctx);

  ImGuiStyle &style = ImGui::GetStyle();
  ImPlot::CreateContext();

  float scaling_x, scaling_y, app_scaling;
  glfwGetWindowContentScale(window, &scaling_x, &scaling_y);

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init();
}

void Dockspace(float width, float height) {
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
  ImGui::SetNextWindowSize(ImVec2(width, height));

  ImGuiWindowFlags host_window_flags = 0;
  host_window_flags |= ImGuiWindowFlags_NoTitleBar |
                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
  host_window_flags |=
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  ImGuiDockNodeFlags dockspace_flags =
      ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("MainDockspace", nullptr, host_window_flags);
  ImGui::PopStyleVar(3);

  ImGui::SetCursorPos({0, 0});

  ImGuiID dockspace_id = ImGui::GetID("DockSpace");
  ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags, nullptr);
  ImGui::End();
}

} // namespace window
