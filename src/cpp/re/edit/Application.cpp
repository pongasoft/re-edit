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
  fPropertyManager->init("/Volumes/Development/github/pongasoft/re-cva-7");

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

//------------------------------------------------------------------------
// Application::PanelState
//------------------------------------------------------------------------
Application::PanelState::PanelState(Panel::Type iPanelType,
                                    std::shared_ptr<TextureManager> iTextureManager,
                                    std::shared_ptr<UserPreferences> iUserPreferences,
                                    std::shared_ptr<PropertyManager> iPropertyManager) :
  fPanel(iPanelType),
  fDrawContext(std::move(iTextureManager), std::move(iUserPreferences), std::move(iPropertyManager))
{
}

//------------------------------------------------------------------------
// Application::render
//------------------------------------------------------------------------
void Application::PanelState::render()
{
  if(ImGui::BeginTabItem(fPanel.getName()))
  {
    ImGui::SliderFloat("zoom", &fDrawContext.fZoom, 0.25f, 1.5f);

    ImGui::Checkbox("Show Widget Border", &fDrawContext.fShowWidgetBorder);

    ReGui::ToggleButton("Show Panel", "Hide Panel", &fShowPanel);
    ImGui::SameLine();
    ReGui::ToggleButton("Show Panel Widgets", "Hide Panel Widgets", &fShowPanelWidgets);
    ImGui::SameLine();
    ReGui::ToggleButton("Show Widgets", "Hide Widgets", &fShowWidgets);
    ImGui::SameLine();
    ReGui::ToggleButton("Show Properties", "Hide Properties", &fShowProperties);

    if(fShowPanel)
      renderPanel();

    if(fShowPanelWidgets)
      renderPanelWidgets();

    if(fShowWidgets)
      renderWidgets();

    if(fShowProperties)
      renderProperties();

    ImGui::EndTabItem();
  }
}

//------------------------------------------------------------------------
// Application::renderWidgets
//------------------------------------------------------------------------
void Application::PanelState::renderWidgets()
{
  if(ImGui::Begin("Widgets", &fShowWidgets))
  {
    // Add widget Button + Popup
    {
      if(ImGui::Button("Add"))
        ImGui::OpenPopup("add_widget");

      if(ImGui::BeginPopup("add_widget"))
      {
        for(auto widgetType: kWidgetTypes)
        {
          if(ImGui::Selectable(widgetType))
            fPanel.addWidget(Widget::widget(widgetType));
        }
        ImGui::EndPopup();
      }
    }

    auto selectedWidgets = fPanel.getSelectedWidgets();

    // Add Duplicate / Clear / Delete buttons
    ImGui::BeginDisabled(selectedWidgets.empty());
    {
      ImGui::SameLine();
      if(ImGui::Button("Dup"))
      {
        for(auto w: selectedWidgets)
          fPanel.addWidget(w->clone());
      }

      ImGui::SameLine();
      if(ImGui::Button("Clr"))
        fPanel.clearSelection();

      ImGui::SameLine();
      if(ImGui::Button("Del"))
      {
        for(auto w: selectedWidgets)
        {
          fPanel.deleteWidget(w->getId());
        }
      }
    }
    ImGui::EndDisabled();

    ImGui::Separator();

    // Show list of widgets
    auto ids = fPanel.getWidgetOrder();
    for(int n = 0; n < ids.size(); n++)
    {
      auto id = ids[n];
      auto const widget = fPanel.getWidget(id);
      auto item = widget->getName();
      ImGui::Selectable(item.c_str(), widget->isSelected());

      if(ImGui::IsItemClicked(ImGuiMouseButton_Left))
      {
        auto &io = ImGui::GetIO();
        fPanel.selectWidget(id, io.KeyShift);
      }

      if(ImGui::IsItemActive() && !ImGui::IsItemHovered())
      {
        int n_next = n + (ImGui::GetMouseDragDelta(0).y < 0.f ? -1 : 1);
        if (n_next >= 0 && n_next < ids.size())
        {
          fPanel.swap(n, n_next);
          ImGui::ResetMouseDragDelta();
        }
      }
    }
  }
  ImGui::End();
}

//------------------------------------------------------------------------
// Application::renderPanel
//------------------------------------------------------------------------
void Application::PanelState::renderPanel()
{
  if(ImGui::Begin("Panel", &fShowPanel, ImGuiWindowFlags_HorizontalScrollbar))
    fPanel.draw(fDrawContext);
  ImGui::End();
}

//------------------------------------------------------------------------
// Application::renderPanel
//------------------------------------------------------------------------
void Application::PanelState::renderPanelWidgets()
{
  if(ImGui::Begin("Panel Widgets", &fShowPanelWidgets, ImGuiWindowFlags_HorizontalScrollbar))
  {
    fPanel.editView(fDrawContext);
  }
  ImGui::End();
}

//------------------------------------------------------------------------
// Application::renderProperties
//------------------------------------------------------------------------
void Application::PanelState::renderProperties()
{
  if(ImGui::Begin("Properties", &fShowProperties, ImGuiWindowFlags_HorizontalScrollbar))
  {
    {
      if(ImGui::Button("Add"))
        ImGui::OpenPopup("add_property");

      if(ImGui::BeginPopup("add_property"))
      {
        auto properties = fDrawContext.fPropertyManager->getNotWatchList();

        for(auto const &path: properties)
        {
          if(ImGui::Selectable(path.c_str()))
            fDrawContext.fPropertyManager->addToWatchlist(path);
        }
        ImGui::EndPopup();
      }
    }

    ImGui::SameLine();
    if(ImGui::Button("Clr"))
      fDrawContext.fPropertyManager->clearWatchList();

    ImGui::Separator();

    auto properties = fDrawContext.fPropertyManager->getWatchList();

    if(!properties.empty())
    {
      for(auto const &path: properties)
      {
        ImGui::PushID(path.c_str());
        if(ImGui::Button("X"))
          fDrawContext.fPropertyManager->removeFromWatchlist(path);
        ImGui::SameLine();
        ImGui::TextWrapped("%s", path.c_str());
        if(ImGui::IsItemHovered())
        {
          ImGui::BeginTooltip();
          ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
          ImGui::TextUnformatted(fDrawContext.getPropertyInfo(path).c_str());
          ImGui::PopTextWrapPos();
          ImGui::EndTooltip();
        }
        ImGui::Indent();
        fDrawContext.fPropertyManager->editView(path);
        ImGui::Unindent();
        ImGui::PopID();
      }
    }
  }
  ImGui::End();
}

}