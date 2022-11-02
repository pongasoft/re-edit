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
#include "lua/ReEdit.h"
#include "LoggingManager.h"
#include "nfd.h"
#include <fstream>
#include <imgui.h>
#include <imgui_internal.h>
#include <efsw/efsw.hpp>
#include <version.h>

namespace re::edit {

class UpdateListener : public efsw::FileWatchListener
{
public:
  void handleFileAction( efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename ) override
  {
    switch( action )
    {
      case efsw::Actions::Add:
        std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Added" << std::endl;
        break;
      case efsw::Actions::Delete:
        std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Delete" << std::endl;
        break;
      case efsw::Actions::Modified:
        std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Modified" << std::endl;
        break;
      case efsw::Actions::Moved:
        std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Moved from (" << oldFilename << ")" << std::endl;
        break;
      default:
        std::cout << "Should never happen!" << std::endl;
    }
  }
};

namespace impl {

//------------------------------------------------------------------------
// impl::what
//------------------------------------------------------------------------
std::string what(std::exception_ptr const &p)
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
// impl::executeCatchAllExceptions
//------------------------------------------------------------------------
template<typename F>
bool executeCatchAllExceptions(F f) noexcept
{
  try
  {
    f();
    return true;
  }
  catch(...)
  {
    fprintf(stderr, "ABORT| Unrecoverable exception detected: %s", what(std::current_exception()).c_str());
    return false;
  }
}

}

//------------------------------------------------------------------------
// Application::parseArgs
//------------------------------------------------------------------------
std::optional<lua::Config> Application::parseArgs(std::vector<std::string> iArgs)
{
  if(iArgs.empty())
  {
    RE_EDIT_LOG_ERROR("You must provide the path to the root folder of the device");
    return std::nullopt;
  }

  fRoot = fs::path(iArgs[0]);

  auto configFile = fRoot / "re-edit.lua";
  lua::Config config{};
  if(fs::exists(configFile))
  {
    try
    {
      config = lua::ReEdit::fromFile(configFile)->getConfig();
    }
    catch(re::mock::Exception &e)
    {
      RE_EDIT_LOG_WARNING("Error while reading %s | %s", configFile.c_str(), e.what());
    }
  }

  return config;
}

//------------------------------------------------------------------------
// Application::init
//------------------------------------------------------------------------
bool Application::init(lua::Config const &iConfig,
                       std::shared_ptr<TextureManager> iTextureManager,
                       std::shared_ptr<NativeFontManager> iNativeFontManager)
{
  try
  {
    AppContext::kCurrent = &fAppContext;

    fAppContext.fFontManager = std::make_shared<FontManager>(iNativeFontManager);
    fAppContext.fTextureManager = std::move(iTextureManager);
    fAppContext.fUserPreferences = std::make_shared<UserPreferences>();
    fAppContext.fPropertyManager = std::make_shared<PropertyManager>();
    fAppContext.fUndoManager = std::make_shared<UndoManager>();

    fAppContext.init(iConfig);
    ImGui::LoadIniSettingsFromMemory(iConfig.fImGuiIni.c_str(), iConfig.fImGuiIni.size());

    auto &io = ImGui::GetIO();
    io.IniFilename = nullptr; // don't use imgui.ini file
    io.WantSaveIniSettings = false; // will be "notified" when it changes
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    auto info = fAppContext.initDevice(fRoot);
    fMainWindow.setName(info.fMediumName);

    fAppContext.fTextureManager->init(fRoot / "GUI2D");
    fAppContext.fTextureManager->scanDirectory();

    fAppContext.initPanels(fRoot / "GUI2D" / "device_2D.lua",
                           fRoot / "GUI2D" / "hdgui_2D.lua");

//    static auto fileWatcher = std::make_unique<efsw::FileWatcher>();
//    static auto listener = std::make_unique<UpdateListener>();
//
//    fileWatcher->addWatch(fRoot.string(), listener.get(), true);
//    fileWatcher->watch();
    return true;
  }
  catch(...)
  {
    auto cont = newExceptionDialog("Error during initialization", false, std::current_exception());
    cont &= impl::executeCatchAllExceptions([e = std::current_exception()] {
      RE_EDIT_LOG_ERROR("Unrecoverable exception detected: %s", impl::what(e));
      ImGui::ErrorCheckEndFrameRecover(nullptr);
    });
    return cont;
  }
}

//------------------------------------------------------------------------
// Application::newFrame
//------------------------------------------------------------------------
bool Application::newFrame() noexcept
{
  try
  {
    if(fNewLayoutRequested)
    {
      auto newLayoutRequest = *fNewLayoutRequested;
      fNewLayoutRequested = std::nullopt;
      ImGui::LoadIniSettingsFromMemory(newLayoutRequest.c_str(), newLayoutRequest.size());
    }

    if(fAppContext.fFontManager->hasFontChangeRequest())
    {
      auto oldDpiScale = fAppContext.fFontManager->getCurrentFontDpiScale();
      fAppContext.fFontManager->applyFontChangeRequest();
      auto newDpiScale = fAppContext.fFontManager->getCurrentFontDpiScale();

      if(oldDpiScale != newDpiScale)
      {
        auto scaleFactor = newDpiScale;
        ImGuiStyle newStyle{};
        ImGui::StyleColorsDark(&newStyle);
        newStyle.ScaleAllSizes(scaleFactor);
        ImGui::GetStyle() = newStyle;
      }

      fRecomputeDimensionsRequested = true;
    }

    return true;
  }
  catch(...)
  {
    auto cont = newExceptionDialog("Error during newFrame", true, std::current_exception());
    cont &= impl::executeCatchAllExceptions([e = std::current_exception()] {
      RE_EDIT_LOG_ERROR("Unrecoverable exception detected: %s", impl::what(e));
      ImGui::ErrorCheckEndFrameRecover(nullptr);
    });
    return cont;
  }
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
          return true;
        case ReGui::Dialog::Result::kExit:
          return false;
        default:
          RE_EDIT_FAIL("not reached");
      }
    }
    catch(...)
    {
      auto cont = newExceptionDialog("Error during dialog rendering", true, std::current_exception());
      cont &= impl::executeCatchAllExceptions([e = std::current_exception()] {
        RE_EDIT_LOG_ERROR("Unrecoverable exception detected: %s", impl::what(e));
        ImGui::ErrorCheckEndFrameRecover(nullptr);
      });
      return cont;
    }
  }

  try
  {
    return doRender();
  }
  catch(...)
  {
    auto cont = newExceptionDialog("Error during rendering", true, std::current_exception());
    cont &= impl::executeCatchAllExceptions([e = std::current_exception()] {
      RE_EDIT_LOG_ERROR("Unrecoverable exception detected: %s", impl::what(e));
      ImGui::ErrorCheckEndFrameRecover(nullptr);
    });
    return cont;
  }
}

