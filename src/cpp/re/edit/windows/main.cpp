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

//// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
//// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
//// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
//#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
//#pragma comment(lib, "legacy_stdio_definitions")
//#endif

static void glfw_error_callback(int error, const char *description)
{
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static void printInfo(GLFWwindow *iWindow)
{
//  auto glfwMonitor = glfwGetWindowMonitor(iWindow);
//  fprintf(stdout, "monitor = %s", glfwGetMonitorName(glfwMonitor));

  int count;
  GLFWmonitor** monitors = glfwGetMonitors(&count);

  if(monitors)
  {
    for(int i = 0; i < count; i++)
    {
      auto monitor = monitors[i];
      float xscale, yscale;
      glfwGetMonitorContentScale(monitor, &xscale, &yscale);
      fprintf(stdout, "monitor[%d] %s | xscale=%f | yscale=%f\n", i, glfwGetMonitorName(monitor), xscale, yscale);
    }
  }

  auto monitor = MonitorFromWindow(glfwGetWin32Window(iWindow), MONITOR_DEFAULTTONULL);
  if(!monitor)
  {
    fprintf(stderr, "no monitor\n");
    return;
  }

  uint32_t dpi_x, dpi_y;
  GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
  fprintf(stdout, "monitor dpi x=%d y=%d\n", dpi_x, dpi_y);

  int value;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);   //Returns 1 value
  fprintf(stdout, "GL_MAX_TEXTURE_SIZE = %d\n", value);
}

static float getFontDpiScale(GLFWwindow *iWindow)
{
  if(true)
    return 1.0f;

  auto monitor = MonitorFromWindow(glfwGetWin32Window(iWindow), MONITOR_DEFAULTTONULL);
  if(!monitor)
    return 1.0f;
  uint32_t dpi_x, dpi_y;
  GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
  return static_cast<float>(dpi_x) / static_cast<float>(USER_DEFAULT_SCREEN_DPI);
}

int main(int argc, char **argv)
{
//  SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
  //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

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

  int glMaxTextureSize;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glMaxTextureSize);

  if(!application.init(std::make_shared<re::edit::OGL3TextureManager>(glMaxTextureSize)))
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

  application.onNativeWindowPositionChange(windowPosX, windowPosY, 1.0f, getFontDpiScale(window));

  printInfo(window);

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
      application.onNativeWindowPositionChange(windowPosX, windowPosY, 1.0f, getFontDpiScale(window));
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