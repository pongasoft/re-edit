/*
 * Copyright (c) 2023 pongasoft
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * @author Yan Pujante
 */

#include <raylib.h>
#include <rlImGui.h>
#include <cstdio>
#include <cstdlib>
#include "NativeApplication.h"
#include "nfd.h"
#include "../UIContext.h"
#include "GLFW/glfw3.h"
#include <version.h>

using namespace re::edit::platform;

struct RLWindow
{
  RLWindow(int width, int height, const char *title)
  {
    SetWindowState(FLAG_WINDOW_HIGHDPI);
    InitWindow(width, height, title);
    SetTraceLogLevel(LOG_WARNING);
    SetWindowState(FLAG_WINDOW_RESIZABLE);
  }

  ~RLWindow() { CloseWindow(); }
};

int doMain(int argc, char **argv)
{
  fprintf(stdout, "RE Edit - %s | %s\n", re::edit::kFullVersion, re::edit::kGitVersion);

  auto nativeApplication = NativeApplication::create();

  RLWindow window{re::edit::config::kWelcomeWindowWidth,
                  re::edit::config::kWelcomeWindowHeight,
                  re::edit::config::kWelcomeWindowTitle};

  SetExitKey(-1); // prevent ESC to close the application

  rlImGuiSetup(true); // true is for Dark Style

  GLint maxTextureSize = 0;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

  re::edit::UIContext uiContext{maxTextureSize};
  uiContext.init();
  re::edit::UIContext::kCurrent = &uiContext;

  ImGuiIO &io = ImGui::GetIO();

  // enable docking
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigDockingWithShift = false;


  std::vector<std::string> args{};
  for(int i = 1; i < argc; i++)
    args.emplace_back(argv[i]);

  std::shared_ptr<RLContext> ctx = nativeApplication->newRLContext();

  auto config = re::edit::Application::parseArgs(ctx->getPreferencesManager().get(), std::move(args));

  if(!nativeApplication->isSingleInstance())
  {
    fprintf(stdout, "Detected multiple instances running. No preferences will be saved to avoid conflict.\n");
    config.fGlobalConfig.fSaveEnabled = false;
  }

  if(!nativeApplication->registerInstance())
    return 1;

  re::edit::Application application{ctx, config};

  if(NFD_Init() != NFD_OKAY)
  {
    fprintf(stderr, "Error while initializing nfd");
    return 1;
  }

  ctx->setup(&application);

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

    // Before New Frame
    if(application.newFrame(uiContext.collectUIActions()))
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

  auto const res = application.hasException() ? 1 : 0;

  if(!application.shutdown(250))
    std::_Exit(application.hasException() ? EXIT_FAILURE : EXIT_SUCCESS);

  return res;
}
