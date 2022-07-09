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
  fBackPanel(Panel::Type::kBack, iTextureManager, fUserPreferences, fPropertyManager)
{
}

//------------------------------------------------------------------------
// Application::init
//------------------------------------------------------------------------
void Application::init()
{
  fDeviceHeightRU = fPropertyManager->init("/Volumes/Development/github/pongasoft/re-cva-7");

  fTextureManager->init("/Volumes/Development/github/pongasoft/re-cva-7/GUI2D");
  fTextureManager->scanDirectory();
  fTextureManager->findTextureKeys([](auto const &) { return true; }); // forces preloading the textures to get their sizes

  initPanels("/Volumes/Development/github/org.pongasoft/re-edit/test/resources/re/edit/lua/re-cva-7-device_2D.lua",
             "/Volumes/Development/github/org.pongasoft/re-edit/test/resources/re/edit/lua/re-cva-7-hdgui_2D.lua");

//  loadFilmStrip("/Volumes/Development/github/pongasoft/re-cva-7/GUI2D/Panel_Front.png", 1);
//  loadFilmStrip("/Volumes/Development/github/pongasoft/re-cva-7/GUI2D/Knob_17_matte_63frames.png", 63);

//  fFrontPanel.fPanel.setBackground(fTextureManager->getTexture("Panel_Front"));
//  {
//    auto knob = Widget::analog_knob();
//    knob->setTexture(fTextureManager->getTexture("Knob_17_matte_63frames"));
//    knob->setPosition({1504, 368});
//    fFrontPanel.fPanel.addWidget(std::move(knob));
//  }
//  {
//    auto knob = Widget::analog_knob();
//    knob->setTexture(fTextureManager->getTexture("Knob_17_matte_63frames"));
//    knob->setPosition({1504, 172});
//    fFrontPanel.fPanel.addWidget(std::move(knob));
//  }
}

//------------------------------------------------------------------------
// Application::initPanels
//------------------------------------------------------------------------
void Application::initPanels(std::string const &iDevice2DFile, std::string const &iHDGui2DFile)
{
  auto d2d = lua::Device2D::fromFile(iDevice2DFile);
  auto hdg = lua::HDGui2D::fromFile(iHDGui2DFile);
  initPanel(d2d->front(), hdg->front(), fFrontPanel.fPanel);
}

//------------------------------------------------------------------------
// Application::initPanel
//------------------------------------------------------------------------
void Application::initPanel(std::shared_ptr<lua::panel_nodes> const &iPanelNodes,
                            std::shared_ptr<lua::jbox_panel> const &iPanel,
                            Panel &oPanel)
{
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

  // TODO handle cable origin

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

      widget->setPosition(node->fPosition);
      widget->setName(node->fName);
    }


    oPanel.addWidget(std::move(widget));
  }

  oPanel.setDeviceHeightRU(fDeviceHeightRU);
}

//------------------------------------------------------------------------
// Application::render
//------------------------------------------------------------------------
void Application::render()
{
  fPropertyManager->beforeRenderFrame();

  ImGui::Begin("re-edit");

  if(ImGui::BeginTabBar("Panels", ImGuiTabBarFlags_None))
  {
    fFrontPanel.render();
    fBackPanel.render();
    ImGui::EndTabBar();
  }

  ImGui::Separator();

  ImGui::Checkbox("Demo Window", &show_demo_window);
  if(show_demo_window)
    ImGui::ShowDemoWindow(&show_demo_window);

  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
              ImGui::GetIO().Framerate);

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