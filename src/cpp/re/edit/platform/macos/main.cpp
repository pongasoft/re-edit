// Dear ImGui: standalone example application for GLFW + Metal, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include <raylib.h>
#include <rlImGui.h>
#include <cstdio>
#include <cstdlib>
#include "../RLContext.h"
#include "NSUserDefaultsManager.h"
#include "MacOSNetworkManager.h"
#include "MacOSMultipleInstanceManager.h"
#include "nfd.h"
#include "../../UIContext.h"
#include <version.h>

class MacOsContext : public re::edit::platform::RLContext
{
public:
  explicit MacOsContext(std::shared_ptr<re::edit::NativePreferencesManager> iPreferencesManager) :
    RLContext(std::move(iPreferencesManager))
    {
      // empty
    }

  std::shared_ptr<re::edit::NetworkManager> newNetworkManager() const override
  {
    return std::make_shared<re::edit::MacOSNetworkManager>();
  }

  float getScale() const override
  {
    return 1.0;
  }

  void openURL(std::string const &iURL) const override
  {
    std::system(re::mock::fmt::printf("open \"%s\"", iURL).c_str());
  }
};

int doMain(int argc, char **argv)
{
  fprintf(stdout, "RE Edit - %s | %s\n", re::edit::kFullVersion, re::edit::kGitVersion);

  re::edit::UIContext uiContext{};
  re::edit::UIContext::kCurrent = &uiContext;

//  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//  glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

  SetWindowState(FLAG_WINDOW_HIGHDPI);

  InitWindow(re::edit::config::kWelcomeWindowWidth,
             re::edit::config::kWelcomeWindowHeight,
             re::edit::config::kWelcomeWindowTitle);

  SetWindowState(FLAG_WINDOW_RESIZABLE);

  rlImGuiSetup(true); // true is for Dark Style

  ImGuiIO &io = ImGui::GetIO();

  // enable docking
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigDockingWithShift = false;

  auto preferencesManager = std::make_shared<re::edit::NSUserDefaultsManager>();

  std::vector<std::string> args{};
  for(int i = 1; i < argc; i++)
    args.emplace_back(argv[i]);

  auto config = re::edit::Application::parseArgs(preferencesManager.get(), std::move(args));

  if(!re::edit::MacOSMultipleInstanceManager::isSingleInstance())
  {
    fprintf(stdout, "Detected multiple instances running. No preferences will be saved to avoid conflict.\n");
    config.fGlobalConfig.fSaveEnabled = false;
  }

  auto ctx = std::make_shared<MacOsContext>(preferencesManager);

  re::edit::Application application{ctx, config};

  if(NFD_Init() != NFD_OKAY)
  {
    fprintf(stderr, "Error while initializing nfd");
    return 1;
  }

  application.onNativeWindowFontScaleChange(ctx->getFontDpiScale());
  ctx->setupCallbacks(&application);
  ctx->centerWindow();

//  SetTargetFPS(60);

  // Main loop
  while(application.running())
  {
    // Handle close event
    if(WindowShouldClose())
      application.maybeExit();

    BeginDrawing();

    //       ca0->setClearColor(MTL::ClearColor::Make(application.clear_color[0] * application.clear_color[3],
    //                                               application.clear_color[1] * application.clear_color[3],
    //                                               application.clear_color[2] * application.clear_color[3],
    //                                               application.clear_color[3]));
//    float clear_color[4] = {0.55f, 0.55f, 0.55f, 1.00f};
    ClearBackground(Color{127, 127, 127, 255});

    // Execute all actions requiring the ui thread
    uiContext.processUIActions();

    // Before New Frame
    if(application.newFrame())
    {
      rlImGuiBegin();
      // Main rendering
      if(application.render())
      {
        // Rendering
        rlImGuiEnd();
      }
    }

    EndDrawing();
  }

  // Cleanup
  rlImGuiShutdown();

  NFD_Quit();

  CloseWindow();

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