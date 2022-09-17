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
#include <fstream>
#include <imgui.h>

namespace re::edit {

//------------------------------------------------------------------------
// Application::parseArgs
//------------------------------------------------------------------------
bool Application::parseArgs(std::vector<std::string> iArgs)
{
  if(iArgs.empty())
  {
    RE_EDIT_LOG_ERROR("You must provide the path to the root folder of the device");
    return false;
  }

  fRoot = std::string(iArgs[0]);

  return true;
}

//------------------------------------------------------------------------
// Application::init
//------------------------------------------------------------------------
bool Application::init(std::shared_ptr<TextureManager> iTextureManager)
{
  fAppContext.fFontManager = std::make_shared<FontManager>(iTextureManager);
  fAppContext.fTextureManager = std::move(iTextureManager);
  fAppContext.fUserPreferences = std::make_shared<UserPreferences>();
  fAppContext.fPropertyManager = std::make_shared<PropertyManager>();
  fAppContext.fUndoManager = std::make_shared<UndoManager>();

  auto configFile = re::mock::fmt::path(fRoot, "re-edit.lua");
  lua::Config config{};
  if(fileExists(configFile))
  {
    try
    {
      config = lua::ReEdit::fromFile(configFile)->getConfig();
    }
    catch(re::mock::Exception &e)
    {
      RE_EDIT_LOG_WARNING("Error while reading %s | %s", configFile, e.what());
    }
  }

  fAppContext.init(config);
  ImGui::LoadIniSettingsFromMemory(config.fImGuiIni.c_str(), config.fImGuiIni.size());

  auto &io = ImGui::GetIO();
  io.IniFilename = nullptr; // don't use imgui.ini file
  io.WantSaveIniSettings = false; // will be "notified" when it changes
  io.ConfigWindowsMoveFromTitleBarOnly = true;

  setDeviceHeightRU(fAppContext.fPropertyManager->init(fRoot));

  fAppContext.fTextureManager->init(re::mock::fmt::path(fRoot, "GUI2D"));
  fAppContext.fTextureManager->scanDirectory();
  fAppContext.fTextureManager->findTextureKeys([](auto const &) { return true; }); // forces preloading the textures to get their sizes

  fAppContext.initPanels(re::mock::fmt::path(fRoot, "GUI2D", "device_2D.lua"),
                         re::mock::fmt::path(fRoot, "GUI2D", "hdgui_2D.lua"));

  return true;
}

//------------------------------------------------------------------------
// Application::newFrame
//------------------------------------------------------------------------
void Application::newFrame()
{
  if(fAppContext.fFontManager->hasNewFontRequest())
    fAppContext.fFontManager->applyNewFontRequest();
}

//------------------------------------------------------------------------
// Application::render
//------------------------------------------------------------------------
void Application::render()
{
  if(fAppContext.fUndoManager->hasUndoHistory())
    fNeedsSaving = true;

  if(ImGui::GetIO().WantSaveIniSettings)
    fNeedsSaving = true;

  auto loggingManager = LoggingManager::instance();

  fAppContext.fCurrentFrame++;
  fAppContext.fPropertyManager->beforeRenderFrame();

  renderMainMenu();

  auto flags = fNeedsSaving ?  ImGuiWindowFlags_UnsavedDocument : ImGuiWindowFlags_None;

  if(ImGui::Begin("re-edit", nullptr, flags))
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

    fAppContext.render();

    ImGui::Text("Rendering");
    ImGui::PushID("Rendering");
    ImGui::Indent();
    
    ImGui::PushID("Widget");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Widget");
    ImGui::SameLine();
    ReGui::RadioButton("None", &fAppContext.fWidgetRendering, AppContext::EWidgetRendering::kNone);
    ImGui::SameLine();
    ReGui::RadioButton("Normal", &fAppContext.fWidgetRendering, AppContext::EWidgetRendering::kNormal);
    ImGui::SameLine();
    ReGui::RadioButton("X-Ray", &fAppContext.fWidgetRendering, AppContext::EWidgetRendering::kXRay);
    ImGui::PopID();

    ImGui::PushID("Border");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Border");
    ImGui::SameLine();
    ReGui::RadioButton("None", &fAppContext.fBorderRendering, AppContext::EBorderRendering::kNone);
    ImGui::SameLine();
    ReGui::RadioButton("Normal", &fAppContext.fBorderRendering, AppContext::EBorderRendering::kNormal);
    ImGui::SameLine();
    ReGui::RadioButton("Hit Boundaries", &fAppContext.fBorderRendering, AppContext::EBorderRendering::kHitBoundaries);
    ImGui::PopID();


    ImGui::PushID("Custom Display");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Custom Display");
    ImGui::SameLine();
    ReGui::RadioButton("None", &fAppContext.fCustomDisplayRendering, AppContext::ECustomDisplayRendering::kNone);
    ImGui::SameLine();
    ReGui::RadioButton("Main", &fAppContext.fCustomDisplayRendering, AppContext::ECustomDisplayRendering::kMain);
    ImGui::SameLine();
    ReGui::RadioButton("SD Bg", &fAppContext.fCustomDisplayRendering, AppContext::ECustomDisplayRendering::kBackgroundSD);
    ImGui::SameLine();
    ReGui::RadioButton("HD Bg", &fAppContext.fCustomDisplayRendering, AppContext::ECustomDisplayRendering::kBackgroundHD);
    ImGui::PopID();

    ImGui::PushID("Sample Drop Zone");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Sample Drop Zone");
    ImGui::SameLine();
    ReGui::RadioButton("None", &fAppContext.fSampleDropZoneRendering, AppContext::ESampleDropZoneRendering::kNone);
    ImGui::SameLine();
    ReGui::RadioButton("Fill", &fAppContext.fSampleDropZoneRendering, AppContext::ESampleDropZoneRendering::kFill);
    ImGui::PopID();

    ImGui::Unindent();
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

  ImGui::End();

  if(fSavingRequested)
    renderSavePopup();

  fAppContext.fPropertyManager->afterRenderFrame();
}

//------------------------------------------------------------------------
// Application::setDeviceHeightRU
//------------------------------------------------------------------------
void Application::setDeviceHeightRU(int iDeviceHeightRU)
{
  fDeviceHeightRU = iDeviceHeightRU;
  fAppContext.fFrontPanel->fPanel.setDeviceHeightRU(iDeviceHeightRU);
  fAppContext.fFoldedFrontPanel->fPanel.setDeviceHeightRU(iDeviceHeightRU);
  fAppContext.fBackPanel->fPanel.setDeviceHeightRU(iDeviceHeightRU);
  fAppContext.fFoldedBackPanel->fPanel.setDeviceHeightRU(iDeviceHeightRU);
}

static constexpr auto kSavePopupWindow = "Save | Warning";

//------------------------------------------------------------------------
// Application::renderMainMenu
//------------------------------------------------------------------------
void Application::renderMainMenu()
{
  auto savePopupId = ImGui::GetID(kSavePopupWindow);

  if(ImGui::BeginMainMenuBar())
  {
    if(ImGui::BeginMenu("File"))
    {
      if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Save, "Save")))
      {
        ImGui::OpenPopup(savePopupId);
        fSavingRequested = true;
      }
      ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Edit"))
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

