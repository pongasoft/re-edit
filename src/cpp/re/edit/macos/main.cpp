// Dear ImGui: standalone example application for GLFW + Metal, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_metal.h>
#include <backends/regui_impl_metal.h>
#include <cstdio>
#include "../Application.h"
#include "MTLManagers.h"
#include "nfd.h"
#include <version.h>


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

//! getFontDpiScale
static float getFontDpiScale(GLFWwindow *iWindow)
{
  float dpiScale{1.0f};

  if(iWindow)
    glfwGetWindowContentScale(iWindow, &dpiScale, nullptr);
  else
  {
    auto monitor = glfwGetPrimaryMonitor();
    if(monitor)
    {
      glfwGetMonitorContentScale(monitor, &dpiScale, nullptr);
    }
  }
  return dpiScale;
}

//! onWindowSizeChange
static void onWindowSizeChange(GLFWwindow* iWindow, int iWidth, int iHeight)
{
  auto application = reinterpret_cast<re::edit::Application *>(glfwGetWindowUserPointer(iWindow));
  application->setNativeWindowSize(iWidth, iHeight);
}

//! onWindowContentScaleChange
static void onWindowContentScaleChange(GLFWwindow* iWindow, float iXscale, float iYscale)
{
  auto application = reinterpret_cast<re::edit::Application *>(glfwGetWindowUserPointer(iWindow));
  application->onNativeWindowFontScaleChange(iXscale);
}

//! onWindowClose
static void onWindowClose(GLFWwindow* iWindow)
{
  auto application = reinterpret_cast<re::edit::Application *>(glfwGetWindowUserPointer(iWindow));
  application->maybeExit();
  if(application->running())
    glfwSetWindowShouldClose(iWindow, GLFW_FALSE);
}

class MacOsContext : public re::edit::Application::Context
{
public:
  explicit MacOsContext(GLFWwindow *iWindow, MTL::Device *iDevice) : fWindow{iWindow}, fDevice{iDevice} {}

  std::shared_ptr<re::edit::TextureManager> newTextureManager() const override
  {
    return std::make_shared<re::edit::MTLTextureManager>(fDevice);
  }

  std::shared_ptr<re::edit::NativeFontManager> newNativeFontManager() const override
  {
    return std::make_shared<re::edit::MTLFontManager>(fDevice);
  }

  void setWindowSize(int iWidth, int iHeight) const override
  {
    glfwSetWindowSize(fWindow, iWidth, iHeight);
  }

private:
  GLFWwindow *fWindow;
  MTL::Device *fDevice;
};

int doMain(int argc, char **argv)
{
  fprintf(stdout, "re-edit - %s | %s\n", re::edit::kFullVersion, re::edit::kGitVersion);
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();

  // enable docking
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigDockingWithShift = false;

  // Setup style
  ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  std::vector<std::string> args{};
  for(int i = 1; i < argc; i++)
    args.emplace_back(argv[i]);

  auto config = re::edit::Application::parseArgs(std::move(args));

  // Setup window
  glfwSetErrorCallback(glfw_error_callback);
  if(!glfwInit())
    return 1;

  // Create window with graphics context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
  GLFWwindow *window = glfwCreateWindow(config.getNativeWindowWidth(),
                                        config.getNativeWindowHeight(),
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

  re::edit::Application application{std::make_shared<MacOsContext>(window, device)};

  if(!application.init(config))
  {
    fprintf(stderr, "An error was detected during initialization...");
  }

  if(NFD_Init() != NFD_OKAY)
  {
    fprintf(stderr, "Error while initializing nfd");
    return 1;
  }

  application.onNativeWindowFontScaleChange(getFontDpiScale(window));
  glfwSetWindowUserPointer(window, &application);
  glfwSetWindowContentScaleCallback(window, onWindowContentScaleChange);
  glfwSetWindowSizeCallback(window, onWindowSizeChange);
  glfwSetWindowCloseCallback(window, onWindowClose);

  // Main loop
  while(application.running())
  {
    NS::AutoreleasePool *pPool = NS::AutoreleasePool::alloc()->init();

    {
      // Poll and handle events (inputs, window resize, etc.)
      // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
      // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
      // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
      // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
      glfwPollEvents();

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
      if(application.newFrame())
      {
        // Start the Dear ImGui frame
        ImGui_ImplMetal_NewFrame(renderPassDescriptor);
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Main rendering
        if(application.render())
        {
          // Rendering
          ImGui::Render();
          ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, renderEncoder);
        }
      }

      renderEncoder->popDebugGroup();
      renderEncoder->endEncoding();

      commandBuffer->presentDrawable(drawable);
      commandBuffer->commit();
    }
    pPool->release();
  }

  // Cleanup
  ImGui_ImplMetal_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  NFD_Quit();

  glfwDestroyWindow(window);
  glfwTerminate();

  return application.hasException() ? 1 : 0;
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