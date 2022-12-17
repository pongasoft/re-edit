/*
 * Copyright (c) 2022 pongasoft
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

#include "Application.h"
#include "Widget.h"
#include "ReGui.h"
#include "Utils.h"
#include "Errors.h"
#include "lua/ConfigParser.h"
#include "LoggingManager.h"
#include <fstream>
#include <chrono>
#include <iterator>
#include <imgui.h>
#include <imgui_internal.h>
#include <version.h>
#include <nfd.h>

namespace re::edit {

//------------------------------------------------------------------------
// Application::what
//------------------------------------------------------------------------
std::string Application::what(std::exception_ptr const &p)
{
  if(p)
  {
    try
    {
      std::rethrow_exception(p);
    }
    catch(std::exception &e)
    {
      return e.what();
    }
    catch(...)
    {
      return "Unknown exception";
    }
  }

  return "No Error";
}

//------------------------------------------------------------------------
// Application::executeCatchAllExceptions
//------------------------------------------------------------------------
template<typename F>
void Application::executeAndAbortOnException(F&& f) noexcept
{
  try
  {
    f();
  }
  catch(...)
  {
    fprintf(stderr, "ABORT| Unrecoverable exception detected: %s", Application::what(std::current_exception()).c_str());
    abort();
  }
}

//------------------------------------------------------------------------
// Application::executeAndLogOnException
//------------------------------------------------------------------------
template<typename R, typename F>
R Application::executeAndLogOnException(F&& f) noexcept
{
  try
  {
    return f();
  }
  catch(...)
  {
    RE_EDIT_LOG_WARNING("Unexpected exception detected: %s (ignored)", Application::what(std::current_exception()).c_str());
    return {};
  }
}

namespace impl {

//------------------------------------------------------------------------
// impl::inferValidRoot
// Given a path, tries to determine a valid root for a rack extension
//------------------------------------------------------------------------
std::optional<fs::path> inferValidRoot(fs::path const &iPath) noexcept
{
  try
  {
    if(!fs::exists(iPath))
      return std::nullopt;

    auto path = fs::canonical(iPath);

    if(fs::is_directory(path))
    {
      if(fs::exists(path / "info.lua"))
        return path;
    }
    else
    {
      auto filename = path.filename().u8string();

      if(filename == "info.lua")
        return path.parent_path();

      if(filename == "motherboard_def.lua" || filename == "realtime_controller.lua")
        return inferValidRoot(path.parent_path());

      if(filename == "hdgui_2D.lua" || filename == "device_2D.lua")
      {
        auto GUI2D = path.parent_path();
        if(GUI2D.has_parent_path())
          return inferValidRoot(GUI2D.parent_path());
      }
    }
  }
  catch(...)
  {
    RE_EDIT_LOG_WARNING("Exception while loading path %s", iPath.u8string());
  }
  return std::nullopt;
}

}


//------------------------------------------------------------------------
// Application::async
//------------------------------------------------------------------------
template<class Function, class... Args>
void Application::async(Function &&f, Args &&... args)
{
  RE_EDIT_INTERNAL_ASSERT(fGUIThreadID == std::this_thread::get_id(), "Can only be called from the GUI thread!");
  std::future<gui_action_t> action = std::async(std::launch::async,
                                                std::forward<Function>(f),
                                                std::forward<Args>(args)...);
  fAsyncActions.push_back(std::move(action));
}

//------------------------------------------------------------------------
// Application::parseArgs
//------------------------------------------------------------------------
Application::Config Application::parseArgs(NativePreferencesManager const *iPreferencesManager,
                                           std::vector<std::string> iArgs)
{
  Application::Config c{};

  if(iPreferencesManager)
  {
    try
    {
      c.fGlobalConfig = PreferencesManager::load(iPreferencesManager);
    }
    catch(...)
    {
      RE_EDIT_LOG_WARNING("Error while loading preferences %s", what(std::current_exception()));
    }
  }

  if(!iArgs.empty())
  {
    c.fProjectRoot = impl::inferValidRoot(fs::path(iArgs[0]));
  }

  return c;
}


//------------------------------------------------------------------------
// Application::savePreferences
//------------------------------------------------------------------------
void Application::savePreferences(UserError *oErrors) noexcept
{
  try
  {
    auto mgr = fContext->getPreferencesManager();
    if(mgr)
    {
      if(fState == State::kReLoaded)
      {
        RE_EDIT_INTERNAL_ASSERT(fAppContext != nullptr);
        auto deviceConfig = fAppContext->getConfig();
        auto ps = fContext->getWindowPositionAndSize();
        deviceConfig.fNativeWindowPos = ImVec2{ps.x, ps.y};
        deviceConfig.fNativeWindowSize = ImVec2{ps.z, ps.w};
        deviceConfig.fLastAccessTime = config::now();
        fConfig.addDeviceConfigToHistory(deviceConfig);
      }
      PreferencesManager::save(mgr.get(), fConfig);
    }
  }
  catch(...)
  {
    if(oErrors)
      oErrors->add("Error while saving preferences %s", what(std::current_exception()));
    else
      RE_EDIT_LOG_WARNING("Error while saving preferences %s", what(std::current_exception()));
  }
}

//------------------------------------------------------------------------
// Application::Application
//------------------------------------------------------------------------
Application::Application(std::shared_ptr<Context> iContext) :
  fContext{std::move(iContext)},
  fConfig{}
{
  init();
}

//------------------------------------------------------------------------
// Application::Application
//------------------------------------------------------------------------
Application::Application(std::shared_ptr<Context> iContext, Application::Config const &iConfig) :
  fContext{std::move(iContext)},
  fConfig{iConfig.fGlobalConfig}
{
  init();
  if(iConfig.fProjectRoot)
    loadProjectDeferred(*iConfig.fProjectRoot);
}

//------------------------------------------------------------------------
// Application::~Application
//------------------------------------------------------------------------
Application::~Application()
{
  if(fReLoadingFuture)
    fReLoadingFuture->fFuture.get();
}

//------------------------------------------------------------------------
// Application::asyncCheckForUpdates
//------------------------------------------------------------------------
void Application::asyncCheckForUpdates()
{
  async([this]() {
    auto latestRelease = fNetworkManager->getLatestRelease();
    if(latestRelease)
      return gui_action_t([this, latestRelease = *latestRelease]() {
        newDialog("Check for Updates")
          .text(latestRelease.fReleaseNotes)
          .buttonOk();
      });
    else
      return gui_action_t();
  });
}

//------------------------------------------------------------------------
// Application::Application
//------------------------------------------------------------------------
void Application::init()
{
  fGUIThreadID = std::this_thread::get_id();

  fTextureManager = fContext->newTextureManager();
  fTextureManager->init(BuiltIns::kGlobalBuiltIns);
  fFontManager = std::make_shared<FontManager>(fContext->newNativeFontManager());
  fNetworkManager = fContext->newNetworkManager();

  if(!fContext->isHeadless())
  {
    auto &io = ImGui::GetIO();
    io.IniFilename = nullptr; // don't use imgui.ini file
    io.WantSaveIniSettings = false; // will be "notified" when it changes
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    fFontManager->requestNewFont({"JetBrains Mono Regular", BuiltInFont::kJetBrainsMonoRegular, fConfig.fFontSize});
  }
}

//------------------------------------------------------------------------
// Application::init
//------------------------------------------------------------------------
std::shared_ptr<AppContext> Application::initAppContext(fs::path const &iRoot,
                                                        config::Device const &iConfig,
                                                        Utils::CancellableSPtr const &iCancellable)
{
  auto ctx = std::make_shared<AppContext>(iRoot, fContext->newTextureManager());

  Utils::StorageRAII<AppContext> current{&AppContext::kCurrent, ctx.get()};

  iCancellable->progress("Loading...");
  ctx->init(iConfig);
  iCancellable->progress("Loading motherboard...");
  ctx->initDevice();
  iCancellable->progress("Loading GUI...");
  ctx->initGUI2D(iCancellable);
  return ctx;
}


//------------------------------------------------------------------------
// Application::exit
//------------------------------------------------------------------------
void Application::exit()
{
  savePreferences();
  fState = State::kDone;
}

//------------------------------------------------------------------------
// Application::loadProject
//------------------------------------------------------------------------
void Application::loadProject(fs::path const &iRoot)
{
  config::Device c = fConfig.getDeviceConfigFromHistory(iRoot.u8string());

  fAppContext = nullptr;
  fState = State::kReLoading;
  fContext->setWindowTitle(fmt::printf("re-edit - Loading [%s]", iRoot.u8string()));
  fReLoadingFuture = std::make_unique<CancellableFuture<gui_action_t>>();
  fReLoadingFuture->launch([this, iRoot, c, cancellable = fReLoadingFuture->fCancellable] {
    try
    {
      return gui_action_t([this, c, ctx = initAppContext(iRoot, c, cancellable)]() {
        if(fState == State::kReLoading)
        {
            fAppContext = ctx;
            fState = State::kReLoaded;
            if(!fContext->isHeadless())
              ImGui::LoadIniSettingsFromMemory(c.fImGuiIni.c_str(), c.fImGuiIni.size());
            fContext->setWindowPositionAndSize(c.fNativeWindowPos, c.fNativeWindowSize);
            fContext->setWindowTitle(fmt::printf("re-edit - %s", fAppContext->getConfig().fName));
            savePreferences();
          }
      });
    }
    catch(Utils::Cancellable::cancelled_t const &e)
    {
      return gui_action_t([this]() {
        RE_EDIT_LOG_DEBUG("Cancelled");
        if(fState == State::kReLoading)
        {
          fAppContext = nullptr;
          fState = State::kNoReLoaded;
        }
      });
    }
    catch(...)
    {
      return gui_action_t([this, c, iRoot, exception = what(std::current_exception())]() {
        if(fState == State::kReLoading)
        {
          fAppContext = nullptr;
          fState = State::kNoReLoaded;
          newDialog("Error")
            .preContentMessage(fmt::printf("Error while loading Rack Extension project [%s]", iRoot.u8string()))
            .text(exception, true)
            .button("Ok", [this] { fContext->setWindowTitle(config::kWelcomeWindowTitle); });
        }
      });
    }
  });
}

namespace impl {
//------------------------------------------------------------------------
// impl::chain
//------------------------------------------------------------------------
Application::gui_action_t chain(Application::gui_action_t iFirst, Application::gui_action_t iNext)
{
  if(!iNext)
    return iFirst;

  if(!iFirst)
    return iNext;

  return [first = std::move(iFirst), next = std::move(iNext)]() { first(); next(); };
}

//------------------------------------------------------------------------
// impl::maybeInvoke
//------------------------------------------------------------------------
constexpr void maybeInvoke(Application::gui_action_t const &iAction) { if(iAction) iAction(); }

}

//------------------------------------------------------------------------
// Application::maybeSaveProject
//------------------------------------------------------------------------
void Application::maybeSaveProject(gui_action_t const &iNextAction)
{
  auto action = impl::chain([this]() { saveProject(); }, iNextAction);

  if(fAppContext)
  {
    if(!fAppContext->getReEditVersion())
    {
      newDialog(fmt::printf("Saving - %s", fAppContext->getDeviceName()))
        .preContentMessage("Warning")
        .text("This is the first time you save this project.\n"
              "Saving will override hdgui_2d.lua and device_2d.lua.\n"
              "Are you sure you want to proceed?")
        .button("Yes (save)", [action] { action(); })
        .button("No (don't save)", [iNextAction] { impl::maybeInvoke(iNextAction); })
        ;
    }
    else
    {
      action();
    }
  }
  else
  {
    impl::maybeInvoke(iNextAction);
  }
}

//------------------------------------------------------------------------
// Application::saveProject
//------------------------------------------------------------------------
void Application::saveProject()
{
  if(fAppContext)
  {
    Utils::StorageRAII<AppContext> current{&AppContext::kCurrent, fAppContext.get()};
    fAppContext->save();
  }
}

//------------------------------------------------------------------------
// Application::loadProjectDeferred
//------------------------------------------------------------------------
void Application::loadProjectDeferred(fs::path const &iRoot)
{
  deferNextFrame([this, iRoot]() { loadProject(iRoot); });
}

//------------------------------------------------------------------------
// Application::maybeCloseProject
//------------------------------------------------------------------------
void Application::maybeCloseProject(std::optional<std::string> const &iDialogTitle, gui_action_t const &iNextAction)
{
  // nextAction must be deferred because closeProject is deferred
  auto nextAction = [this, iNextAction]() { deferNextFrame(iNextAction); };
  auto action = [this, nextAction]() { deferNextFrame(impl::chain([this]() { closeProject(); }, nextAction)); };

  if(fState == State::kReLoaded)
  {
    if(fAppContext->needsSaving())
    {
      newDialog(iDialogTitle ? *iDialogTitle : fmt::printf("Closing - %s", fAppContext->getDeviceName()))
        .postContentMessage("You have unsaved changes, do you want to save them?")
        .button("Yes", [this, action] { maybeSaveProject(action); })
        .button("No", [action] { impl::maybeInvoke(action); })
        .button("Cancel", {});
    }
    else
      action();
  }
  else
  {
    impl::maybeInvoke(nextAction);
  }
}

//------------------------------------------------------------------------
// Application::closeProject
//------------------------------------------------------------------------
void Application::closeProject()
{
  savePreferences();
  fAppContext = nullptr;
  if(fState == State::kReLoaded)
  {
    fState = State::kNoReLoaded;
    fContext->setWindowPositionAndSize(std::nullopt, ImVec2{config::kWelcomeWindowWidth,
                                                            config::kWelcomeWindowHeight});
    fContext->setWindowTitle(config::kWelcomeWindowTitle);
  }
}

//------------------------------------------------------------------------
// Application::onNativeWindowFontDpiScaleChange
//------------------------------------------------------------------------
void Application::onNativeWindowFontDpiScaleChange(float iFontDpiScale)
{
  fFontManager->setDpiFontScale(iFontDpiScale);
}

//------------------------------------------------------------------------
// Application::onNativeWindowFontScaleChange
//------------------------------------------------------------------------
void Application::onNativeWindowFontScaleChange(float iFontScale)
{
  fFontManager->setFontScale(iFontScale);
}

//------------------------------------------------------------------------
// Application::handleNewFrameActions
//------------------------------------------------------------------------
void Application::handleNewFrameActions()
{
  // we move before iterating so that action() can enqueue for next frame
  auto actions = std::move(fNewFrameActions);
  for(auto &action: actions)
    action();
}

//------------------------------------------------------------------------
// Application::handleAsyncActions
//------------------------------------------------------------------------
void Application::handleAsyncActions()
{
  auto futures = std::move(fAsyncActions);
  for(auto &future : futures)
  {
    switch(future.wait_for(std::chrono::seconds(0)))
    {
      case std::future_status::ready:
      {
        auto action = executeAndLogOnException<gui_action_t>([&future] { return future.get(); });
        if(action)
          action();
        break;
      }
      case std::future_status::timeout:
        // not complete => moving back to fAsyncActions
        fAsyncActions.emplace_back(std::move(future));
        break;
      case std::future_status::deferred:
        RE_EDIT_FAIL("not reached");
    }
  }
}

//------------------------------------------------------------------------
// Application::handleFontChangeRequest
//------------------------------------------------------------------------
void Application::handleFontChangeRequest()
{
  auto oldDpiScale = fFontManager->getCurrentFontDpiScale();
  fFontManager->applyFontChangeRequest();
  auto newDpiScale = fFontManager->getCurrentFontDpiScale();

  if(oldDpiScale != newDpiScale)
  {
    auto scaleFactor = newDpiScale;
    ImGuiStyle newStyle{};
    ImGui::StyleColorsDark(&newStyle);
    newStyle.ScaleAllSizes(scaleFactor);
    ImGui::GetStyle() = newStyle;
  }

  if(fAppContext)
    fAppContext->fRecomputeDimensionsRequested = true;
}

//------------------------------------------------------------------------
// Application::newFrame
//------------------------------------------------------------------------
bool Application::newFrame() noexcept
{
  Utils::StorageRAII<Application> current{&kCurrent, this};

  try
  {
    if(!fNewFrameActions.empty())
      handleNewFrameActions();

    if(!fAsyncActions.empty())
      handleAsyncActions();

    if(fFontManager->hasFontChangeRequest())
      handleFontChangeRequest();

    if(fAppContext)
      fAppContext->newFrame();
  }
  catch(...)
  {
    newExceptionDialog("Error during newFrame", true, std::current_exception());
    executeAndAbortOnException([e = std::current_exception()] {
      RE_EDIT_LOG_ERROR("Unrecoverable exception detected: %s", what(e));
      ImGui::ErrorCheckEndFrameRecover(nullptr);
    });
  }

  return running();
}

//------------------------------------------------------------------------
// Application::render
//------------------------------------------------------------------------
bool Application::render() noexcept
{
  Utils::StorageRAII<Application> current{&kCurrent, this};

  if(hasDialog())
  {
    try
    {
      renderDialog();
    }
    catch(...)
    {
      newExceptionDialog("Error during dialog rendering", true, std::current_exception());
      executeAndAbortOnException([e = std::current_exception()] {
        RE_EDIT_LOG_ERROR("Unrecoverable exception detected: %s", what(e));
        ImGui::ErrorCheckEndFrameRecover(nullptr);
      });
    }
  }

  try
  {
    switch(fState)
    {
      case State::kReLoaded:
        renderAppContext();
        break;

      case State::kNoReLoaded:
        renderWelcome();
        break;

      case State::kReLoading:
        renderLoading();
        break;

      default:
        break;
    }
  }
  catch(...)
  {
    newExceptionDialog("Error during rendering", true, std::current_exception());
    executeAndAbortOnException([e = std::current_exception()] {
      RE_EDIT_LOG_ERROR("Unrecoverable exception detected: %s", what(e));
      ImGui::ErrorCheckEndFrameRecover(nullptr);
    });
  }

  return running();
}

namespace impl {

//------------------------------------------------------------------------
// impl::getFrameNumberFromDeviceType (filmstrip with each frame being a different type)
//------------------------------------------------------------------------
static int getFrameNumberFromDeviceType(std::string const &iType)
{
  if(iType == "creative_fx" || iType == "studio_fx")
    return 0;

  if(iType == "instrument")
    return 1;

  if(iType == "note_player")
    return 2;

  return 3;
}

}


//------------------------------------------------------------------------
// Application::getLogo
//------------------------------------------------------------------------
std::shared_ptr<Texture> Application::getLogo() const
{
  return fTextureManager->getTexture(BuiltIns::kLogoDark.fKey);
}

//------------------------------------------------------------------------
// Application::renderWelcome
//------------------------------------------------------------------------
void Application::renderWelcome()
{
  auto scale = getCurrentFontDpiScale();

  const float padding = 20.0f * scale;
  const ImVec2 buttonSize{120.0f * scale, 0};
  const auto logoModifier = ReGui::Modifier{}
    .padding(10.0f * scale)
    .backgroundColor(ReGui::GetColorU32(toFloatColor(78, 78, 78)))
    .borderColor(ReGui::kWhiteColorU32);

  if(hasDialog())
    return;

  auto viewport = ImGui::GetWindowViewport();

  auto defaultWindowPadding = ImGui::GetStyle().WindowPadding;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, {0.5f, 0.5f});
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {padding, padding});

  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowPos({});

  if(ImGui::Begin(config::kWelcomeWindowTitle, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_HorizontalScrollbar))
  {
    auto textSizeHeight = ImGui::CalcTextSize("R").y;

    ReGui::Box(logoModifier, [this, textSizeHeight]() {
      auto logo = getLogo();
      auto computedHeight = 2.0f * textSizeHeight + (ImGui::GetStyle().ItemSpacing.y);
      logo->Item({}, {computedHeight, computedHeight});

      ImGui::SameLine();

      ImGui::BeginGroup();
      {
        ImGui::TextUnformatted("re-edit");
        ImGui::Text("%s", kFullVersion);
        ImGui::EndGroup();
      }
    });

    ImGui::SameLine(0, padding);

    ImGui::BeginGroup();
    {
      if(ImGui::Button("Open", buttonSize))
      {
        renderLoadDialogBlocking();
      }
      ImGui::SameLine();
      if(ImGui::Button("Quit", buttonSize))
      {
        exit();
      }
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      if(fConfig.fDeviceHistory.empty())
      {
        ImGui::TextUnformatted("No history");
      }
      else
      {
        auto icon = fTextureManager->getTexture(BuiltIns::kDeviceType.fKey);

        ImGui::PushStyleColor(ImGuiCol_Button, 0); // make the button transparent

        auto buttonHeight = 2.0f * (textSizeHeight + ImGui::GetStyle().FramePadding.y);

        std::optional<std::string> deviceToRemoveFromHistory{};

        for(auto i = fConfig.fDeviceHistory.rbegin(); i != fConfig.fDeviceHistory.rend(); i++)
        {
          auto const &item = *i;

          ImGui::Spacing();
          ImGui::BeginGroup();
          {
            ImGui::AlignTextToFramePadding();
            icon->Item({}, {buttonHeight, buttonHeight}, 1.0f, impl::getFrameNumberFromDeviceType(item.fType));
            ImGui::SameLine();
            if(ImGui::Button(fmt::printf("%s\n%s", item.fName, item.fPath).c_str()))
              loadProjectDeferred(fs::path(item.fPath));
          }
          ImGui::EndGroup();
          ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, defaultWindowPadding);
          if(ImGui::BeginPopupContextItem(item.fPath.c_str()))
          {
            if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Open, "Open")))
              loadProjectDeferred(fs::path(item.fPath));

            if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Reset, "Remove from recent")))
              deviceToRemoveFromHistory = item.fPath;
            ImGui::EndPopup();
          }
          ImGui::PopStyleVar();
        }

        if(deviceToRemoveFromHistory)
          fConfig.removeDeviceConfigFromHistory(*deviceToRemoveFromHistory);

        ImGui::PopStyleColor(1);
      }

      ImGui::EndGroup();
    }
  }

  ImGui::End();

  ImGui::PopStyleVar(2);

}

//------------------------------------------------------------------------
// Application::renderLoading
//------------------------------------------------------------------------
void Application::renderLoading()
{
  RE_EDIT_INTERNAL_ASSERT(fReLoadingFuture != nullptr);

  auto scale = getCurrentFontDpiScale();
  const float padding = 32.0f * scale;

  auto constexpr kTitle = "Loading...";
  const auto logoModifier = ReGui::Modifier{}
    .padding(padding)
    .backgroundColor(ReGui::GetColorU32(toFloatColor(78, 78, 78)))
    .borderColor(ReGui::kWhiteColorU32);

  if(fReLoadingFuture->fFuture.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
  {
    deferNextFrame(fReLoadingFuture->fFuture.get());
    fReLoadingFuture = nullptr;
  }

  if(!ImGui::IsPopupOpen(kTitle))
  {
    ImGui::OpenPopup(kTitle);
    ReGui::CenterNextWindow();
  }

  if(ImGui::BeginPopupModal(kTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar))
  {
    if(fReLoadingFuture)
    {
      ReGui::Box(logoModifier, [this]() {
        auto logo = getLogo();
        logo->Item({}, {64.0f, 64.0f});
      });
      auto progress = fReLoadingFuture->progress();
      ImGui::ProgressBar(static_cast<float>(progress.first) / 12.0f,
                         {ImGui::GetItemRectSize().x, 0},
                         progress.second.c_str());
      if(ImGui::Button("Cancel"))
      {
        fReLoadingFuture->cancel();
      }
    }
    else
    {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}


//------------------------------------------------------------------------
// Application::renderAppContext
//------------------------------------------------------------------------
void Application::renderAppContext()
{
  RE_EDIT_INTERNAL_ASSERT(fAppContext != nullptr);

  Utils::StorageRAII<AppContext> current{&AppContext::kCurrent, fAppContext.get()};

  fAppContext->beforeRenderFrame();
  renderMainMenu();
  fAppContext->renderMainMenu();

  ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

  fAppContext->render();

  auto loggingManager = LoggingManager::instance();

  if(loggingManager->isShowDebug())
  {
    loggingManager->debug("Undo", "History[%d]", fAppContext->fUndoManager->getUndoHistory().size());
    loggingManager->debug("Redo", "History[%d]", fAppContext->fUndoManager->getRedoHistory().size());
  }

  loggingManager->render();

  if(fShowDemoWindow)
    ImGui::ShowDemoWindow(&fShowDemoWindow);
  if(fShowMetricsWindow)
    ImGui::ShowMetricsWindow(&fShowMetricsWindow);

  fAppContext->afterRenderFrame();
}

//------------------------------------------------------------------------
// Application::renderMainMenu
//------------------------------------------------------------------------
void Application::renderMainMenu()
{
  if(ImGui::BeginMainMenuBar())
  {
    if(ImGui::BeginMenu("re-edit"))
    {
      if(ImGui::MenuItem("About"))
      {
        newDialog("About")
          .lambda([this]() { about(); }, true)
          .buttonOk();
      }
      if(ImGui::MenuItem("Check for Updates..."))
      {
        asyncCheckForUpdates();
      }
      if(ImGui::MenuItem("Quit"))
      {
        maybeExit();
      }
      ImGui::EndMenu();
    }
    if(ImGui::BeginMenu("File"))
    {
      if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Open, "Open")))
      {
        renderLoadDialogBlocking();
      }
      if(fConfig.fDeviceHistory.size() > 1)
      {
        if(ImGui::BeginMenu("Open Recent"))
        {
          // we skip the current device!
          for(auto i = fConfig.fDeviceHistory.rbegin() + 1; i != fConfig.fDeviceHistory.rend(); i++)
          {
            auto const &item = *i;
            ImGui::PushID(item.fPath.c_str()); // fName is not unique
            if(ImGui::MenuItem(item.fName.c_str()))
            {
              maybeCloseProject(std::nullopt, [this, path = item.fPath]() { loadProject(path); });
            }
            if(ImGui::IsItemHovered())
            {
              ImGui::BeginTooltip();
              ImGui::TextUnformatted(item.fPath.c_str());
              ImGui::EndTooltip();
            }
            ImGui::PopID();
          }
          ImGui::EndMenu();
        }
      }
      ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Edit"))
    {
      // empty on purpose (AppContext fills this)
      ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Window"))
    {
      // empty on purpose (AppContext fills this)
      ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Dev"))
    {
      auto loggingManager = LoggingManager::instance();
      {
        bool b = loggingManager->isShowDebug();
        if(ImGui::MenuItem("Debug", nullptr, &b))
          loggingManager->setShowDebug(b);
      }
      {
        bool b = loggingManager->isShowLog();
        if(ImGui::MenuItem(fmt::printf("Log [%d]##Log", loggingManager->getLogCount()).c_str(), nullptr, &b))
          loggingManager->setShowLog(b);
      }
      ImGui::Separator();
      ImGui::MenuItem("ImGui Demo", nullptr, &fShowDemoWindow);
      ImGui::MenuItem("ImGui Metrics", nullptr, &fShowMetricsWindow);
      if(ImGui::MenuItem("gui_2D.cmake"))
      {
        newDialog("gui_2D.cmake")
        .text(fAppContext->cmake(), true)
        .buttonOk();
      }
      if(ImGui::MenuItem("imgui.ini"))
      {
        newDialog("imgui.ini")
          .text(ImGui::SaveIniSettingsToMemory(), true)
          .buttonOk();
      }
      if(ImGui::MenuItem("Global Config"))
      {
        newDialog("Global Config")
          .text(PreferencesManager::getAsLua(fConfig), true)
          .buttonOk();
      }
      ImGui::Separator();
      ImGui::Text("Version: %s", kFullVersion);
      ImGui::Text("Build: %s", kGitVersion);
      ImGui::EndMenu();
    }
//    ImGui::Separator();
//    ImGui::TextUnformatted(fAppContext->fPropertyManager->getDeviceInfo().fMediumName.c_str());
//    ImGui::TextUnformatted(fAppContext->fPropertyManager->getDeviceInfo().fVersionNumber.c_str());
    ImGui::EndMainMenuBar();
  }
}

//------------------------------------------------------------------------
// Application::renderLoadDialogBlocking
//------------------------------------------------------------------------
void Application::renderLoadDialogBlocking()
{
  nfdchar_t *outPath;
  nfdfilteritem_t filterItem[] = { { "Info", "lua" } };
  nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);
  if(result == NFD_OKAY)
  {
    fs::path luaPath{outPath};
    NFD_FreePath(outPath);
    auto newProjectPath = impl::inferValidRoot(luaPath);
    if(!newProjectPath)
      newDialog("Invalid")
      .preContentMessage("Cannot load Rack Extension")
      .text(fmt::printf("%s is not a valid Rack Extension project (could not find info.lua)", luaPath.u8string()))
      .buttonOk();
    else
      maybeCloseProject(std::nullopt, [this, path = *newProjectPath]() { loadProject(path); });
  }
  else if(result == NFD_CANCEL)
  {
    // nothing to do
  }
  else
  {
    RE_EDIT_LOG_WARNING("Error while opening project: %s", NFD_GetError());
  }
}

//------------------------------------------------------------------------
// Application::newDialog
//------------------------------------------------------------------------
ReGui::Dialog &Application::newDialog(std::string iTitle, bool iHighPriority)
{
  auto dialog = std::make_unique<ReGui::Dialog>(std::move(iTitle));
  if(iHighPriority)
  {
    fCurrentDialog = std::move(dialog);
    return *fCurrentDialog;
  }
  else
  {
    fDialogs.emplace_back(std::move(dialog));
    return *fDialogs[fDialogs.size() - 1];
  }
}


//------------------------------------------------------------------------
// Application::newExceptionDialog
//------------------------------------------------------------------------
void Application::newExceptionDialog(std::string iMessage, bool iSaveButton, std::exception_ptr const &iException)
{
  if(!hasException())
  {
    fState = State::kException;
    auto &dialog =
      newDialog("Error", true)
        .preContentMessage(std::move(iMessage))
        .text(what(iException), true);

    if(iSaveButton)
      dialog.button("Save", [this] { maybeSaveProject([this]() { exit(); }); }, true);

    dialog.button("Exit", [this] { exit(); }, true);

    dialog.postContentMessage("Note: If you think this is an error in the tool, please report it at https://github.com/pongasoft/re-edit-dev/issues");
  }
  else
  {
    RE_EDIT_LOG_ERROR("Error while handling error... aborting | %s", what(iException));
    exit();
  }
}

//------------------------------------------------------------------------
// Application::renderDialog
//------------------------------------------------------------------------
void Application::renderDialog()
{
  if(!fCurrentDialog)
  {
    if(fDialogs.empty())
      return;
    fCurrentDialog = std::move(fDialogs[0]);
    fDialogs.erase(fDialogs.begin());
  }

  fCurrentDialog->render();
  if(!fCurrentDialog->isOpen())
    fCurrentDialog = nullptr;
}


//------------------------------------------------------------------------
// Application::maybeExit
//------------------------------------------------------------------------
void Application::maybeExit()
{
  if(!running())
    return;

  maybeCloseProject("Quit", [this] { exit(); });
}

//------------------------------------------------------------------------
// Application::readFile
//------------------------------------------------------------------------
std::optional<std::string> Application::readFile(fs::path const &iFile, UserError *oErrors)
{
  try
  {
    // we check if the content has changed as there is no reason to overwrite (which changes the modified time)
    if(fs::exists(iFile) && !fs::is_directory(iFile))
    {
      const auto size = fs::file_size(iFile);
      std::ifstream contentFile(iFile, std::ios::in | std::ios::binary);
      std::string content(size, '\0');
      contentFile.read(content.data(), static_cast<std::streamsize>(size));
      return content;
    }
  }
  catch(...)
  {
    if(oErrors)
      oErrors->add("Error while reading file %s: %s", iFile, what(std::current_exception()));
  }

  return std::nullopt;
}

//------------------------------------------------------------------------
// Application::saveFile
//------------------------------------------------------------------------
void Application::saveFile(fs::path const &iFile, std::string const &iContent, UserError *oErrors)
{
  try
  {
    auto originalContent = readFile(iFile);
    if(originalContent && *originalContent == iContent)
    {
      // no change => no save
      return;
    }

    // if the parent directory does not exist, just create it
    auto dir = iFile.parent_path();
    if(!fs::exists(dir))
      fs::create_directories(dir);

    // we do it in 2 steps since step 1 is the one that is more likely to fail
    // 1. we save in a new file
    auto tmpFile = dir / fmt::printf("%s.re_edit.tmp", iFile.filename().u8string());
    std::ofstream f{tmpFile};
    f.exceptions(std::ofstream::ios_base::failbit | std::ofstream::ios_base::badbit);
    f << iContent;
    f.close();
    // 2. we rename
    fs::rename(tmpFile, iFile);
  }
  catch(...)
  {
    if(oErrors)
      oErrors->add("Error while saving file %s: %s", iFile, what(std::current_exception()));
  }
}

//------------------------------------------------------------------------
// Application::about
//------------------------------------------------------------------------
void Application::about() const
{
  ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
  if(ImGui::TreeNodeEx("re-edit", ImGuiTreeNodeFlags_Framed))
  {
    ImGui::Text("Version:      %s", kFullVersion);
    ImGui::Text("Git Version:  %s", kGitVersion);
    ImGui::Text("Git Tag:      %s", kGitTag);
    ImGui::Text("Architecture: %s", kArchiveArchitecture);
    ImGui::Text("re-mock:      %s", kReMockVersion);
    ImGui::TreePop();
  }

  constexpr auto boolToString = [](bool b) { return b ? "true" : "false"; };

  ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
  if(ImGui::TreeNodeEx("Rack Extension", ImGuiTreeNodeFlags_Framed))
  {
    auto const &info = fAppContext->fPropertyManager->getDeviceInfo();
    ImGui::Text("long_name:                       %s", info.fLongName.c_str());
    ImGui::Text("medium_name:                     %s", info.fMediumName.c_str());
    ImGui::Text("short_name:                      %s", info.fShortName.c_str());
    ImGui::Text("product_id:                      %s", info.fProductId.c_str());
    ImGui::Text("manufacturer:                    %s", info.fManufacturer.c_str());
    ImGui::Text("version_number:                  %s", info.fVersionNumber.c_str());
    ImGui::Text("device_type:                     %s", deviceTypeToString(info.fDeviceType));
    ImGui::Text("supports_patches:                %s", boolToString(info.fSupportPatches));
    ImGui::Text("default_patch:                   %s", info.fDefaultPatch.c_str());
    ImGui::Text("accepts_notes:                   %s", boolToString(info.fAcceptNotes));
    ImGui::Text("auto_create_note_lane:           %s", boolToString(info.fAutoCreateNoteLane));
    ImGui::Text("supports_performance_automation: %s", boolToString(info.fSupportsPerformanceAutomation));
    ImGui::Text("device_height_ru:                %d", info.fDeviceHeightRU);
    ImGui::TreePop();
  }


}


}