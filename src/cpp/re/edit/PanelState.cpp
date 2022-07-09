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

#include "PanelState.h"

namespace re::edit {
//------------------------------------------------------------------------
// PanelState::PanelState
//------------------------------------------------------------------------
PanelState::PanelState(Panel::Type iPanelType,
                       std::shared_ptr<TextureManager> iTextureManager,
                       std::shared_ptr<UserPreferences> iUserPreferences,
                       std::shared_ptr<PropertyManager> iPropertyManager) :
  fPanel(iPanelType)
{
  fDrawContext.fTextureManager = std::move(iTextureManager);
  fDrawContext.fUserPreferences = std::move(iUserPreferences);
  fDrawContext.fPropertyManager = std::move(iPropertyManager);
}

//------------------------------------------------------------------------
// PanelState::render
//------------------------------------------------------------------------
void PanelState::render()
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
// PanelState::renderWidgets
//------------------------------------------------------------------------
void PanelState::renderWidgets()
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
        for(auto const &w: selectedWidgets)
          fPanel.addWidget(w->clone());
      }

      ImGui::SameLine();
      if(ImGui::Button("Clr"))
        fPanel.clearSelection();

      ImGui::SameLine();
      if(ImGui::Button("Del"))
      {
        for(auto const &w: selectedWidgets)
        {
          fPanel.deleteWidget(w->getId());
        }
      }
    }
    ImGui::EndDisabled();

    ImGui::Separator();

    // Show list of widgets
    fPanel.editOrderView(fDrawContext);
  }
  ImGui::End();
}

//------------------------------------------------------------------------
// PanelState::renderPanel
//------------------------------------------------------------------------
void PanelState::renderPanel()
{
  if(ImGui::Begin("Panel", &fShowPanel, ImGuiWindowFlags_HorizontalScrollbar))
    fPanel.draw(fDrawContext);
  ImGui::End();
}

//------------------------------------------------------------------------
// PanelState::renderPanelWidgets
//------------------------------------------------------------------------
void PanelState::renderPanelWidgets()
{
  if(ImGui::Begin("Panel Widgets", &fShowPanelWidgets, ImGuiWindowFlags_HorizontalScrollbar))
  {
    fPanel.editView(fDrawContext);
  }
  ImGui::End();
}

//------------------------------------------------------------------------
// PanelState::renderProperties
//------------------------------------------------------------------------
void PanelState::renderProperties()
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