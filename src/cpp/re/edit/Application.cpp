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
#include <imgui.h>

namespace re::edit {

//------------------------------------------------------------------------
// Application::Application
//------------------------------------------------------------------------
Application::Application(std::shared_ptr<TextureManager> const &iTextureManager) :
  fTextureManager{iTextureManager},
  fUserPreferences{std::make_shared<UserPreferences>()},
  fFrontPanel(Panel::Type::kFront, iTextureManager, fUserPreferences),
  fBackPanel(Panel::Type::kBack, iTextureManager, fUserPreferences)
{
}

//------------------------------------------------------------------------
// Application::init
//------------------------------------------------------------------------
void Application::init()
{
  fTextureManager->init("/Volumes/Development/github/pongasoft/re-cva-7/GUI2D");
  fTextureManager->scanDirectory();

//  loadFilmStrip("/Volumes/Development/github/pongasoft/re-cva-7/GUI2D/Panel_Front.png", 1);
//  loadFilmStrip("/Volumes/Development/github/pongasoft/re-cva-7/GUI2D/Knob_17_matte_63frames.png", 63);

  fFrontPanel.fPanel.setBackground(
    fTextureManager->getTexture("Panel_Front"));
  {
    auto knob = Widget::analog_knob();
    knob->setTexture(fTextureManager->getTexture("Knob_17_matte_63frames"));
    knob->setPosition({1504, 368});
    fFrontPanel.fPanel.addWidget(std::move(knob));
  }
  {
    auto knob = Widget::analog_knob();
    knob->setTexture(fTextureManager->getTexture("Knob_17_matte_63frames"));
    knob->setPosition({1504, 172});
    fFrontPanel.fPanel.addWidget(std::move(knob));
  }
}

//------------------------------------------------------------------------
// Application::render
//------------------------------------------------------------------------
void Application::render()
{
  ImGui::Begin("re-edit");

  if(ImGui::BeginTabBar("Panels", ImGuiTabBarFlags_None))
  {
    if(ImGui::BeginTabItem("Front Panel"))
    {
      ImGui::SliderFloat("zoom", &fFrontPanel.fDrawContext.fZoom, 0.25f, 1.5f);
      ImGui::Checkbox("Show Widget Border", &fFrontPanel.fDrawContext.fShowWidgetBorder);
      if(ImGui::Begin("Front Panel", nullptr, ImGuiWindowFlags_HorizontalScrollbar))
      {
        fFrontPanel.fPanel.draw(fFrontPanel.fDrawContext);
        fFrontPanel.fPanel.editView(*this);
      }
      ImGui::End();
      ImGui::EndTabItem();
    }

    if(ImGui::BeginTabItem("Back Panel"))
    {
      ImGui::SliderFloat("zoom", &fBackPanel.fDrawContext.fZoom, 0.25f, 1.5f);
      ImGui::Checkbox("Show Widget Border", &fBackPanel.fDrawContext.fShowWidgetBorder);
      if(ImGui::Begin("Back Panel", nullptr, ImGuiWindowFlags_HorizontalScrollbar))
      {
        fBackPanel.fPanel.draw(fBackPanel.fDrawContext);
        fBackPanel.fPanel.editView(*this);
      }
      ImGui::End();
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }

  ImGui::Separator();

  ImGui::Checkbox("Demo Window", &show_demo_window);
  if(show_demo_window)
    ImGui::ShowDemoWindow(&show_demo_window);

  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
              ImGui::GetIO().Framerate);

  ImGui::End();
}

//------------------------------------------------------------------------
// Application::getStepCount
//------------------------------------------------------------------------
int Application::getStepCount(std::string const &iPropertyPath) const
{
  if(iPropertyPath == "/custom_properties/c1")
    return 5;
  else
    return 0;
}

//------------------------------------------------------------------------
// Application::getTextureKeys
//------------------------------------------------------------------------
std::vector<std::string> const &Application::getTextureKeys() const
{
  return fTextureManager->getTextureKeys();
}

//------------------------------------------------------------------------
// Application::getTexture
//------------------------------------------------------------------------
std::shared_ptr<Texture> Application::getTexture(std::string const &iKey) const
{
  return fTextureManager->getTexture(iKey);
}

//------------------------------------------------------------------------
// Application::doGetPropertyNames
//------------------------------------------------------------------------
std::vector<std::string> Application::doGetPropertyNames(EditContext::PropertyKind iPropertyKind) const
{
  if(iPropertyKind == EditContext::PropertyKind::kAny)
    return {"/custom_properties/c1", "/custom_properties/c2",
            "/custom_properties/this/is/a/very/long/name/will/it/fit"};
  else
    return {"/custom_properties/c1"};
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

//------------------------------------------------------------------------
// Application::PanelState
//------------------------------------------------------------------------
Application::PanelState::PanelState(Panel::Type iPanelType,
                                    std::shared_ptr<TextureManager> iTextureManager,
                                    std::shared_ptr<UserPreferences> iUserPreferences) :
  fPanel(iPanelType),
  fDrawContext(std::move(iTextureManager), std::move(iUserPreferences))
{
}

}