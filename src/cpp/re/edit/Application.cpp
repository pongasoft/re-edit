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

const std::string kCheckForUpdatesKey{"CFU_Key"};

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
bool Application::async(std::string const &iKey, Function &&f, Args &&... args)
{
  RE_EDIT_INTERNAL_ASSERT(fGUIThreadID == std::this_thread::get_id(), "Can only be called from the GUI thread!");
  if(!hasAsyncAction(iKey))
  {
    std::future<gui_action_t> action = std::async(std::launch::async,
                                                  std::forward<Function>(f),
                                                  std::forward<Args>(args)...);
    fAsyncActions[iKey] = std::move(action);
    return true;
  }
  else
  {
    RE_EDIT_LOG_DEBUG("already running %s", iKey);
    return false;
  }
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
  if(!fConfig.fSaveEnabled)
  {
    RE_EDIT_LOG_DEBUG("Preference saving is disabled... skipping");
    return;
  }

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
  // we resize the window once the scale is set/known
  deferNextFrame([this]{
    fContext->setWindowPositionAndSize(std::nullopt, ImVec2{config::kWelcomeWindowWidth,
                                                            config::kWelcomeWindowHeight});
  });
  if(iConfig.fProjectRoot)
    loadProjectDeferred(*iConfig.fProjectRoot);
}


//------------------------------------------------------------------------
// Application::shutdown
//------------------------------------------------------------------------
bool Application::shutdown(long iTimeoutMillis)
{
  auto timeout = std::chrono::milliseconds(iTimeoutMillis);

  if(fReLoadingFuture)
  {
    fReLoadingFuture->cancel();
    if(fReLoadingFuture->fFuture.wait_for(timeout) != std::future_status::ready)
      return false;
  }

  for(auto &[k, future]: fAsyncActions)
  {
    if(future.wait_for(timeout) != std::future_status::ready)
      return false;
  }

  return true;
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
  async(kCheckForUpdatesKey, [this]() {
    auto latestRelease = fNetworkManager->getLatestRelease();
//    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
//    std::optional<Release> latestRelease = Release{"v0.0.0", "https://github.com/pongasoft/jamba/releases/tag/v6.0.1", "* Fixed gtest crash on Apple M1 platform\r\n"};
//    std::optional<Release> latestRelease = Release{kGitTag, "https://github.com/pongasoft/jamba/releases/tag/v6.0.1", "* Fixed gtest crash on Apple M1 platform\r\n"};
    if(latestRelease)
      return gui_action_t([this, latestRelease = *latestRelease]() {
        fLatestRelease = latestRelease;
        if(fState == State::kReLoaded)
        {
          auto &dialog = newDialog("Check for Updates");
          dialog.lambda([this]() { renderLogoBox(); });
          dialog.text(fmt::printf("Latest Version: %s", latestRelease.fVersion));
          if(latestRelease.fVersion != std::string(kGitTag))
          {
            dialog.text(fmt::printf("There is a new version (you currently have %s).\n"
                                    "Release Notes\n"
                                    "-------------\n"
                                    "%s", kGitTag, (latestRelease.fReleaseNotes ? *latestRelease.fReleaseNotes : "")));
            if(latestRelease.fURL)
            {
              dialog.button("Download", [this, url = *latestRelease.fURL]() { fContext->openURL(url); });
              dialog.buttonCancel();
            }
            else
              dialog.buttonOk();
          }
          else
          {
            dialog.text("You are running the latest version.");
            dialog.buttonOk();
          }
        }
      });
    else
      return gui_action_t();
  });
}


//------------------------------------------------------------------------
// Application::applyConfigStyle
//------------------------------------------------------------------------
void Application::applyConfigStyle() const
{
  auto scaleFactor = fFontManager->getCurrentFontDpiScale();
  ImGuiStyle newStyle{};
  newStyle.WindowMenuButtonPosition = ImGuiDir_Right;
  switch(fConfig.fStyle)
  {
    case config::Style::kLight:
      ImGui::StyleColorsLight(&newStyle);
      break;

    case config::Style::kClassic:
      ImGui::StyleColorsClassic(&newStyle);
      break;

    default:
      ImGui::StyleColorsDark(&newStyle);
      break;
  }
  newStyle.ScaleAllSizes(scaleFactor);
  ImGui::GetStyle() = newStyle;
}

