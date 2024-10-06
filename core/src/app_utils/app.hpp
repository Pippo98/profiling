#pragma once

#include "imgui.hpp"
#include <string>

class App {
public:
  virtual bool Init();
  virtual void Render();
  virtual void Shutdown();

  void SetTitle(const std::string &title);

  bool IsRunning() const;
  void Run();

protected:
  virtual void Draw();

  std::string title;
  GLFWwindow *window;
};
