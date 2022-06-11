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
#include <imgui.h>

namespace re::edit {

//------------------------------------------------------------------------
// Application::Application
//------------------------------------------------------------------------
Application::Application(std::shared_ptr<TextureManager> const &iTextureManager) :
  fTextureManager{iTextureManager},
  fFrontPanel(iTextureManager)
{
}

//------------------------------------------------------------------------
// Application::init
//------------------------------------------------------------------------
void Application::init()
{
  fFrontPanel.fPanelView.setBackground(
    fTextureManager->getTexture("/Volumes/Development/github/pongasoft/re-cva-7/GUI2D/Panel_Front.png"));
  {
    auto knob = std::make_unique<AnalogKnobWidget>();
    knob->setTexture(
      fTextureManager->getTexture("/Volumes/Development/github/pongasoft/re-cva-7/GUI2D/Knob_17_matte_63frames.png"));
    knob->setPosition({3414, 440});
    fFrontPanel.fPanelView.addWidget(std::move(knob));
  }
  {
    auto knob = std::make_unique<AnalogKnobWidget>();
    knob->setTexture(
      fTextureManager->getTexture("/Volumes/Development/github/pongasoft/re-cva-7/GUI2D/Knob_17_matte_63frames.png"));
    knob->setPosition({200, 440});
    fFrontPanel.fPanelView.addWidget(std::move(knob));
  }
}

//------------------------------------------------------------------------
// Application::loadFilmStrip
//------------------------------------------------------------------------
bool Application::loadFilmStrip(char const *iPath, int iNumFrames)
{
  return fTextureManager->loadFilmStrip(iPath, iNumFrames);
}

//------------------------------------------------------------------------
// Application::render
//------------------------------------------------------------------------
void Application::render()
{
  // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
  if(show_demo_window)
    ImGui::ShowDemoWindow(&show_demo_window);

  // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
  {
    ImGui::Begin("Main");

    ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &show_another_window);
    ImGui::Checkbox("Show/Hide Front Panel", &fFrontPanel.fVisible);

    ImGui::SliderFloat("zoom", &fFrontPanel.fDrawContext.getZoom(), 0.25f, 1.5f);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);

    ImGui::End();
  }

  if(fFrontPanel.fVisible)
  {
    if(ImGui::Begin("Front Panel", nullptr, ImGuiWindowFlags_HorizontalScrollbar))
    {
      fFrontPanel.fPanelView.draw(fFrontPanel.fDrawContext);
    }
    ImGui::End();
  }

  // 3. Show another simple window.
  if(show_another_window)
  {
    ImGui::Begin("Another Window",
                 &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    ImGui::Text("Hello from another window!");
    if(ImGui::Button("Close Me"))
      show_another_window = false;
    ImGui::End();
  }
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
Application::PanelState::PanelState(std::shared_ptr<TextureManager> iTextureManager) :
  fDrawContext(std::move(iTextureManager))
{
}

}