//------------------------------------------------------------------------
// Application::init
//------------------------------------------------------------------------
void Application::init()
{
  fGUIThreadID = std::this_thread::get_id();

  fTextureManager = fContext->newTextureManager();
  fTextureManager->init(BuiltIns::kGlobalBuiltIns);
  fFontManager = std::make_shared<FontManager>();
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
// Application::initAppContext
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
  fContext->setWindowTitle(fmt::printf("RE Edit - Loading [%s]", iRoot.u8string()));
  fReLoadingFuture = std::make_unique<CancellableFuture<gui_action_t>>();
  fReLoadingFuture->launch([this, iRoot, c, cancellable = fReLoadingFuture->fCancellable] {
    try
    {
      //std::this_thread::sleep_for(std::chrono::seconds(5));
      auto appContext = initAppContext(iRoot, c, cancellable);
      return gui_action_t([this, c, ctx = std::move(appContext)]() {
        if(fState == State::kReLoading)
        {
          fAppContext = ctx;
          fState = State::kReLoaded;
          if(!fContext->isHeadless())
            ImGui::LoadIniSettingsFromMemory(c.fImGuiIni.c_str(), c.fImGuiIni.size());
          fContext->setWindowPositionAndSize(c.fNativeWindowPos, c.fNativeWindowSize);
          fContext->setWindowTitle(fmt::printf("RE Edit - %s", fAppContext->getConfig().fName));
          savePreferences();
          deferNextFrame([ctx] { ctx->requestZoomToFit(); });
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
inline void maybeInvoke(Application::gui_action_t const &iAction) { if(iAction) iAction(); }

}

//------------------------------------------------------------------------
// Application::maybeLoadProject
//------------------------------------------------------------------------
void Application::maybeLoadProject(fs::path const &iProjectRoot)
{
  auto validRoot = impl::inferValidRoot(iProjectRoot);
  if(validRoot)
  {
    if(fState == State::kReLoaded || fState == State::kNoReLoaded)
      maybeCloseProject(std::nullopt, [this, path = *validRoot]() { loadProject(path); });
  }
}

//------------------------------------------------------------------------
// Application::maybeSaveProject
//------------------------------------------------------------------------
void Application::maybeSaveProject(gui_action_t const &iNextAction)
{
  if(fAppContext)
  {
    auto action = impl::chain([this]() { saveProject(); }, iNextAction);

    action = [this, action, iNextAction] {
      if(!fAppContext->getReEditVersion())
      {
        newDialog(fmt::printf("Saving - %s", fAppContext->getDeviceName()))
          .preContentMessage("Warning")
          .text("This is the first time you save this project.\n"
                "Saving will override hdgui_2d.lua and device_2d.lua.\n"
                "Are you sure you want to proceed?")
          .button("Yes (save)", [action] { action(); })
          .button("No (don't save)", [iNextAction] { impl::maybeInvoke(iNextAction); })
          .button("Cancel", {});
      }
      else
        action();
    };

    action = [this, action, iNextAction] {
      if(fAppContext->computeErrors())
      {
        auto &dialog = newDialog(fmt::printf("Saving - %s", fAppContext->getDeviceName()))
          .preContentMessage("Warning - Errors detected")
          .lambda([this] { fAppContext->renderErrors(); })
          .postContentMessage("Are you sure you want to proceed?")
          .button("Yes (save)", [action] { action(); })
          .button("No (don't save)", [iNextAction] { impl::maybeInvoke(iNextAction); })
          .button("Cancel", {});
          ;
      }
      else
        action();
    };

    action();
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
  fNotifications.clear();
  fAppContext = nullptr;
  auto loggingManager = LoggingManager::instance();
  loggingManager->clearAll();
  loggingManager->setShowLog(false);
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
// Application::onNativeDropFiles
//------------------------------------------------------------------------
void Application::onNativeDropFiles(std::vector<fs::path> const &iPaths)
{
  if(fState == State::kReLoaded || fState == State::kNoReLoaded)
  {
    // first we try to find a project in the list of paths
    auto project = std::find_if(iPaths.begin(), iPaths.end(), [](const fs::path &p) { return impl::inferValidRoot(p) != std::nullopt; });
    if(project != iPaths.end())
    {
      maybeLoadProject(*project);
      RE_EDIT_LOG_DEBUG("onNativeDropFiles: Detected valid project %s", project->u8string().c_str());
    }
    else
    {
      if(fState == State::kReLoaded)
      {
        // no project found => textures?
        std::vector<fs::path> textures{};
        std::copy_if(iPaths.begin(),
                     iPaths.end(),
                     std::back_inserter(textures),
                     [](const fs::path &p) { return FilmStripMgr::isValidTexturePath(p); }
        );
        if(!textures.empty())
        {
          RE_EDIT_LOG_DEBUG("onNativeDropFiles: Detected %ld textures to import", textures.size());
          deferNextFrame([textures = std::move(textures), this]() {
            // we check again to make sure we still are in the proper state...
            if(fState == State::kReLoaded)
            {
              RE_EDIT_INTERNAL_ASSERT(fAppContext != nullptr);

              std::vector<FilmStrip::key_t> textureKeys{};
              textureKeys.reserve(textures.size());
              for(auto const &texture: textures)
              {
                auto maybeKey = fAppContext->importTexture(texture);
                if(maybeKey)
                  textureKeys.emplace_back(*maybeKey);
              }

              if(!textureKeys.empty())
              {
                newDialog("Texture Import")
                  .preContentMessage(fmt::printf("Successfully imported %ld graphics", textureKeys.size()))
                  .lambda([ctx = fAppContext.get(), keys = textureKeys]() {
                    if(AppContext::IsCurrent(ctx))
                    {
                      auto textureSize = ImGui::CalcTextSize("W").y * 2.0f;

                      for(auto &key: keys)
                      {
                        auto texture = ctx->findTexture(key);
                        if(texture && texture->isValid())
                        {
                          texture->ItemFit({textureSize, textureSize});
                          ImGui::SameLine();
                          ImGui::TextUnformatted(key.c_str());
                        }
                      }
                    }
                  })
                  .buttonOk("Ok", true);
              }
            }
          });
        }
      }
      else
      {
        RE_EDIT_LOG_DEBUG("onNativeDropFiles: cannot import textures when no project loaded");
      }
    }
  }
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
  for(auto &pair : futures)
  {
    auto &key = pair.first;
    auto &future = pair.second;
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
        fAsyncActions[key] = std::move(future);
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
  fFontManager->applyFontChangeRequest();

  applyConfigStyle();

  if(fAppContext)
    fAppContext->fRecomputeDimensionsRequested = true;
}

//------------------------------------------------------------------------
// Application::newFrame
//------------------------------------------------------------------------
bool Application::newFrame(std::vector<gui_action_t> const &iFrameActions) noexcept
{
  Utils::StorageRAII<Application> current{&kCurrent, this};

  try
  {
    if(!iFrameActions.empty())
    {
      for(auto &action: iFrameActions)
        action();
    }
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
    renderNotifications({});
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
// Application::renderLogoBox
//------------------------------------------------------------------------
void Application::renderLogoBox(float iPadding)
{
  static constexpr auto kBackgroundLightColor = ReGui::GetColorU32(toFloatColor(156, 156, 156));
  static constexpr auto kBackgroundDarkColor = ReGui::GetColorU32(toFloatColor(78, 78, 78));

  auto scale = getCurrentFontDpiScale();
  const auto logoModifier = ReGui::Modifier{}
    .padding(iPadding * scale)
    .backgroundColor(fConfig.fStyle == config::Style::kLight ? kBackgroundLightColor : kBackgroundDarkColor)
    .borderColor(fConfig.fStyle == config::Style::kLight ? ReGui::kBlackColorU32 : ReGui::kWhiteColorU32);
  auto textSizeHeight = ImGui::CalcTextSize("R").y;

  auto newVersion = hasNewVersion();

  ReGui::Box(logoModifier, [this, textSizeHeight, newVersion]() {
    auto logo = getLogo();
    auto computedHeight = 2.0f * textSizeHeight + (ImGui::GetStyle().ItemSpacing.y);
    logo->Item({computedHeight, computedHeight});

    ImGui::SameLine();

    ImGui::BeginGroup();
    {
      ImGui::TextUnformatted("RE Edit");
      ImGui::Text("%s%s", kFullVersion, newVersion ? "*" : "");
      ImGui::EndGroup();
    }
  });
}

//------------------------------------------------------------------------
// Application::getDeviceTypeIcon
//------------------------------------------------------------------------
Icon Application::getDeviceTypeIcon(config::Device const &iDevice) const
{
  return {fTextureManager->getTexture(BuiltIns::kDeviceType.fKey), impl::getFrameNumberFromDeviceType(iDevice.fType)};
}

//------------------------------------------------------------------------
// Application::renderWelcome
//------------------------------------------------------------------------
void Application::renderWelcome()
{
  auto scale = getCurrentFontDpiScale();
  const float padding = 20.0f * scale;
  const ImVec2 buttonSize{120.0f * scale, 0};

  if(hasDialog())
    return;

  auto viewport = ImGui::GetWindowViewport();

  auto defaultWindowPadding = ImGui::GetStyle().WindowPadding;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, {0.5f, 0.5f});
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {padding, padding});

  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowPos({});

  constexpr auto flags = ImGuiWindowFlags_NoMove
                         | ImGuiWindowFlags_NoCollapse
                         | ImGuiWindowFlags_NoTitleBar
                         | ImGuiWindowFlags_NoResize
                         | ImGuiWindowFlags_HorizontalScrollbar;
  if(ImGui::Begin(config::kWelcomeWindowTitle, nullptr, flags))
  {
    ImGui::BeginGroup();
    {
      renderLogoBox();

      ImGui::TextUnformatted("© 2024 pongasoft");

      auto textSizeHeight = ImGui::CalcTextSize("R").y;
      auto remainingSizeY = viewport->Size.y - ImGui::GetCursorPosY();
      auto sizeY = remainingSizeY - textSizeHeight - padding - ImGui::GetStyle().ScrollbarSize;

      // empty space to position the Menu button at the bottom
      ImGui::Dummy({0, sizeY});

      if(ReGui::MenuButton())
        ImGui::OpenPopup("Menu");

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, defaultWindowPadding);
      if(ImGui::BeginPopup("Menu"))
      {
        renderApplicationMenuItems();
        ImGui::EndPopup();
      }
      ImGui::PopStyleVar();
      ImGui::EndGroup();
    }

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

      if(hasNewVersion())
      {
        if(ImGui::TreeNodeEx("New Release"))
        {
          ImGui::Text("Latest Release: %s", fLatestRelease->fVersion.c_str());
          if(fLatestRelease->fReleaseNotes)
            ReGui::MultiLineText(*fLatestRelease->fReleaseNotes);
          if(ImGui::Button("Download"))
            downloadLatestVersion();
          ImGui::TreePop();
        }
        ImGui::Separator();
        ImGui::Spacing();
      }

      if(fConfig.fDeviceHistory.empty())
      {
        ImGui::TextUnformatted("No history");
      }
      else
      {
        auto icon = fTextureManager->getTexture(BuiltIns::kDeviceType.fKey);

        ImGui::PushStyleColor(ImGuiCol_Button, 0); // make the button transparent

        auto textSizeHeight = ImGui::CalcTextSize("R").y;
        auto buttonHeight = 2.0f * (textSizeHeight + ImGui::GetStyle().FramePadding.y);

        std::optional<std::string> deviceToRemoveFromHistory{};

        for(auto i = fConfig.fDeviceHistory.rbegin(); i != fConfig.fDeviceHistory.rend(); i++)
        {
          auto const &item = *i;

          ImGui::Spacing();
          ImGui::BeginGroup();
          {
            ImGui::AlignTextToFramePadding();
            icon->Item({buttonHeight, buttonHeight}, impl::getFrameNumberFromDeviceType(item.fType));
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
// Application::downloadLatestVersion
//------------------------------------------------------------------------
void Application::downloadLatestVersion() const
{
  if(fLatestRelease && fLatestRelease->fURL)
    fContext->openURL(*fLatestRelease->fURL);
}

//------------------------------------------------------------------------
// Application::renderLoading
//------------------------------------------------------------------------
void Application::renderLoading()
{
  RE_EDIT_INTERNAL_ASSERT(fReLoadingFuture != nullptr);

  auto constexpr kTitle = "Loading...";

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
      renderLogoBox(32.0f);
      auto progress = fReLoadingFuture->progress();
      ImGui::ProgressBar(static_cast<float>(progress.first) / 14.0f,
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
    loggingManager->debug("ActiveID", "ActiveID[%d]", ImGui::GetCurrentContext()->ActiveId);
  }

  loggingManager->render();

  if(fShowDemoWindow)
    ImGui::ShowDemoWindow(&fShowDemoWindow);
  if(fShowMetricsWindow)
    ImGui::ShowMetricsWindow(&fShowMetricsWindow);

  fAppContext->afterRenderFrame();
}


//------------------------------------------------------------------------
// Application::renderApplicationMenuItems
//------------------------------------------------------------------------
void Application::renderApplicationMenuItems()
{
  if(ImGui::MenuItem("About"))
  {
    newAboutDialog();
  }
  if(ImGui::MenuItem("Help"))
  {
    newHelpDialog();
  }
  if(ImGui::BeginMenu("Style"))
  {
    std::optional<config::Style> newStyle{};
    if(ImGui::MenuItem("Dark", nullptr, fConfig.fStyle == config::Style::kDark))
      newStyle = config::Style::kDark;
    if(ImGui::MenuItem("Light", nullptr, fConfig.fStyle == config::Style::kLight))
      newStyle = config::Style::kLight;
    if(ImGui::MenuItem("Classic", nullptr, fConfig.fStyle == config::Style::kClassic))
      newStyle = config::Style::kClassic;
    if(newStyle)
    {
      fConfig.fStyle = *newStyle;
      applyConfigStyle();
      savePreferences();
    }
    ImGui::EndMenu();
  }
  if(ImGui::BeginMenu("Performance"))
  {
    if(ImGui::BeginMenu("Frame Rate"))
    {
      auto targetFrameRate = getTargetFrameRate();
      if(ImGui::MenuItem("60", nullptr, targetFrameRate == 60))
        targetFrameRate = 60;
      if(ImGui::MenuItem("120", nullptr, targetFrameRate == 120))
        targetFrameRate = 120;
      if(ImGui::MenuItem("Unlocked", nullptr, targetFrameRate == 0))
        targetFrameRate = 0;
      if(targetFrameRate != getTargetFrameRate())
      {
        fConfig.fTargetFrameRate = targetFrameRate;
        fContext->setTargetFrameRate(targetFrameRate);
      }
      ImGui::EndMenu();
    }
    if(ImGui::MenuItem("V-Sync Enabled", nullptr, &fConfig.fVSyncEnabled))
    {
      fContext->setVSyncEnabled(fConfig.fVSyncEnabled);
    }
    ImGui::MenuItem("Show Performance", nullptr, &fConfig.fShowPerformance);
    ImGui::EndMenu();
  }
  if(ImGui::MenuItem("Check For Updates...", nullptr, false, !hasAsyncAction(kCheckForUpdatesKey)))
  {
    asyncCheckForUpdates();
  }
  if(fLatestRelease)
  {
    bool newVersion = hasNewVersion();
    if(newVersion)
    {
      if(ImGui::MenuItem(fmt::printf("  Download - %s", fLatestRelease->fVersion).c_str()))
        downloadLatestVersion();
    }
    else
    {
      ImGui::MenuItem("  No New Update", nullptr, false, false);
    }
  }
  if(ImGui::MenuItem("Clear Recent List"))
  {
    fConfig.clearDeviceConfigHistory();
  }
  if(ImGui::MenuItem("Quit", ReGui_Menu_Shortcut2(ReGui_Icon_KeySuper, "Q")))
  {
    maybeExit();
  }
}

//------------------------------------------------------------------------
// Application::renderMainMenu
//------------------------------------------------------------------------
void Application::renderMainMenu()
{
  if(ImGui::BeginMainMenuBar())
  {
    if(ImGui::BeginMenu("RE Edit"))
    {
      renderApplicationMenuItems();
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
              maybeLoadProject(item.fPath);
            }
            if(ReGui::ShowQuickView())
            {
              ReGui::ToolTip([this, &item] {
                auto textSizeHeight = ImGui::CalcTextSize("R").y;
                auto buttonHeight = 2.0f * (textSizeHeight + ImGui::GetStyle().FramePadding.y);
                ImGui::BeginGroup();
                {
                  ImGui::AlignTextToFramePadding();
                  auto icon = getDeviceTypeIcon(item);
                  icon.Item({buttonHeight, buttonHeight});
                  ImGui::SameLine();
                  ImGui::TextUnformatted(fmt::printf("%s\n%s", item.fName, item.fPath).c_str());
                }
                ImGui::EndGroup();
              });
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
      auto loggingManager = LoggingManager::instance();
#ifndef NDEBUG
      {
        bool b = loggingManager->isShowDebug();
        if(ImGui::MenuItem("Debug", nullptr, &b))
          loggingManager->setShowDebug(b);
      }
#endif
      {
        bool b = loggingManager->isShowLog();
        if(ImGui::MenuItem(fmt::printf("Log [%d]##Log", loggingManager->getLogCount()).c_str(), nullptr, &b))
          loggingManager->setShowLog(b);
      }
      ImGui::Separator();
      ImGui::EndMenu();
    }

#ifndef NDEBUG
    if(ImGui::BeginMenu("Dev"))
    {
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
#endif

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
      maybeLoadProject(*newProjectPath);
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

    dialog.postContentMessage("Note: If you think this is an error in the tool, please report it at https://github.com/pongasoft/re-edit/issues");
  }
  else
  {
    RE_EDIT_LOG_ERROR("Error while handling error... aborting | %s", what(iException));
    exit();
  }
}


//------------------------------------------------------------------------
// Application::newNotification
//------------------------------------------------------------------------
ReGui::Notification &Application::newNotification()
{
  fNotifications.emplace_back(std::make_unique<ReGui::Notification>());
  return *fNotifications[fNotifications.size() - 1];
}

//------------------------------------------------------------------------
// Application::newUniqueNotification
//------------------------------------------------------------------------
ReGui::Notification &Application::newUniqueNotification(ReGui::Notification::Key const &iKey)
{
  auto iter = std::find_if(fNotifications.begin(), fNotifications.end(), [&iKey](auto &n) {
    return n->key() == iKey;
  });
  if(iter != fNotifications.end())
    (*iter)->dismiss();
  fNotifications.emplace_back(std::make_unique<ReGui::Notification>(iKey));
  return *fNotifications[fNotifications.size() - 1];
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
// Application::renderNotifications
//------------------------------------------------------------------------
void Application::renderNotifications(ImVec2 const &iOffset)
{
  static std::pair<ImGuiStyle, ImGuiStyle> kImGuiStyles = []() {
    ImGuiStyle darkStyle{};
    ImGui::StyleColorsDark(&darkStyle);
    ImGuiStyle lightStyle{};
    ImGui::StyleColorsLight(&lightStyle);
    return std::pair<ImGuiStyle, ImGuiStyle>{ darkStyle, lightStyle };
  }();

  if(fNotifications.empty())
    return;

  auto const notificationSize = ImGui::CalcTextSize("MMMMMMMMMM""MMMMMMMMMM""MMMMMMMMMM""MMMMMMMMMM") * ImVec2{1.0, 5.0};
  auto const notificationPadding = ImGui::GetStyle().FramePadding;
  auto mainViewPort = ImGui::GetMainViewport();

  auto &style = fConfig.fStyle == config::Style::kLight ? kImGuiStyles.first : kImGuiStyles.second;
  ImVec2 pos{mainViewPort->WorkSize.x - notificationSize.x - notificationPadding.x, notificationPadding.y};
  pos += iOffset;

  ImGui::PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_Text]);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, style.Colors[ImGuiCol_WindowBg]);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{4.0, 4.0});

  auto notifications = std::move(fNotifications);

  for(auto &notification: notifications)
  {
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(notificationSize);
    if(notification->render())
      pos.y += notificationSize.y + notificationPadding.x;
  }

  ImGui::PopStyleVar();
  ImGui::PopStyleColor(2);

  for(auto &notification: notifications)
  {
    if(notification->isActive())
      fNotifications.emplace_back(std::move(notification));
  }
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
      oErrors->add("Error while reading file %s: %s", iFile.u8string(), what(std::current_exception()));
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
      oErrors->add("Error while saving file %s: %s", iFile.u8string(), what(std::current_exception()));
  }
}

//------------------------------------------------------------------------
// Application::newAboutDialog
//------------------------------------------------------------------------
void Application::newAboutDialog()
{
  newDialog("About")
    .lambda([this]() {
      renderLogoBox();
      if(ImGui::Button("pongasoft.com"))
        fContext->openURL("https://www.pongasoft.com");
    })
    .lambda([this]() { about(); }, true)
    .buttonOk();
}

//------------------------------------------------------------------------
// Application::about
//------------------------------------------------------------------------
void Application::about() const
{
  ImGui::SeparatorText("RE Edit");
  ImGui::Text("Version:      %s", kFullVersion);
  ImGui::Text("Git Version:  %s", kGitVersion);
  ImGui::Text("Git Tag:      %s", kGitTag);
  ImGui::Text("Architecture: %s", kArchiveArchitecture);
  ImGui::Text("re-mock:      %s", kReMockVersion);

  if(fAppContext)
  {
    constexpr auto boolToString = [](bool b) { return b ? "true" : "false"; };

    ImGui::SeparatorText("Rack Extension");

    auto const &info = fAppContext->fPropertyManager->getDeviceInfo();
    ImGui::Text("long_name:                       %s", info.fLongName.c_str());
    ImGui::Text("medium_name:                     %s", info.fMediumName.c_str());
    ImGui::Text("short_name:                      %s", info.fShortName.c_str());
    ImGui::Text("product_id:                      %s", info.fProductId.c_str());
    ImGui::Text("manufacturer:                    %s", info.fManufacturer.c_str());
    ImGui::Text("version_number:                  %s", info.fVersionNumber.c_str());
    ImGui::Text("device_type:                     %s", deviceTypeToString(info.fDeviceType));
    ImGui::Text("device_categories:               %s", re::mock::stl::join_to_string(info.fDeviceCategories).c_str());
    ImGui::Text("supports_patches:                %s", boolToString(info.fSupportPatches));
    ImGui::Text("default_patch:                   %s", info.fDefaultPatch.c_str());
    ImGui::Text("accepts_notes:                   %s", boolToString(info.fAcceptNotes));
    ImGui::Text("auto_create_note_lane:           %s", boolToString(info.fAutoCreateNoteLane));
    ImGui::Text("supports_performance_automation: %s", boolToString(info.fSupportsPerformanceAutomation));
    ImGui::Text("device_height_ru:                %d", info.fDeviceHeightRU);
  }

  static const std::vector<std::tuple<char const *, char const *, char const *>> kLicenses = {
    {"craigsapp/midifile            ", "BSD 2-Clause \"Simplified\" License", "https://github.com/craigsapp/midifile/blob/master/LICENSE.txt"},
    {"bitmask_operators             ", "Boost Software License", "http://www.boost.org/LICENSE_1_0.txt"},
    {"btzy/nativefiledialog-extended", "ZLib license", "https://github.com/btzy/nativefiledialog-extended/blob/master/LICENSE"},
    {"Dear ImGui                    ", "MIT License", "https://github.com/ocornut/imgui/blob/master/LICENSE.txt"},
    {"glfw                          ", "ZLib license", "https://www.glfw.org/license.html"},
    {"libsndfile                    ", "LGPL-2.1 License", "https://github.com/libsndfile/libsndfile/blob/master/COPYING"},
    {"Lua                           ", "MIT License", "https://www.lua.org/license.html"},
    {"lubgr/lua-cmake               ", "MIT License", "https://github.com/lubgr/lua-cmake/blob/master/LICENSE"},
    {"nlohmann/json                 ", "MIT License", "https://github.com/nlohmann/json/blob/develop/LICENSE.MIT"},
    {"nothings/stb                  ", "Public Domain", "https://github.com/nothings/stb/blob/master/docs/why_public_domain.md"},
    {"pongasoft/re-mock             ", "Apache 2.0", "https://github.com/pongasoft/re-mock/blob/master/LICENSE.txt"},
    {"raylib                        ", "ZLIB License", "https://github.com/raysan5/raylib/blob/master/LICENSE"},
    {"raylib-extras/rlImGui         ", "ZLIB License", "https://github.com/raylib-extras/rlImGui/blob/main/LICENSE"},
    {"SpartanJ/efsw                 ", "MIT License", "https://github.com/SpartanJ/efsw/blob/master/LICENSE"},
  };

  if(ImGui::TreeNodeEx("Licenses", ImGuiTreeNodeFlags_Framed))
  {
    for(auto &[project, license, url]: kLicenses)
    {
      ImGui::Text("%s", project);
      ImGui::SameLine();
      ImGui::PushID(project);
      if(ImGui::Button(license))
      {
        fContext->openURL(url);
      }
      ImGui::PopID();
    }
    ImGui::TreePop();
  }
}

//------------------------------------------------------------------------
// Application::newHelpDialog
//------------------------------------------------------------------------
void Application::newHelpDialog()
{
  newDialog("Help")
    .lambda([this]() { help(); })
    .buttonOk();
}

//------------------------------------------------------------------------
// Application::help
//------------------------------------------------------------------------
void Application::help() const
{
  static const std::vector<std::tuple<char const *, char const *, std::vector<char const *>>> kShortcuts = {
    {"CMD =", "Ctrl + =", {"Increment zoom (+10%)"} },
    {"CMD -", "Ctrl + -", {"Decrement zoom (-10%)"} },
    {"CMD 0", "Ctrl + 0", {"Zoom to fit"} },
    {"CMD Z", "Ctrl + Z", {"Undo"} },
    {"CMD Shift Z", "Ctrl + Shift + Z", {"Redo"} },
    {"CMD S", "Ctrl + S", {"Save"} },
    {"CMD Q", "Ctrl + Q", {"Quit"} },
    {"Ctrl + Mouse Click", "Ctrl + Mouse Click", {R"(Increases (resp. decreases) by the grid size when clicking on "+" (resp. "-") for position and size)"} },
    {"Alt", "Alt", { "Disable the grid temporarily while being held",
                     "Display alternate menu entries (ex: \"Select All\" includes hidden widgets)",
                     "Disable filtering (ex: when selecting a property or a graphics)",
                     "Add the dragged widget to the visibility group"} },
    {"Arrows", "Arrows", {"Move the panel"} },
    {"Space Bar + LMB", "Space Bar + LMB", {"Move the panel (when mouse is over the panel)"} },
    {"Mouse Wheel", "Mouse Wheel", {"Zoom in/out (when mouse is over the Panel)"} },
    {"A", "A", {"Toggle Select All/Select None"} },
    {"B", "B", {"Toggle Widget Borders"} },
    {"C", "C", {"Center the panel"} },
    {"F", "F", {"Zoom to fit (one hand shortcut)"} },
    {"Q", "Q", {"Quick View (while being held)"} },
    {"R", "R", {"Toggle Rails (also toggle Panel X-Ray to see the rails)"} },
    {"X", "X", {"Toggle Widget X-Ray"} },
  };

#if WIN32
  static constexpr int kShortcutIndex = 1;
#else
  static constexpr int kShortcutIndex = 0;
#endif

  ImGui::SeparatorText("Keyboard Shortcuts");
  if(ImGui::BeginTable("keyboard_shortcuts", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV))
  {
    ImGui::TableSetupColumn("Shortcut");
    ImGui::TableSetupColumn("Description");
    ImGui::TableHeadersRow();

    for(auto &shortcut: kShortcuts)
    {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(std::get<kShortcutIndex>(shortcut));
      ImGui::TableSetColumnIndex(1);
      auto const &description = std::get<2>(shortcut);
      for(auto const &item: std::get<2>(shortcut))
      {
        ImGui::TextUnformatted(item);
      }
    }

    ImGui::EndTable();
  }

}

}