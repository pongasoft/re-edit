// Dear ImGui: standalone example application for GLFW + Metal, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_metal.h>
#include <cstdio>
#include "../Application.h"
#include "../MTLTextureManager.h"


#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#import <Metal/Metal.hpp>

#import <Metal/MTLPixelFormat.hpp>

#define CA_PRIVATE_IMPLEMENTATION

#include "QuartzCore/QuartzCore.hpp"

static void glfw_error_callback(int error, const char *description)
{
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int argc, char **argv)
{
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void) io;
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

  // Setup style
  ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  // Load Fonts
  // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
  // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
  // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
  // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
  // - Read 'docs/FONTS.txt' for more instructions and details.
  // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
  //io.Fonts->AddFontDefault();
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
  //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
  //IM_ASSERT(font != NULL);

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
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
  GLFWwindow *window = glfwCreateWindow(application.getNativeWindowWidth(),
                                        application.getNativeWindowHeight(),
                                        "re-edit",
                                        nullptr,
                                        nullptr);
  if(window == nullptr)
    return 1;

  auto device = MTL::CreateSystemDefaultDevice();
  auto commandQueue = device->newCommandQueue();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplMetal_Init(device);

  auto nswin = glfwGetCocoaWindow(window);
  auto layer = ImGui_ImplMetal_Layer();
  ImGui_ImplMetal_Layer_SetDevice(layer, device);
  ImGui_ImplMetal_Layer_SetPixelFormat(layer, MTL::PixelFormatBGRA8Unorm);
  ImGui_ImplMetal_NSWindow_SetLayer(nswin, layer);

  auto renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();

  if(!application.init(std::make_shared<re::edit::MTLTextureManager>(device, ImGui_ImplMetal_GetBackingScaleFactorMainScreen())))
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
    NS::AutoreleasePool *pPool = NS::AutoreleasePool::alloc()->init();

    {
      // Poll and handle events (inputs, window resize, etc.)
      // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
      // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
      // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
      // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
      glfwPollEvents();

      int newWindowPosX;
      int newWindowPosY;
      glfwGetWindowPos(window, &newWindowPosX, &newWindowPosX);
      if(newWindowPosX != windowPosX || newWindowPosY != windowPosY)
      {
        windowPosX = newWindowPosX;
        windowPosY = newWindowPosY;
        application.onNativeWindowPositionChange(windowPosX, windowPosY, ImGui_ImplMetal_GetBackingScaleFactorMainScreen());
      }

      int width, height;
      glfwGetFramebufferSize(window, &width, &height);
      ImGui_ImplMetal_Layer_SetDrawableSize(layer, width, height);
      auto drawable = ImGui_ImplMetal_Layer_GetNextDrawable(layer);

      auto commandBuffer = commandQueue->commandBuffer();
      auto ca0 = renderPassDescriptor->colorAttachments()->object(0);
      ca0->setClearColor(MTL::ClearColor::Make(application.clear_color[0] * application.clear_color[3],
                                               application.clear_color[1] * application.clear_color[3],
                                               application.clear_color[2] * application.clear_color[3],
                                               application.clear_color[3]));
      ca0->setTexture(drawable->texture());
      ca0->setLoadAction(MTL::LoadActionClear);
      ca0->setStoreAction(MTL::StoreActionStore);
      auto renderEncoder = commandBuffer->renderCommandEncoder(renderPassDescriptor);
      renderEncoder->pushDebugGroup(NS::String::string("re-edit", NS::ASCIIStringEncoding));

      // Before New Frame
      application.newFrame();

      // Start the Dear ImGui frame
      ImGui_ImplMetal_NewFrame(renderPassDescriptor);
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      // Main rendering
      application.render();

      // Rendering
      ImGui::Render();
      ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, renderEncoder);

      renderEncoder->popDebugGroup();
      renderEncoder->endEncoding();

      commandBuffer->presentDrawable(drawable);
      commandBuffer->commit();

      int w, h;
      glfwGetWindowSize(window, &w, &h);
      application.setNativeWindowSize(w, h);
    }
    pPool->release();
  }

  // Cleanup
  ImGui_ImplMetal_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