//------------------------------------------------------------------------
// Application::doRender
//------------------------------------------------------------------------
bool Application::doRender()
{
  if(fRecomputeDimensionsRequested)
  {
    fAppContext.fItemWidth = 40 * ImGui::CalcTextSize("W").x;
    fRecomputeDimensionsRequested = false;
  }

  if(fReloadTexturesRequested)
  {
    fReloadTexturesRequested = false;
    fAppContext.fTextureManager->scanDirectory();
    fAppContext.reloadTextures();
  }

  if(fReloadDeviceRequested)
  {
    fReloadDeviceRequested = false;
    try
    {
      auto info = fAppContext.reloadDevice(fRoot);
      fMainWindow.setName(info.fMediumName);
    }
    catch(...)
    {
      newDialog("Error")
        .preContentMessage("Error while reloading rack extension definition")
        .text(impl::what(std::current_exception()), true)
        .buttonCancel("Ok");
    }
  }

  if(fAppContext.fUndoManager->hasUndoHistory())
    fNeedsSaving = true;

  auto loggingManager = LoggingManager::instance();

  fAppContext.fCurrentFrame++;
  fAppContext.fPropertyManager->beforeRenderFrame();

  renderMainMenu();

  ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

  int flags = fNeedsSaving ?  ImGuiWindowFlags_UnsavedDocument : ImGuiWindowFlags_None;

  if(auto l = fMainWindow.begin(flags))
  {
//    if(ImGui::Button("Bigger"))
//    {
//      kCurrentFont.fSize++;
//      kNewFont = kCurrentFont;
//      fNewFontRequested = true;
//    }
//    ImGui::SameLine();
//    if(ImGui::Button("Smaller"))
//    {
//      kCurrentFont.fSize--;
//      kNewFont = kCurrentFont;
//      fNewFontRequested = true;
//    }
//    ImGui::SameLine();
//    ImGui::Text("%dpx", static_cast<int>(kCurrentFont.fSize));

    fAppContext.renderTabs();

//    auto const &currentFont = fAppContext.fFontManager->getCurrentFont();
//    auto fontSize = static_cast<int>(currentFont.fSize);
//    if(ImGui::InputInt("font_size", &fontSize))
//    {
//      fAppContext.fFontManager->requestNewFont(FontDef{currentFont.fName, currentFont.fSource, static_cast<float>(fontSize)});
//    }

//    static float kScale = 1.0f;
//
//    if(ImGui::SliderFloat("scale", &kScale, 1.0f, 3.0f))
//    {
//      ImGuiStyle newStyle{};
//      newStyle.ScaleAllSizes(kScale);
//      ImGui::GetStyle() = newStyle;
//    }

//    if(ImGui::Button("Fit"))
//    {
//      fMainWindow.requestSizeToFit();
//      fAppContext.fPanelWindow.requestSizeToFit();
//      fAppContext.fPanelWidgetsWindow.requestSizeToFit();
//      fAppContext.fWidgetsWindow.requestSizeToFit();
//      fAppContext.fPropertiesWindow.requestSizeToFit();
//      RE_EDIT_LOG_INFO("dpiScale=%f | ImGui::GetFontSize()=%f | kResetIcon=%f | X=%f",
//                       fAppContext.fFontManager->getCurrentFontDpiScale(), ImGui::GetFontSize(), ImGui::CalcTextSize(ReGui::kResetIcon).x, ImGui::CalcTextSize("X").x);
//    }

//
//    auto fontScale = fAppContext.fFontManager->getFontScale();
//    if(ImGui::SliderFloat("font_scale", &fontScale, 1.0f, 2.0f))
//    {
//      fAppContext.fFontManager->setFontScales(fontScale, fAppContext.fFontManager->getFontDpiScale());
//    }

//    if(ImGui::Button("Error"))
//    {
//      RE_EDIT_INTERNAL_ASSERT(false, "do you see me?");
//    }

    ImGui::PushID("Rendering");

    ImGui::PushID("Widget");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Widget          ");
    ImGui::SameLine();
    ReGui::TextRadioButton("None  ", &fAppContext.fWidgetRendering, AppContext::EWidgetRendering::kNone);
    ImGui::SameLine();
    ReGui::TextRadioButton("Normal", &fAppContext.fWidgetRendering, AppContext::EWidgetRendering::kNormal);
    ImGui::SameLine();
    ReGui::TextRadioButton("X-Ray ", &fAppContext.fWidgetRendering, AppContext::EWidgetRendering::kXRay);
    ImGui::PopID();

    ImGui::PushID("Border");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Border          ");
    ImGui::SameLine();
    ReGui::TextRadioButton("None  ", &fAppContext.fBorderRendering, AppContext::EBorderRendering::kNone);
    ImGui::SameLine();
    ReGui::TextRadioButton("Normal", &fAppContext.fBorderRendering, AppContext::EBorderRendering::kNormal);
    ImGui::SameLine();
    ReGui::TextRadioButton("Hit B.", &fAppContext.fBorderRendering, AppContext::EBorderRendering::kHitBoundaries);
    ImGui::PopID();

    ImGui::PushID("SizeOnly");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("No Graphics     ");
    ImGui::SameLine();
    ReGui::TextRadioButton("None  ", &fAppContext.fNoGraphicsRendering, AppContext::ENoGraphicsRendering::kNone);
    ImGui::SameLine();
    ReGui::TextRadioButton("Border", &fAppContext.fNoGraphicsRendering, AppContext::ENoGraphicsRendering::kBorder);
    ImGui::SameLine();
    ReGui::TextRadioButton("Fill  ", &fAppContext.fNoGraphicsRendering, AppContext::ENoGraphicsRendering::kFill);
//    ImGui::SameLine();
//    auto noTextureColor = ReGui::GetColorImVec4(fAppContext.fUserPreferences->fWidgetNoGraphicsColor);
//    if(ImGui::ColorEdit4("No Graphics", &noTextureColor.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
//    {
//      fAppContext.fUserPreferences->fWidgetNoGraphicsColor = ReGui::GetColorU32(noTextureColor);
//      noTextureColor.w *= 0.4;
//      fAppContext.fUserPreferences->fWidgetNoGraphicsXRayColor = ReGui::GetColorU32(noTextureColor);
//    }
    ImGui::PopID();

    ImGui::PushID("Custom Display");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Custom Display  ");
    ImGui::SameLine();
    ReGui::TextRadioButton("None  ", &fAppContext.fCustomDisplayRendering, AppContext::ECustomDisplayRendering::kNone);
    ImGui::SameLine();
    ReGui::TextRadioButton("Main  ", &fAppContext.fCustomDisplayRendering, AppContext::ECustomDisplayRendering::kMain);
    ImGui::SameLine();
    ReGui::TextRadioButton("SD Bg.", &fAppContext.fCustomDisplayRendering, AppContext::ECustomDisplayRendering::kBackgroundSD);
    ImGui::SameLine();
    ReGui::TextRadioButton("HD Bg.", &fAppContext.fCustomDisplayRendering, AppContext::ECustomDisplayRendering::kBackgroundHD);
    ImGui::PopID();

    ImGui::PushID("Sample Drop Zone");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Sample Drop Zone");
    ImGui::SameLine();
    ReGui::TextRadioButton("None  ", &fAppContext.fSampleDropZoneRendering, AppContext::ESampleDropZoneRendering::kNone);
    ImGui::SameLine();
    ReGui::TextRadioButton("Fill  ", &fAppContext.fSampleDropZoneRendering, AppContext::ESampleDropZoneRendering::kFill);
    ImGui::PopID();

    ImGui::PopID();
    ImGui::Separator();

    if(fShowDemoWindow)
      ImGui::ShowDemoWindow(&fShowDemoWindow);
    if(fShowMetricsWindow)
      ImGui::ShowMetricsWindow(&fShowMetricsWindow);

//  if(ImGui::Button("Fake log"))
//  {
//    loggingManager->logInfo("Info 1");
//    loggingManager->logWarning("Warning 2");
//    loggingManager->logError("This is a long error message... %d", 89);
//  }

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);

    if(loggingManager->getShowDebug())
    {
      loggingManager->debug("Undo", "History[%d]", fAppContext.fUndoManager->getUndoHistory().size());
      loggingManager->debug("Redo", "History[%d]", fAppContext.fUndoManager->getRedoHistory().size());
    }

    loggingManager->render();
  }

  fAppContext.render();

  fAppContext.fPropertyManager->afterRenderFrame();

  return true;
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
      ImGui::EndMenu();
    }
    if(ImGui::BeginMenu("File"))
    {
//      if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Open, "Load")))
//      {
//        nfdchar_t *outPath;
//        nfdresult_t result = NFD_PickFolder(&outPath, nullptr);
//        if(result == NFD_OKAY)
//        {
//          RE_EDIT_LOG_INFO("Success %s", outPath);
//          NFD_FreePath(outPath);
//        }
//        else if(result == NFD_CANCEL)
//        {
//          RE_EDIT_LOG_INFO("Cancel");
//        }
//        else
//        {
//          RE_EDIT_LOG_ERROR("Error: %s\n", NFD_GetError());
//        }
//      }
      if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Save, "Save")))
      {
        newDialog("Save")
          .preContentMessage("!!! Warning !!!")
          .text("This is an experimental build. Saving will override hdgui_2d.lua and device_2d.lua\nAre you sure you want to proceed?")
          .button("Ok", [this] { save(); return ReGui::Dialog::Result::kContinue; })
          .buttonCancel("Cancel", true)
          ;
      }
      ImGui::Separator();
      if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_RescanImages, "Rescan images")))
      {
        fReloadTexturesRequested = true;
      }
      if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_ReloadMotherboard, "Reload motherboard")))
      {
        fReloadDeviceRequested = true;
      }

      ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Edit"))
    {
      // Undo
      {
        auto const undoAction = fAppContext.fUndoManager->getLastUndoAction();
        if(undoAction)
        {
          fAppContext.resetUndoMergeKey();
          auto desc = re::mock::fmt::printf(ReGui_Prefix(ReGui_Icon_Undo, "Undo %s"), undoAction->fDescription);
          if(fAppContext.fCurrentPanelState && fAppContext.fCurrentPanelState->getType() != undoAction->fPanelType)
          {
            if(undoAction->fPanelType == PanelType::kUnknown)
              RE_EDIT_LOG_WARNING("unknown panel type for %s", undoAction->fDescription);
            else
              desc = re::mock::fmt::printf("%s (%s)", desc, Panel::toString(undoAction->fPanelType));
          }
          if(ImGui::MenuItem(desc.c_str()))
          {
            fAppContext.undoLastAction();
          }
        }
        else
        {
          ImGui::BeginDisabled();
          ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Undo, "Undo"));
          ImGui::EndDisabled();
        }
      }

      // Redo
      {
        auto const redoAction = fAppContext.fUndoManager->getLastRedoAction();
        if(redoAction)
        {
          auto const undoAction = redoAction->fUndoAction;
          auto desc = re::mock::fmt::printf(ReGui_Prefix(ReGui_Icon_Redo, "Redo %s"), undoAction->fDescription);
          if(fAppContext.fCurrentPanelState && fAppContext.fCurrentPanelState->getType() != undoAction->fPanelType)
          {
            if(undoAction->fPanelType == PanelType::kUnknown)
              RE_EDIT_LOG_WARNING("unknown panel type for %s", undoAction->fDescription);
            else
              desc = re::mock::fmt::printf("%s (%s)", desc, Panel::toString(undoAction->fPanelType));
          }
          if(ImGui::MenuItem(desc.c_str()))
          {
            fAppContext.redoLastAction();
          }
        }
        else
        {
          ImGui::BeginDisabled();
          ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Redo, "Redo"));
          ImGui::EndDisabled();
        }
      }

      ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Window"))
    {
      fAppContext.fPanelWindow.menuItem();
      fAppContext.fPanelWidgetsWindow.menuItem();
      fAppContext.fWidgetsWindow.menuItem();
      fAppContext.fPropertiesWindow.menuItem();
      ImGui::Separator();
      if(ImGui::MenuItem("Horizontal Layout"))
        fNewLayoutRequested = lua::kDefaultHorizontalLayout;
      if(ImGui::MenuItem("Vertical Layout"))
        fNewLayoutRequested = lua::kDefaultVerticalLayout;
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
//    ImGui::TextUnformatted(fAppContext.fPropertyManager->getDeviceInfo().fMediumName.c_str());
//    ImGui::TextUnformatted(fAppContext.fPropertyManager->getDeviceInfo().fVersionNumber.c_str());
    ImGui::EndMainMenuBar();
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
bool Application::newExceptionDialog(std::string iMessage, bool iSaveButton, std::exception_ptr const &iException)
{
  if(!fHasException)
  {
    fHasException = true;
    auto &dialog =
      newDialog("Error", true)
        .breakOnNoAction()
        .preContentMessage(std::move(iMessage))
        .text(impl::what(iException), true);

    if(iSaveButton)
      dialog.button("Save", [this] { save(); return ReGui::Dialog::Result::kExit; }, true);

    dialog.buttonExit()
      .postContentMessage("Note: If you think this is an error in the tool, please report it at https://github.com/pongasoft/re-edit-dev/issues");

    return true;
  }
  else
  {
    RE_EDIT_LOG_ERROR("Error while handling error... aborting | %s", impl::what(iException));
    return false;
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
// Application::save
//------------------------------------------------------------------------
void Application::save()
{
  saveFile(fRoot / "GUI2D" / "device_2D.lua", device2D());
  saveFile(fRoot / "GUI2D" / "hdgui_2D.lua", hdgui2D());
  saveConfig();
//  fAppContext.fUndoManager->clear();
  fNeedsSaving = false;
  ImGui::GetIO().WantSaveIniSettings = false;
}

//------------------------------------------------------------------------
// Application::hdgui2D
//------------------------------------------------------------------------
std::string Application::hdgui2D()
{
  std::stringstream s{};
  s << "format_version = \"2.0\"\n\n";
  s << fAppContext.fFrontPanel->fPanel.hdgui2D(fAppContext);
  s << "\n";
  s << fAppContext.fBackPanel->fPanel.hdgui2D(fAppContext);
  s << "\n";
  s << fAppContext.fFoldedFrontPanel->fPanel.hdgui2D(fAppContext);
  s << "\n";
  s << fAppContext.fFoldedBackPanel->fPanel.hdgui2D(fAppContext);
  return s.str();
}

//------------------------------------------------------------------------
// Application::device2D
//------------------------------------------------------------------------
std::string Application::device2D() const
{
  std::stringstream s{};
  s << "format_version = \"2.0\"\n\n";
  s << fAppContext.fFrontPanel->fPanel.device2D();
  s << "\n";
  s << fAppContext.fBackPanel->fPanel.device2D();
  s << "\n";
  s << fAppContext.fFoldedFrontPanel->fPanel.device2D();
  s << "\n";
  s << fAppContext.fFoldedBackPanel->fPanel.device2D();
  return s.str();
}

//------------------------------------------------------------------------
// Application::saveFile
//------------------------------------------------------------------------
void Application::saveFile(fs::path const &iFile, std::string const &iContent)
{
  try
  {
    // we do it in 2 steps since step 1 is the one that is more likely to fail
    // 1. we save in a new file
    auto tmpFile = iFile.parent_path() / fmt::printf("%s.re_edit.tmp", iFile.filename());
    std::ofstream f{tmpFile};
    f << iContent;
    f.close();
    // 2. we rename
    fs::rename(tmpFile, iFile);
  }
  catch(...)
  {
    RE_EDIT_LOG_ERROR("Error while saving file %s: %s", iFile, impl::what(std::current_exception()));
  }
}

//------------------------------------------------------------------------
// Application::saveConfig
//------------------------------------------------------------------------
void Application::saveConfig()
{
  std::stringstream s{};

  s << "format_version = \"1.0\"\n\n";
  s << "re_edit = {}\n";

  s << fAppContext.getLuaConfig() << "\n";

  s << fmt::printf("re_edit[\"imgui.ini\"] = [==[\n%s\n]==]\n", ImGui::SaveIniSettingsToMemory());

  saveFile(fRoot / "re-edit.lua", s.str());
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
    auto const &info = fAppContext.fPropertyManager->getDeviceInfo();
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