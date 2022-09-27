// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "../Application.h"
#include "OGL3TextureManager.h"
#include <stdio.h>
#include <shellscalingapi.h>
#include <winuser.h>

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif

#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_NATIVE_INCLUDE_NONE
#include <GLFW/glfw3native.h>

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char *description)
{
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static float getScaleFactor(GLFWwindow *iWindow)
{
  auto monitor = MonitorFromWindow(glfwGetWin32Window(iWindow), MONITOR_DEFAULTTOPRIMARY);
  if(!monitor)
    return 1.0f;
  DEVICE_SCALE_FACTOR scaleFactor;
  if(GetScaleFactorForMonitor(monitor, &scaleFactor) != S_OK || scaleFactor == DEVICE_SCALE_FACTOR::DEVICE_SCALE_FACTOR_INVALID)
    return 1.0f;
  return static_cast<float>(scaleFactor) / 100.0f;
}

int main(int argc, char **argv)
{
  // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
  // GL ES 2.0 + GLSL 100
  const char* glsl_version = "#version 100";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
  // GL 3.2 + GLSL 150
  const char* glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
  // GL 3.0 + GLSL 130
  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
  //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void) io;
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  re::edit::Application application{};

  std::vector<std::string> args{};
  for(int i = 1; i < argc; i++)
    args.emplace_back(argv[i]);

  if(!application.parseArgs(std::move(args)))
    return 1;

  // Setup window
  glfwSetErrorCallback(glfw_error_callback);
  if(!glfwInit())
    return 1;

  // Create window with graphics context
  GLFWwindow *window = glfwCreateWindow(application.getNativeWindowWidth(),
                                        application.getNativeWindowHeight(),
                                        "re-edit",
                                        nullptr,
                                        nullptr);
  if(window == nullptr)
    return 1;

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  if(!application.init(std::make_shared<re::edit::OGL3TextureManager>(getScaleFactor(window))))
    return 1;

  // sets the initial size
  {
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    application.setNativeWindowSize(w, h);
  }

  int windowPosX;
  int windowPosY;
  glfwGetWindowPos(window, &windowPosX, &windowPosY);

  // Main loop
  while(!glfwWindowShouldClose(window))
  {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();

    int newWindowPosX;
    int newWindowPosY;
    glfwGetWindowPos(window, &newWindowPosX, &newWindowPosY);
    if(newWindowPosX != windowPosX || newWindowPosY != windowPosY)
    {
      windowPosX = newWindowPosX;
      windowPosY = newWindowPosY;
      application.onNativeWindowPositionChange(windowPosX, windowPosY, getScaleFactor(window));
    }

    // Before New Frame
    application.newFrame();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Main rendering
    application.render();

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(application.clear_color[0] * application.clear_color[3],
                 application.clear_color[1] * application.clear_color[3],
                 application.clear_color[2] * application.clear_color[3],
                 application.clear_color[3]);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    int w, h;
    glfwGetWindowSize(window, &w, &h);
    application.setNativeWindowSize(w, h);

    glfwSwapBuffers(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
