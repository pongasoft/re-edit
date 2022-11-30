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
std::optional<fs::path> inferValidRoot(fs::path const &iPath)
{
  if(!fs::exists(iPath))
    return std::nullopt;

  if(fs::is_directory(iPath))
  {
    if(fs::exists(iPath / "info.lua"))
      return iPath;
  }
  else
  {
    auto filename = iPath.filename().u8string();

    if(filename == "info.lua")
      return iPath.parent_path();

    if(filename == "re_edit.lua" || filename == "motherboard_def.lua" || filename == "realtime_controller.lua")
      return inferValidRoot(iPath.parent_path());

    if(filename == "hdgui_2D.lua" || filename == "device_2D.lua")
    {
      auto GUI2D = iPath.parent_path();
      if(GUI2D.has_parent_path())
        return inferValidRoot(GUI2D.parent_path());
    }
  }

  return std::nullopt;
}

}

//------------------------------------------------------------------------
// Application::parseArgs
//------------------------------------------------------------------------
Application::Config Application::parseArgs(std::vector<std::string> iArgs)
{
  Application::Config c{};

  // TODO: initialize global config from preferences

  if(!iArgs.empty())
  {
    c.fProjectRoot = impl::inferValidRoot(fs::path(iArgs[0]));
  }

  return c;
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
  }
}


//------------------------------------------------------------------------
// Application::init
//------------------------------------------------------------------------
void Application::initAppContext(fs::path const &iRoot, config::Local const &iConfig)
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
// Application::loadProject
//------------------------------------------------------------------------
void Application::loadProject(fs::path const &iRoot)
{
  try
  {
    config::Local c{};
    auto configFile = iRoot / "re-edit.lua";
    if(fs::exists(configFile))
    {
      c = lua::LocalConfigParser::fromFile(configFile);
      c.copyTo(fConfig);
    }
    else
      c.copyFrom(fConfig);

    initAppContext(iRoot, c);

    fFontManager->requestNewFont({"JetBrains Mono Regular", BuiltInFont::kJetBrainsMonoRegular, fConfig.fFontSize});
    fContext->setWindowSize(fConfig.fNativeWindowWidth, fConfig.fNativeWindowHeight);
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
    fAppContext = nullptr;
    if(fState == State::kReLoaded)
      fState = State::kNoReLoaded;
  });
}

