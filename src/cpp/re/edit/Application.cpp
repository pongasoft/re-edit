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
#include "lua/Device2D.h"
#include "LoggingManager.h"
#include <fstream>
#include <imgui.h>

namespace re::edit {

//------------------------------------------------------------------------
// Application::Application
//------------------------------------------------------------------------
Application::Application(std::shared_ptr<TextureManager> iTextureManager)
{
  fAppContext.fTextureManager = std::move(iTextureManager);
  fAppContext.fUserPreferences = std::make_shared<UserPreferences>();
  fAppContext.fPropertyManager = std::make_shared<PropertyManager>();
  fAppContext.fUndoManager = std::make_shared<UndoManager>();
}

//------------------------------------------------------------------------
// Application::init
//------------------------------------------------------------------------
bool Application::init(std::vector<std::string> iArgs)
{
//  constexpr auto root = "/Volumes/Development/github/pongasoft/re-cva-7";
//  constexpr auto root = "/Volumes/Development/local/com.presteign.Macro-plugin";

  if(iArgs.empty())
  {
    RE_EDIT_LOG_ERROR("You must provide the path to the root folder of the device");
    return false;
  }

  fRoot = iArgs[0];

  setDeviceHeightRU(fAppContext.fPropertyManager->init(fRoot));

  fAppContext.fTextureManager->init(re::mock::fmt::path(fRoot, "GUI2D"));
  fAppContext.fTextureManager->scanDirectory();
  fAppContext.fTextureManager->findTextureKeys([](auto const &) { return true; }); // forces preloading the textures to get their sizes

  fAppContext.initPanels(re::mock::fmt::path(fRoot, "GUI2D", "device_2D.lua"),
                         re::mock::fmt::path(fRoot, "GUI2D", "hdgui_2D.lua"));

  return true;
}

//------------------------------------------------------------------------
// Application::render
//------------------------------------------------------------------------
void Application::render()
{
  auto loggingManager = LoggingManager::instance();

  fAppContext.fCurrentFrame++;
  fAppContext.fPropertyManager->beforeRenderFrame();

  renderMainMenu();

  if(ImGui::Begin("re-edit"))
  {
    fAppContext.render();

    ImGui::PushID("Border");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Border");
    ImGui::SameLine();
    ReGui::RadioButton("None", &fAppContext.fShowBorder, AppContext::ShowBorder::kNone);
    ImGui::SameLine();
    ReGui::RadioButton("Widget", &fAppContext.fShowBorder, AppContext::ShowBorder::kWidget);
    ImGui::SameLine();
    ReGui::RadioButton("Hit Boundaries", &fAppContext.fShowBorder, AppContext::ShowBorder::kHitBoundaries);
    ImGui::PopID();

    ImGui::PushID("Custom Display");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Custom Display");
    ImGui::SameLine();
    ReGui::RadioButton("None", &fAppContext.fShowCustomDisplay, AppContext::ShowCustomDisplay::kNone);
    ImGui::SameLine();
    ReGui::RadioButton("Main", &fAppContext.fShowCustomDisplay, AppContext::ShowCustomDisplay::kMain);
    ImGui::SameLine();
    ReGui::RadioButton("SD Bg", &fAppContext.fShowCustomDisplay, AppContext::ShowCustomDisplay::kBackgroundSD);
    ImGui::SameLine();
    ReGui::RadioButton("HD Bg", &fAppContext.fShowCustomDisplay, AppContext::ShowCustomDisplay::kBackgroundHD);
    ImGui::PopID();

    ImGui::PushID("Sample Drop Zone");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Sample Drop Zone");
    ImGui::SameLine();
    ReGui::RadioButton("None", &fAppContext.fShowSampleDropZone, AppContext::ShowSampleDropZone::kNone);
    ImGui::SameLine();
    ReGui::RadioButton("Fill", &fAppContext.fShowSampleDropZone, AppContext::ShowSampleDropZone::kFill);
    ImGui::PopID();

    ReGui::ToggleButton("Show Panel", "Hide Panel", &fAppContext.fShowPanel);
    ImGui::SameLine();
    ReGui::ToggleButton("Show Panel Widgets", "Hide Panel Widgets", &fAppContext.fShowPanelWidgets);
    ImGui::SameLine();
    ReGui::ToggleButton("Show Widgets", "Hide Widgets", &fAppContext.fShowWidgets);
    ImGui::SameLine();
    ReGui::ToggleButton("Show Properties", "Hide Properties", &fAppContext.fShowProperties);

    ImGui::Separator();

    ImGui::Checkbox("Demo Window", &show_demo_window);
    if(show_demo_window)
      ImGui::ShowDemoWindow(&show_demo_window);

    ImGui::Checkbox(re::mock::fmt::printf("Log [%d]##Log", loggingManager->getLogCount()).c_str(), &loggingManager->getShowLog());
    ImGui::SameLine();
    ImGui::Checkbox("Debug", &loggingManager->getShowDebug());

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
      if(ImGui::MenuItem("Save"))
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
        auto desc = re::mock::fmt::printf("Undo %s", undoAction->fDescription);
        if(fAppContext.fCurrentPanelState && fAppContext.fCurrentPanelState->getType() != undoAction->fPanelType)
          desc = re::mock::fmt::printf("%s (%s)", desc, Panel::toString(undoAction->fPanelType));
        if(ImGui::MenuItem(desc.c_str()))
        {
          fAppContext.undoLastAction();
        }
      }
      else
      {
        ImGui::BeginDisabled();
        ImGui::MenuItem("Undo");
        ImGui::EndDisabled();
      }

      auto const redoAction = fAppContext.fUndoManager->getLastRedoAction();
      if(redoAction)
      {
        auto desc = re::mock::fmt::printf("Redo %s", redoAction->fUndoAction->fDescription);
        if(fAppContext.fCurrentPanelState && fAppContext.fCurrentPanelState->getType() != redoAction->fUndoAction->fPanelType)
          desc = re::mock::fmt::printf("%s (%s)", desc, Panel::toString(redoAction->fUndoAction->fPanelType));
        if(ImGui::MenuItem(desc.c_str()))
        {
          fAppContext.redoLastAction();
        }
      }
      else
      {
        ImGui::BeginDisabled();
        ImGui::MenuItem("Redo");
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
}

//------------------------------------------------------------------------
// Application::saveFile
//------------------------------------------------------------------------
void Application::saveFile(std::string const &iFile, std::string const &iContent) const
{
  std::ofstream f{iFile};
  f << iContent;
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

////------------------------------------------------------------------------
//// Application::Texture
////------------------------------------------------------------------------
//bool Application::Texture(std::string const &iName, int iFrameNumber, ImVec2 const &iRoot, ImVec2 const &iPosition, float zoom)
//{
//  auto texture = fTextureManager->getTexture(iName);
//  if(texture.fFilmStrip->isValid())
//  {
//    ImGui::SetCursorPos({iRoot.x + (iPosition.x * zoom), iRoot.y + (iPosition.y * zoom)});
//
//    auto frameWidth = static_cast<float>(texture.fFilmStrip->frameWidth());
//    auto frameHeight = static_cast<float>(texture.fFilmStrip->frameHeight());
//    auto frameY = frameHeight * iFrameNumber;
//    auto width = static_cast<float>(texture.fFilmStrip->width());
//    auto height = static_cast<float>(texture.fFilmStrip->height());
//
//
//    ImGui::Image(texture.fData,
//                 { frameWidth * zoom, frameHeight * zoom},
//                 ImVec2(0, (frameY) / height),
//                 ImVec2((0 + frameWidth) / width, (frameY + frameHeight) / height)
//                 );
//    return true;
//  }
//  else
//  {
//    ImGui::Text("Failed to load texture %s", texture.fFilmStrip->errorMessage().c_str());
//    return false;
//  }
//}
//
//
////------------------------------------------------------------------------
//// Application::Panel
////------------------------------------------------------------------------
//void Application::Panel()
//{
//  static ImVec2 knobPos{3414, 440};
//  static bool knobSelected{false};
//  static ImVec2 mousePos{};
//
//  ImVec2 pos;
//  if(ImGui::Begin("Front Panel", nullptr, ImGuiWindowFlags_HorizontalScrollbar))
//  {
//    static int frame = 0;
//    ImGui::SliderInt("Frame", &frame, 0, 62);
//    pos = ImGui::GetCursorPos();
//    Texture("/Volumes/Development/github/pongasoft/re-cva-7/GUI2D/Panel_Front.png", 0, pos, {0, 0}, fPanelZoom);
//    auto bottom = ImGui::GetCursorPos();
//    Texture("/Volumes/Development/github/pongasoft/re-cva-7/GUI2D/Knob_17_matte_63frames.png", frame, pos, {knobPos.x, knobPos.y}, fPanelZoom);
//    if(ImGui::IsItemClicked())
//    {
//      knobSelected = true;
//      mousePos = ImGui::GetMousePos();
//    }
//    ImGui::SetCursorPos(bottom);
//
//    if(knobSelected)
//    {
//      if(ImGui::IsMouseReleased(ImGuiMouseButton_Left))
//      {
//        knobSelected = false;
//      }
//      else
//      {
//        auto newMousePos = ImGui::GetMousePos();
//        knobPos.x = knobPos.x + (newMousePos.x - mousePos.x) / fPanelZoom;
//        knobPos.y = knobPos.y + (newMousePos.y - mousePos.y) / fPanelZoom;
//        mousePos = newMousePos;
//      }
//      ImGui::Text("Mouse pos: (%g, %g)", mousePos.x, mousePos.y);
//      auto drag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
//      ImGui::Text("Mouse drag: (%g, %g)", drag.x, drag.y);
//    }
//  }
//  ImGui::End();
//}
//

}