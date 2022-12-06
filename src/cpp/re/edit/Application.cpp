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
#include "Errors.h"
#include "lua/ConfigParser.h"
#include "LoggingManager.h"
#include <fstream>
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
void Application::executeCatchAllExceptions(F f) noexcept
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
      if(fAppContext)
      {
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
// AppContext::AppContext
//------------------------------------------------------------------------
AppContext &AppContext::GetCurrent()
{
  return Application::GetCurrent().getAppContext();
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
// Application::Application
//------------------------------------------------------------------------
void Application::init()
{
  RE_EDIT_INTERNAL_ASSERT(Application::kCurrent == nullptr, "Only one instance of Application allowed");
  Application::kCurrent = this;

  fTextureManager = fContext->newTextureManager();
  fTextureManager->init(BuiltIns::kGlobalBuiltIns, fs::current_path());
  fFontManager = std::make_shared<FontManager>(fContext->newNativeFontManager());

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
void Application::initAppContext(fs::path const &iRoot, config::Device const &iConfig)
{
  fAppContext = std::make_shared<AppContext>(iRoot);
  fAppContext->fTextureManager = fContext->newTextureManager();
  fAppContext->fUserPreferences = std::make_shared<UserPreferences>();
  fAppContext->fPropertyManager = std::make_shared<PropertyManager>();
  fAppContext->fUndoManager = std::make_shared<UndoManager>();

  fAppContext->init(iConfig);
  fAppContext->initDevice();
  fAppContext->initGUI2D();

  if(!fContext->isHeadless())
    ImGui::LoadIniSettingsFromMemory(iConfig.fImGuiIni.c_str(), iConfig.fImGuiIni.size());

  fState = State::kReLoaded;
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
  try
  {
    config::Device c = fConfig.getDeviceConfigFromHistory(iRoot.u8string());

    fContext->setWindowPositionAndSize(c.fNativeWindowPos, c.fNativeWindowSize);
//    fContext->setWindowTitle(fmt::printf("re-edit - %s", c.fName));

    initAppContext(iRoot, c);

    fContext->setWindowTitle(fmt::printf("re-edit - %s", fAppContext->getConfig().fName));

    savePreferences();
  }
  catch(...)
  {
    fState = State::kNoReLoaded;
    newDialog("Error")
      .preContentMessage(fmt::printf("Error while loading Rack Extension project [%s]", iRoot.u8string()))
      .text(what(std::current_exception()), true)
      .buttonCancel("Ok");
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
void Application::maybeCloseProject()
{
  if(fState == State::kReLoaded)
  {
    if(fAppContext->needsSaving())
    {
      newDialog("Close")
        .postContentMessage("You have unsaved changes, do you want to save them before closing?")
        .button("Yes", [this] { fAppContext->save(); closeProjectDeferred(); return ReGui::Dialog::Result::kContinue; })
        .button("No", [this] { closeProjectDeferred(); return ReGui::Dialog::Result::kContinue; })
        .buttonCancel("Cancel", true);
    }
    else
      closeProjectDeferred();
  }
}

//------------------------------------------------------------------------
// Application::closeProjectDeferred
//------------------------------------------------------------------------
void Application::closeProjectDeferred()
{
  deferNextFrame([this]() {
    savePreferences();
    fAppContext = nullptr;
    if(fState == State::kReLoaded)
    {
      fState = State::kNoReLoaded;
      fContext->setWindowPositionAndSize(std::nullopt, ImVec2{config::kWelcomeWindowWidth,
                                                              config::kWelcomeWindowHeight});
      fContext->setWindowTitle(config::kWelcomeWindowTitle);
    }
  });
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
// Application::newFrame
//------------------------------------------------------------------------
bool Application::newFrame() noexcept
{
  try
  {
    for(auto &action: fNewFrameActions)
      action();
    fNewFrameActions.clear();

    if(fFontManager->hasFontChangeRequest())
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

    if(fAppContext)
      fAppContext->newFrame();
  }
  catch(...)
  {
    newExceptionDialog("Error during newFrame", true, std::current_exception());
    executeCatchAllExceptions([e = std::current_exception()] {
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
  if(hasDialog())
  {
    try
    {
      auto res = renderDialog();
      switch(res)
      {
        case ReGui::Dialog::Result::kContinue:
          // nothing to do... just continue
          break;
        case ReGui::Dialog::Result::kBreak:
          return running();
        case ReGui::Dialog::Result::kExit:
          exit();
          return running();
        default:
          RE_EDIT_FAIL("not reached");
      }
    }
    catch(...)
    {
      newExceptionDialog("Error during dialog rendering", true, std::current_exception());
      executeCatchAllExceptions([e = std::current_exception()] {
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

      default:
        break;
    }
  }
  catch(...)
  {
    newExceptionDialog("Error during rendering", true, std::current_exception());
    executeCatchAllExceptions([e = std::current_exception()] {
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


  ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, {0.5f, 0.5f});
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {padding, padding});

  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowPos({});

  if(ImGui::Begin(config::kWelcomeWindowTitle, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_HorizontalScrollbar))
  {
    auto textSizeHeight = ImGui::CalcTextSize("R").y;

    ReGui::Box(logoModifier, [this, textSizeHeight]() {
      auto logo = fTextureManager->getTexture(BuiltIns::kLogoDark.fKey);
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

        for(auto i = fConfig.fDeviceHistory.rbegin(); i != fConfig.fDeviceHistory.rend(); i++)
        {
          auto const &item = *i;

          ImGui::Spacing();
          ImGui::AlignTextToFramePadding();
          icon->Item({}, {buttonHeight, buttonHeight}, 1.0f, impl::getFrameNumberFromDeviceType(item.fType));
          ImGui::SameLine();
          if(ImGui::Button(fmt::printf("%s\n%s", item.fName, item.fPath).c_str()))
          {
            loadProjectDeferred(fs::path(item.fPath));
          }
        }

        ImGui::PopStyleColor(1);
      }

      ImGui::EndGroup();
    }
  }

  ImGui::End();

  ImGui::PopStyleVar(2);

}

//------------------------------------------------------------------------
// Application::renderAppContext
//------------------------------------------------------------------------
void Application::renderAppContext()
{
  RE_EDIT_INTERNAL_ASSERT(fAppContext != nullptr);

  fAppContext->beforeRenderFrame();
  renderMainMenu();
  fAppContext->renderMainMenu();

  ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

  fAppContext->render();

  auto loggingManager = LoggingManager::instance();

  if(loggingManager->getShowDebug())
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
      if(ImGui::MenuItem("Quit"))
      {
        maybeExit();
      }
      ImGui::EndMenu();
    }
    if(ImGui::BeginMenu("File"))
    {
      if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Open, "Open")))
        renderLoadDialogBlocking();
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
      ImGui::MenuItem("Debug", nullptr, &loggingManager->getShowDebug());
      ImGui::MenuItem(fmt::printf("Log [%d]##Log", loggingManager->getLogCount()).c_str(), nullptr, &loggingManager->getShowLog());
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
      loadProjectDeferred(*newProjectPath);
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
        .breakOnNoAction()
        .preContentMessage(std::move(iMessage))
        .text(what(iException), true);

    if(iSaveButton)
      dialog.button("Save", [this] { fAppContext->save(); return ReGui::Dialog::Result::kExit; }, true);

    dialog.buttonExit()
      .postContentMessage("Note: If you think this is an error in the tool, please report it at https://github.com/pongasoft/re-edit-dev/issues");
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
ReGui::Dialog::Result Application::renderDialog()
{
  if(!fCurrentDialog)
  {
    if(fDialogs.empty())
      return ReGui::Dialog::Result::kContinue;
    fCurrentDialog = std::move(fDialogs[0]);
    fDialogs.erase(fDialogs.begin());
  }

  auto res = fCurrentDialog->render();
  if(!fCurrentDialog->isOpen())
    fCurrentDialog = nullptr;
  return res;
}


//------------------------------------------------------------------------
// Application::maybeExit
//------------------------------------------------------------------------
void Application::maybeExit()
{
  if(!running())
    return;

  if(fAppContext && fAppContext->needsSaving())
  {
    newDialog("Quit")
      .postContentMessage("You have unsaved changes, do you want to save them before quitting?")
      .button("Yes", [this] { fAppContext->save(); return ReGui::Dialog::Result::kExit; })
      .button("No", [] { return ReGui::Dialog::Result::kExit; })
      .buttonCancel("Cancel", true);
  }
  else
    exit();
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