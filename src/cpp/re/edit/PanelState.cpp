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
PanelState::PanelState(PanelType iPanelType) :
  fPanel(iPanelType)
{
  std::copy_if(std::begin(kAllWidgetDefs), std::end(kAllWidgetDefs), std::back_inserter(fWidgetDefs),
               [iPanelType](auto const &def) { return isPanelOfType(iPanelType, def.fAllowedPanels); });
}

//------------------------------------------------------------------------
// PanelState::initPanel
//------------------------------------------------------------------------
void PanelState::initPanel(AppContext &iCtx,
                           std::shared_ptr<lua::panel_nodes> const &iPanelNodes,
                           std::shared_ptr<lua::jbox_panel> const &iPanel)
{
  if(iPanelNodes == nullptr || iPanel == nullptr)
    return;

  iCtx.disableUndo();

  iCtx.fCurrentPanelState = this;

  // handle background
  {
    auto node = iPanelNodes->findNodeByName(iPanel->fGraphicsNode);
    if(node && node->hasKey())
    {
      auto background = iCtx.fTextureManager->findTexture(node->getKey());
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

    widget->init(iCtx);

    auto node = iPanelNodes->findNodeByName(w->fGraphics.fNode);
    if(node)
    {
      if(node->hasKey())
      {
        auto graphics = iCtx.fTextureManager->findTexture(node->getKey());
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


    fPanel.addWidget(iCtx, std::move(widget), false);
  }

  // handle decals
  for(auto &node: iPanelNodes->fDecalNodes)
  {
    auto widget = Widget::panel_decal();
    widget->setPosition(node.fPosition);

    auto graphics = iCtx.fTextureManager->findTexture(node.fKey);
    if(graphics)
      widget->setTexture(std::move(graphics));

    if(node.fName)
      widget->setName(*node.fName);

    fPanel.addWidget(iCtx, std::move(widget), false);
  }

  iCtx.enableUndo();

  iCtx.fCurrentPanelState = nullptr;
}

//------------------------------------------------------------------------
// PanelState::render
//------------------------------------------------------------------------
void PanelState::render(AppContext &iCtx)
{
  if(ImGui::BeginTabItem(fPanel.getName()))
  {
    bool switchedTab = iCtx.fCurrentPanelState != this;

    iCtx.fCurrentPanelState = this;
    iCtx.fZoom = fZoom;

    fPanel.computeIsHidden(iCtx);
    fPanel.checkForWidgetErrors(iCtx);

    int zoom = static_cast<int>(iCtx.fZoom * 5);
    if(ImGui::SliderInt("zoom", &zoom, 1, 10))
      iCtx.fZoom = static_cast<float>(zoom) / 5.0f;
    ImGui::SameLine();
    ImGui::Text("%d%%", static_cast<int>(iCtx.fZoom * 100));

    if(iCtx.fShowPanel)
      renderPanel(iCtx, switchedTab);

    if(iCtx.fShowPanelWidgets)
      renderPanelWidgets(iCtx);

    if(iCtx.fShowWidgets)
      renderWidgets(iCtx);

    if(iCtx.fShowProperties)
      renderProperties(iCtx);

    ImGui::EndTabItem();

    fZoom = iCtx.fZoom;
  }
}

//------------------------------------------------------------------------
// PanelState::renderWidgets
//------------------------------------------------------------------------
void PanelState::renderWidgets(AppContext &iCtx)
{
  if(ImGui::Begin("Widgets", &iCtx.fShowWidgets))
  {
    // Show list of widgets
    if(ImGui::BeginTabBar("Widgets & Decals", ImGuiTabBarFlags_None))
    {
      fPanel.editOrderView(iCtx);
      ImGui::EndTabBar();
    }
  }
  ImGui::End();
}

//------------------------------------------------------------------------
// PanelState::renderPanel
//------------------------------------------------------------------------
void PanelState::renderPanel(AppContext &iCtx, bool iSetScroll)
{
  if(ImGui::Begin("Panel", &iCtx.fShowPanel, ImGuiWindowFlags_HorizontalScrollbar))
  {
    fPanel.draw(iCtx);
    if(iSetScroll)
    {
      ImGui::SetScrollX(fScroll.x);
      ImGui::SetScrollY(fScroll.y);
    }
    else
      fScroll = {ImGui::GetScrollX(), ImGui::GetScrollY()};
  }
  ImGui::End();
}

//------------------------------------------------------------------------
// PanelState::renderPanelWidgets
//------------------------------------------------------------------------
void PanelState::renderPanelWidgets(AppContext &iCtx)
{
  if(ImGui::Begin("Panel Widgets", &iCtx.fShowPanelWidgets, ImGuiWindowFlags_HorizontalScrollbar))
  {
    fPanel.editView(iCtx);
  }
  ImGui::End();
}

//------------------------------------------------------------------------
// PanelState::renderProperties
//------------------------------------------------------------------------
void PanelState::renderProperties(AppContext &iCtx)
{
  if(ImGui::Begin("Properties", &iCtx.fShowProperties, ImGuiWindowFlags_HorizontalScrollbar))
  {
    {
      if(ImGui::Button("Add"))
        ImGui::OpenPopup("add_property");

      if(ImGui::BeginPopup("add_property"))
      {
        auto properties = iCtx.fPropertyManager->getNotWatchList();

        for(auto const &path: properties)
        {
          if(ImGui::Selectable(path.c_str()))
            iCtx.fPropertyManager->addToWatchlist(path);
        }
        ImGui::EndPopup();
      }
    }

    ImGui::SameLine();
    if(ImGui::Button("Clr"))
      iCtx.fPropertyManager->clearWatchList();

    ImGui::Separator();

    if(ImGui::BeginChild("Content"))
    {
      auto properties = iCtx.fPropertyManager->getWatchList();

      if(!properties.empty())
      {
        for(auto const &path: properties)
        {
          ImGui::PushID(path.c_str());
          if(ImGui::Button("X"))
            iCtx.fPropertyManager->removeFromWatchlist(path);
          ImGui::SameLine();
          ImGui::TextWrapped("%s", path.c_str());
          if(ImGui::IsItemHovered())
          {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(iCtx.getPropertyInfo(path).c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
          }
          ImGui::Indent();
          iCtx.fPropertyManager->editView(path);
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