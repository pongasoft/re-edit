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
#include "Errors.h"

namespace re::edit {

//------------------------------------------------------------------------
// PanelState::PanelState
//------------------------------------------------------------------------
PanelState::PanelState(PanelType iPanelType,
                       std::shared_ptr<TextureManager> iTextureManager,
                       std::shared_ptr<UserPreferences> iUserPreferences,
                       std::shared_ptr<PropertyManager> iPropertyManager) :
  fPanel(iPanelType)
{
  fDrawContext.fPanelState = this;
  fDrawContext.fTextureManager = std::move(iTextureManager);
  fDrawContext.fUserPreferences = std::move(iUserPreferences);
  fDrawContext.fPropertyManager = std::move(iPropertyManager);
  std::copy_if(std::begin(kAllWidgetDefs), std::end(kAllWidgetDefs), std::back_inserter(fWidgetDefs),
               [iPanelType](auto const &def) { return isPanelOfType(iPanelType, def.fAllowedPanels); });
}

//------------------------------------------------------------------------
// PanelState::initPanel
//------------------------------------------------------------------------
void PanelState::initPanel(std::shared_ptr<lua::panel_nodes> const &iPanelNodes,
                           std::shared_ptr<lua::jbox_panel> const &iPanel)
{
  if(iPanelNodes == nullptr || iPanel == nullptr)
    return;

  // handle background
  {
    auto node = iPanelNodes->findNodeByName(iPanel->fGraphicsNode);
    if(node && node->hasKey())
    {
      auto background = fDrawContext.fTextureManager->findTexture(node->getKey());
      if(background)
        fPanel.setBackground(std::move(background));
      else
        RE_EDIT_LOG_WARNING ("Could not locate background texture [%s] for panel [%s]", iPanel->fGraphicsNode,
                             fPanel.getName());
    }
  }

  // Cable origin
  {
    if(iPanel->fCableOrigin)
    {
      auto node = iPanelNodes->findNodeByName(*iPanel->fCableOrigin);
      if(node)
        fPanel.setCableOrigin(node->fPosition);
      else
        RE_EDIT_LOG_WARNING ("Could not locate cable origin for panel [%s]", *iPanel->fCableOrigin,
                             fPanel.getName());
    }
  }

  // Options
  {
    if(!iPanel->fOptions.empty())
      fPanel.setOptions(iPanel->fOptions);
  }

  for(auto const &w: iPanel->fWidgets)
  {
    auto widget = w->fWidget->clone();

    widget->init(fDrawContext);

    auto node = iPanelNodes->findNodeByName(w->fGraphics.fNode);
    if(node)
    {
      if(node->hasKey())
      {
        auto graphics = fDrawContext.fTextureManager->findTexture(node->getKey());
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


    fPanel.addWidget(fDrawContext, std::move(widget));
  }
}

//------------------------------------------------------------------------
// PanelState::render
//------------------------------------------------------------------------
void PanelState::render()
{
  if(ImGui::BeginTabItem(fPanel.getName()))
  {
    fPanel.computeIsHidden(fDrawContext);
    fPanel.checkForWidgetErrors(fDrawContext);

    int zoom = static_cast<int>(fDrawContext.fZoom * 5);
    if(ImGui::SliderInt("zoom", &zoom, 1, 10))
      fDrawContext.fZoom = static_cast<float>(zoom) / 5.0f;
    ImGui::SameLine();
    ImGui::Text("%d%%", static_cast<int>(fDrawContext.fZoom * 100));

    ImGui::PushID("Border");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Border");
    ImGui::SameLine();
    ReGui::RadioButton("None", &fDrawContext.fShowBorder, EditContext::ShowBorder::kNone);
    ImGui::SameLine();
    ReGui::RadioButton("Widget", &fDrawContext.fShowBorder, EditContext::ShowBorder::kWidget);
    ImGui::SameLine();
    ReGui::RadioButton("Hit Boundaries", &fDrawContext.fShowBorder, EditContext::ShowBorder::kHitBoundaries);
    ImGui::PopID();

    ImGui::PushID("Custom Display");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Custom Display");
    ImGui::SameLine();
    ReGui::RadioButton("None", &fDrawContext.fShowCustomDisplay, EditContext::ShowCustomDisplay::kNone);
    ImGui::SameLine();
    ReGui::RadioButton("Main", &fDrawContext.fShowCustomDisplay, EditContext::ShowCustomDisplay::kMain);
    ImGui::SameLine();
    ReGui::RadioButton("SD Bg", &fDrawContext.fShowCustomDisplay, EditContext::ShowCustomDisplay::kBackgroundSD);
    ImGui::SameLine();
    ReGui::RadioButton("HD Bg", &fDrawContext.fShowCustomDisplay, EditContext::ShowCustomDisplay::kBackgroundHD);
    ImGui::PopID();

    ImGui::PushID("Sample Drop Zone");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Sample Drop Zone");
    ImGui::SameLine();
    ReGui::RadioButton("None", &fDrawContext.fShowSampleDropZone, EditContext::ShowSampleDropZone::kNone);
    ImGui::SameLine();
    ReGui::RadioButton("Fill", &fDrawContext.fShowSampleDropZone, EditContext::ShowSampleDropZone::kFill);
    ImGui::PopID();

    ReGui::ToggleButton("Show Panel", "Hide Panel", &fShowPanel);
    ImGui::SameLine();
    ReGui::ToggleButton("Show Panel Widgets", "Hide Panel Widgets", &fShowPanelWidgets);
    ImGui::SameLine();
    ReGui::ToggleButton("Show Widgets", "Hide Widgets", &fShowWidgets);
    ImGui::SameLine();
    ReGui::ToggleButton("Show Properties", "Hide Properties", &fDrawContext.fShowProperties);

    if(fShowPanel)
      renderPanel();

    if(fShowPanelWidgets)
      renderPanelWidgets();

    if(fShowWidgets)
      renderWidgets();

    if(fDrawContext.fShowProperties)
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
        for(auto const &def: fWidgetDefs)
        {
          if(ImGui::MenuItem(def.fName))
            fPanel.addWidget(fDrawContext, def.fFactory());
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
          fPanel.addWidget(fDrawContext, w->copy());
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
    if(ImGui::BeginTabBar("Widgets & Decals", ImGuiTabBarFlags_None))
    {
      fPanel.editOrderView(fDrawContext);
      ImGui::EndTabBar();
    }
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
  if(ImGui::Begin("Properties", &fDrawContext.fShowProperties, ImGuiWindowFlags_HorizontalScrollbar))
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

    if(ImGui::BeginChild("Content"))
    {
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
    ImGui::EndChild();
  }
  ImGui::End();
}

}