//------------------------------------------------------------------------
// Application::setNativeWindowSize
//------------------------------------------------------------------------
void Application::setNativeWindowSize(int iWidth, int iHeight)
{
  fConfig.fNativeWindowWidth = iWidth;
  fConfig.fNativeWindowHeight = iHeight;
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
          abort();
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

//------------------------------------------------------------------------
// Application::renderWelcome
//------------------------------------------------------------------------
void Application::renderWelcome()
{
  static constexpr char const *kWelcomeTitle = "Welcome to re-edit";
  static constexpr float kMinSize = 500.0f;
  static constexpr float kPadding = 20.0f;
  static constexpr ImVec2 kButtonSize{120.0f, 0};
  constexpr ImU32 kWelcomeColorU32 = ReGui::GetColorU32(toFloatColor(100, 100, 100));

  if(hasDialog())
    return;

  if(!ImGui::IsPopupOpen(kWelcomeTitle))
  {
    ImGui::OpenPopup(kWelcomeTitle);
    ReGui::CenterNextWindow();
  }


  ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, {kMinSize, kMinSize});
  ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, {0.5f, 0.5f});
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {kPadding, kPadding});

  if(ImGui::BeginPopupModal(kWelcomeTitle, nullptr, ImGuiWindowFlags_HorizontalScrollbar))
  {
    auto textSizeHeight = ImGui::CalcTextSize("R").y;
    auto computedHeight = 2.0f * textSizeHeight + ImGui::GetStyle().ItemSpacing.y;
    auto logo = fTextureManager->getTexture(BuiltIns::kLogoDark.fKey);
    auto zoom = computedHeight / logo->frameHeight();

    logo->Item({}, getCurrentFontDpiScale() * zoom, 0);

    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::TextUnformatted("re-edit");
    ImGui::Text("%s", kFullVersion);
    ImGui::EndGroup();

    ImGui::SameLine(0, kPadding);

    ImGui::BeginGroup();
    if(ImGui::Button("Open", kButtonSize))
    {
      renderLoadDialogBlocking();
    }
    ImGui::SameLine();
    if(ImGui::Button("Quit", kButtonSize))
    {
      abort();
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    auto icon = fTextureManager->getTexture(BuiltIns::kDeviceType.fKey);
    zoom = textSizeHeight / icon->frameHeight();

    static auto itemSpacing = ImGui::GetStyle().ItemSpacing;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, itemSpacing);

    icon->Item({}, getCurrentFontDpiScale() * zoom, 0);
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::TextUnformatted("CVA - 7 djdjdj");
    ImGui::TextUnformatted("/Volumes/Development/github/pongasoft/re-cva-7");
    ImGui::EndGroup();

    if(ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
      RE_EDIT_LOG_DEBUG("clicked!");
      loadProjectDeferred(fs::path("/Volumes/Development/github/pongasoft/re-cva-7"));
    }

    ImGui::SameLine();
    if(ReGui::ResetButton())
    {
      RE_EDIT_LOG_DEBUG("clear me!");
    }

    ImGui::PopStyleVar(1);

    ImGui::Spacing();

    ImGui::TextUnformatted("No history...");

    ImGui::EndGroup();

    ImGui::SliderFloat("paddingx", &itemSpacing.x, 5.0f, 200.0f);
    ImGui::SliderFloat("paddingy", &itemSpacing.y, 5.0f, 200.0f);

    ImGui::EndPopup();
  }

  ImGui::PopStyleVar(3);

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
    abort();
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
      .button("Yes", [this] { fAppContext->save(); fState = State::kDone; return ReGui::Dialog::Result::kExit; })
      .button("No", [this] { fState = State::kDone; return ReGui::Dialog::Result::kExit; })
      .buttonCancel("Cancel", true);
  }
  else
    fState = State::kDone;
}

//------------------------------------------------------------------------
// Application::saveFile
//------------------------------------------------------------------------
void Application::saveFile(fs::path const &iFile, std::string const &iContent, UserError *oErrors)
{
  try
  {
    // we check if the content has changed as there is no reason to overwrite (which changes the modified time)
    if(fs::exists(iFile))
    {
      try
      {
        const auto size = fs::file_size(iFile);
        std::ifstream originalFile(iFile, std::ios::in | std::ios::binary);
        std::string originalContent(size, '\0');
        originalFile.read(originalContent.data(), static_cast<std::streamsize>(size));
        if(originalContent == iContent)
        {
          // no change => no save
          return;
        }
      }
      catch(...)
      {
        // we cannot read the previous file, but it's fine: we will just overwrite it
        RE_EDIT_LOG_DEBUG("Error while reading file %s: %s", iFile, what(std::current_exception()));
      }
    }

    // if the parent directory does not exist, just create it
    auto dir = iFile.parent_path();
    if(!fs::exists(dir))
      fs::create_directories(dir);

    // we do it in 2 steps since step 1 is the one that is more likely to fail
    // 1. we save in a new file
    auto tmpFile = dir / fmt::printf("%s.re_edit.tmp", iFile.filename());
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
  constexpr auto deviceTypeToString = [](re::mock::DeviceType t) {
    switch(t)
    {
      case mock::DeviceType::kUnknown:
        return "unknown";
      case mock::DeviceType::kInstrument:
        return "instrument";
      case mock::DeviceType::kCreativeFX:
        return "creative_fx";
      case mock::DeviceType::kStudioFX:
        return "studio_fx";
      case mock::DeviceType::kHelper:
        return "helper";
      case mock::DeviceType::kNotePlayer:
        return "note_player";
    }
  };


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

//------------------------------------------------------------------------
// Application::welcome
//------------------------------------------------------------------------
void Application::welcome() const
{
  ImGui::TextUnformatted("welcome text...");
}


}