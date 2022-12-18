// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "OGL3Managers.h"
#include <stdio.h>
#include <shellscalingapi.h>
#include <winuser.h>
#include "nfd.h"
#include <version.h>
#include "LocalSettingsManager.h"
#include "../GLFWContext.h"

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

class WindowsContext : public re::edit::platform::GLFWContext
{
public:
  explicit WindowsContext(std::shared_ptr<re::edit::NativePreferencesManager> iPreferencesManager,
                          GLFWwindow *iWindow,
                          int iGLMaxTextureSize) :
    GLFWContext(std::move(iPreferencesManager), iWindow),
    fGLMaxTextureSize{iGLMaxTextureSize}
  {
    // empty
  }

  std::shared_ptr<re::edit::TextureManager> newTextureManager() const override
  {
    return std::make_shared<re::edit::OGL3TextureManager>(fGLMaxTextureSize);
  }

  std::shared_ptr<re::edit::NativeFontManager> newNativeFontManager() const override
  {
    return std::make_shared<re::edit::OGL3FontManager>();
  }

  float getScale() const override
  {
    return getFontDpiScale();
  }

private:
  int fGLMaxTextureSize;
};

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

int doMain(int argc, char **argv)
{
  fprintf(stdout, "re-edit - %s | %s\n", re::edit::kFullVersion, re::edit::kGitVersion);
  SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
  //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();

  // enable docking
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigDockingWithShift = false;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  auto preferencesManager = std::make_shared<re::edit::LocalSettingsManager>();

  std::vector<std::string> args{};
  for(int i = 1; i < argc; i++)
    args.emplace_back(argv[i]);

  auto config = re::edit::Application::parseArgs(preferencesManager.get(), std::move(args));

  // init glfw
  if(!re::edit::platform::GLFWContext::initGLFW())
    return 1;

  auto scale = re::edit::platform::GLFWContext::getFontDpiScale(nullptr); // primary monitor

  // Create window with graphics context
  GLFWwindow *window = glfwCreateWindow(re::edit::config::kWelcomeWindowWidth * scale,
                                        re::edit::config::kWelcomeWindowHeight * scale,
                                        re::edit::config::kWelcomeWindowTitle,
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

  printInfo(window);

  auto ctx = std::make_shared<WindowsContext>(preferencesManager, window, glMaxTextureSize);

  re::edit::Application application{ctx, config};

  if(NFD_Init() != NFD_OKAY)
  {
    fprintf(stderr, "Error while initializing nfd");
    return 1;
  }

  application.onNativeWindowFontDpiScaleChange(ctx->getFontDpiScale());
  ctx->setupCallbacks(&application);
  ctx->centerWindow();

  {
    auto logo = application.getLogo();
    GLFWimage image{static_cast<int>(logo->frameWidth()),
                    static_cast<int>(logo->frameHeight()),
                    const_cast<unsigned char*>(logo->getFilmStrip()->data()) };

    glfwSetWindowIcon(window, 1, &image);
  }

  // Main loop
  while(application.running())
  {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();

    // Before New Frame
    if(application.newFrame())
    {
      // Start the Dear ImGui frame
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      int display_w, display_h;
      glfwGetFramebufferSize(window, &display_w, &display_h);
      glViewport(0, 0, display_w, display_h);
      glClearColor(application.clear_color[0] * application.clear_color[3],
                   application.clear_color[1] * application.clear_color[3],
                   application.clear_color[2] * application.clear_color[3],
                   application.clear_color[3]);
      glClear(GL_COLOR_BUFFER_BIT);

      // Main rendering
      if(application.render())
      {
        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      }
    }

    glfwSwapBuffers(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  NFD_Quit();

  glfwDestroyWindow(window);
  glfwTerminate();

  auto const res = application.hasException() ? 1 : 0;

  if(!application.shutdown(250))
    std::_Exit(application.hasException() ? EXIT_FAILURE : EXIT_SUCCESS);

  return res;
}

int main(int argc, char **argv)
{
  try
  {
    return doMain(argc, argv);
  }
  catch(...)
  {
    RE_EDIT_LOG_ERROR("Unrecoverable error detected... aborting: %s", re::edit::Application::what(std::current_exception()));
    return 1;
  }
}

INT WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,
                   PSTR lpCmdLine, INT nCmdShow)
{
  std::string mainExe("re-edit.exe");

  if(lpCmdLine && !std::string(lpCmdLine).empty())
  {
    std::string args(lpCmdLine);
    char *argv[2];
    argv[0] = mainExe.data();
    argv[1] = args.data();
    return main(2, argv);
  }
  else
  {
    char *argv[1];
    argv[0] = mainExe.data();
    return main(1, argv);
  }

}
