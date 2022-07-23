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
#include <imgui.h>

namespace re::edit {

//------------------------------------------------------------------------
// Application::Application
//------------------------------------------------------------------------
Application::Application(std::shared_ptr<TextureManager> const &iTextureManager) :
  fTextureManager{iTextureManager},
  fUserPreferences{std::make_shared<UserPreferences>()},
  fPropertyManager{std::make_shared<PropertyManager>()},
  fFrontPanel(Panel::Type::kFront, iTextureManager, fUserPreferences, fPropertyManager),
  fFoldedFrontPanel(Panel::Type::kFoldedFront, iTextureManager, fUserPreferences, fPropertyManager),
  fBackPanel(Panel::Type::kBack, iTextureManager, fUserPreferences, fPropertyManager),
  fFoldedBackPanel(Panel::Type::kFoldedBack, iTextureManager, fUserPreferences, fPropertyManager)
{
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

  auto root = iArgs[0];

  fDeviceHeightRU = fPropertyManager->init(root);

  fTextureManager->init(re::mock::fmt::path(root, "GUI2D"));
  fTextureManager->scanDirectory();
  fTextureManager->findTextureKeys([](auto const &) { return true; }); // forces preloading the textures to get their sizes

  initPanels(re::mock::fmt::path(root, "GUI2D", "device_2D.lua"),
             re::mock::fmt::path(root, "GUI2D", "hdgui_2D.lua"));

  return true;
}

//------------------------------------------------------------------------
// Application::initPanels
//------------------------------------------------------------------------
void Application::initPanels(std::string const &iDevice2DFile, std::string const &iHDGui2DFile)
{
  auto d2d = lua::Device2D::fromFile(iDevice2DFile);
  auto hdg = lua::HDGui2D::fromFile(iHDGui2DFile);
  initPanel(d2d->front(), hdg->front(), fFrontPanel.fPanel);
  initPanel(d2d->folded_front(), hdg->folded_front(), fFoldedFrontPanel.fPanel);
  initPanel(d2d->back(), hdg->back(), fBackPanel.fPanel);
  initPanel(d2d->folded_back(), hdg->folded_back(), fFoldedBackPanel.fPanel);
}

//------------------------------------------------------------------------
// Application::initPanel
//------------------------------------------------------------------------
void Application::initPanel(std::shared_ptr<lua::panel_nodes> const &iPanelNodes,
                            std::shared_ptr<lua::jbox_panel> const &iPanel,
                            Panel &oPanel)
{
  oPanel.setDeviceHeightRU(fDeviceHeightRU);

  if(iPanelNodes == nullptr || iPanel == nullptr)
    return;

  // handle background
  {
    auto node = iPanelNodes->findNodeByName(iPanel->fGraphicsNode);
    if(node && node->hasKey())
    {
      auto background = fTextureManager->findTexture(node->getKey());
      if(background)
        oPanel.setBackground(std::move(background));
      else
        RE_EDIT_LOG_WARNING ("Could not locate background texture [%s] for panel [%s]", iPanel->fGraphicsNode,
                             oPanel.getName());
    }
  }

  // Cable origin
  {
    if(iPanel->fCableOrigin)
    {
      auto node = iPanelNodes->findNodeByName(*iPanel->fCableOrigin);
      if(node)
        oPanel.setCableOrigin(node->fPosition);
      else
        RE_EDIT_LOG_WARNING ("Could not locate cable origin for panel [%s]", *iPanel->fCableOrigin,
                             oPanel.getName());
    }

  }

  for(auto const &w: iPanel->fWidgets)
  {
    auto widget = w->fWidget;

    auto node = iPanelNodes->findNodeByName(w->fGraphics.fNode);
    if(node)
    {
      if(node->hasKey())
      {
        auto graphics = fTextureManager->findTexture(node->getKey());
        if(graphics)
        {
          if(graphics->numFrames() != node->fNumFrames)
            graphics->getFilmStrip()->overrideNumFrames(node->fNumFrames);
          widget->setTexture(std::move(graphics));
        }
      }

      if(node->hasSize())
        widget->setSize(node->getSize());

      if(w->fGraphics.fHitBoundaries)
        widget->setHitBoundaries(*w->fGraphics.fHitBoundaries);

      widget->setPosition(node->fPosition);
      widget->setName(node->fName);
    }


    oPanel.addWidget(std::move(widget));
  }
}

//------------------------------------------------------------------------
// Application::render
//------------------------------------------------------------------------
void Application::render()
{
  auto loggingManager = LoggingManager::instance();

  fPropertyManager->beforeRenderFrame();

  ImGui::Begin("re-edit");

  if(ImGui::BeginTabBar("Panels", ImGuiTabBarFlags_None))
  {
    fFrontPanel.render();
    fBackPanel.render();
    fFoldedFrontPanel.render();
    fFoldedBackPanel.render();
    ImGui::EndTabBar();
  }

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

  loggingManager->render();

  ImGui::End();

  fPropertyManager->afterRenderFrame();
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