      auto const redoAction = fAppContext.fUndoManager->getLastRedoAction();
      if(redoAction)
      {
        auto desc = re::mock::fmt::printf(ReGui_Prefix(ReGui_Icon_Redo, "Redo %s"), redoAction->fUndoAction->fDescription);
        if(fAppContext.fCurrentPanelState && fAppContext.fCurrentPanelState->getType() != redoAction->fUndoAction->fPanelType)
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

      ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Window"))
    {
      ImGui::MenuItem("Panel", nullptr, &fAppContext.fShowPanel);
      ImGui::MenuItem("Panel Widgets", nullptr, &fAppContext.fShowPanelWidgets);
      ImGui::MenuItem("Widgets", nullptr, &fAppContext.fShowWidgets);
      ImGui::MenuItem("Properties", nullptr, &fAppContext.fShowProperties);
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
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}

//------------------------------------------------------------------------
// Application::renderSavePopup
//------------------------------------------------------------------------
void Application::renderSavePopup()
{
  if(ImGui::BeginPopupModal(kSavePopupWindow, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
  {
    ImGui::Text("!!! Warning !!!");
    ImGui::Text("This is an experimental build. Saving will override hdgui_2d.lua and device_2d.lua");
    ImGui::Text("Are you sure you want to proceed?");
    if(ImGui::Button("OK", ImVec2(120, 0)))
    {
      save();
      fSavingRequested = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if(ImGui::Button("Cancel", ImVec2(120, 0)))
    {
      fSavingRequested = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SetItemDefaultFocus();
    ImGui::EndPopup();
  }
}

//------------------------------------------------------------------------
// Application::save
//------------------------------------------------------------------------
void Application::save()
{
  saveFile(re::mock::fmt::path(fRoot, "GUI2D", "device_2D.lua"), device2D());
  saveFile(re::mock::fmt::path(fRoot, "GUI2D", "hdgui_2D.lua"), hdgui2D());
  saveConfig();
  fAppContext.fUndoManager->clear();
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
void Application::saveFile(std::string const &iFile, std::string const &iContent)
{
  std::ofstream f{iFile};
  f << iContent;
}

//------------------------------------------------------------------------
// Application::fileExists
//------------------------------------------------------------------------
bool Application::fileExists(std::string const &iFile)
{
  std::ifstream f{iFile};
  return f.good();
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

  saveFile(re::mock::fmt::path(fRoot, "re-edit.lua"), s.str());
}

}