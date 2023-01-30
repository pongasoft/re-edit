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
#include "Application.h"
#include <set>

namespace re::edit {

//------------------------------------------------------------------------
// PanelState::PanelState
//------------------------------------------------------------------------
PanelState::PanelState(PanelType iPanelType) :
  fPanel(iPanelType)
{
  std::copy_if(std::begin(kAllWidgetDefs), std::end(kAllWidgetDefs), std::back_inserter(fWidgetDefs),
               [iPanelType](auto const &def) { return isPanelOfType(iPanelType, def.fAllowedPanels); });
  for(auto &def: fWidgetDefs)
    fAllowedWidgetTypes[static_cast<size_t>(def.fType)] = true;
}

//------------------------------------------------------------------------
// PanelState::initPanel
//------------------------------------------------------------------------
std::map<std::string, int> PanelState::initPanel(AppContext &iCtx,
                                                 std::shared_ptr<lua::panel_nodes> const &iPanelNodes,
                                                 std::shared_ptr<lua::jbox_panel> const &iPanel)
{
  if(iPanelNodes == nullptr || iPanel == nullptr)
    return {};

  iCtx.disableUndo();

  auto currentPanelState = iCtx.fCurrentPanelState;

  iCtx.fCurrentPanelState = this;

  std::set<std::string> widgetNames{};

  // handle background
  {
    auto node = iPanelNodes->findNodeByName(iPanel->fGraphicsNode);
    if(node)
    {
      widgetNames.emplace(node->fName);
      if(node->hasKey())
        fPanel.setBackgroundKey(node->getKey());
    }
  }

  // Cable origin
  {
    if(iPanel->fCableOrigin)
    {
      auto node = iPanelNodes->findNodeByName(*iPanel->fCableOrigin);
      if(node)
      {
        widgetNames.emplace(node->fName);
        fPanel.setCableOrigin(node->fPosition);
      }
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
      widgetNames.emplace(node->fName);

      if(node->hasSize())
        widget->setSize(node->getSize());

      if(node->hasKey())
      {
        auto key = node->getKey();
        if(!key.empty())
          widget->setTextureKey(key);
        else
          RE_EDIT_LOG_WARNING("Empty node path for widget %s", node->fName);
      }

      if(w->fGraphics.fHitBoundaries)
        widget->setHitBoundaries(*w->fGraphics.fHitBoundaries);

      widget->setPosition(node->fPosition);
      widget->setName(node->fName);
    }


    fPanel.addWidget(iCtx, std::move(widget), "Add", false);
  }

  // handle decals: decals are all the nodes that have not been assigned to a widget
  for(auto &name: iPanelNodes->getDecalNames(widgetNames))
  {
    auto node = iPanelNodes->findNodeByName(name).value(); // we know that the node must exist

    if(node.hasKey())
    {
      auto widget = Widget::panel_decal(name);
      widget->setPosition(node.fPosition);

      auto key = node.getKey();
      if(!key.empty())
        widget->setTextureKey(key);
      else
        RE_EDIT_LOG_WARNING("Empty node path for decal %s", name);

      fPanel.addWidget(iCtx, std::move(widget), "Add", false);
    }
  }

  iCtx.enableUndo();

  iCtx.fCurrentPanelState = currentPanelState;

  return iPanelNodes->getNumFrames();
}

//------------------------------------------------------------------------
// PanelState::beforeRender
//------------------------------------------------------------------------
void PanelState::beforeRender(AppContext &iCtx)
{
  fPanel.computeEachFrame(iCtx);
  fPanel.checkForErrors(iCtx);
}

//------------------------------------------------------------------------
// PanelState::render
//------------------------------------------------------------------------
void PanelState::render(AppContext &iCtx)
{
  // when this panel becomes current, we force a check for widget errors as things may have changed (like
  // removed images, motherboard...)
  if(iCtx.fCurrentPanelState != iCtx.fPreviousPanelState)
  {
    fPanel.markEdited();
    if(iCtx.fPreviousPanelState &&
       iCtx.fCurrentPanelState->isUnfoldedPanel() != iCtx.fPreviousPanelState->isUnfoldedPanel())
    {
      iCtx.requestZoomToFit();
    }
  }

  renderPanel(iCtx);
  renderPanelWidgets(iCtx);
  renderWidgets(iCtx);
  renderProperties(iCtx);
}

//------------------------------------------------------------------------
// PanelState::renderWidgets
//------------------------------------------------------------------------
void PanelState::renderWidgets(AppContext &iCtx)
{
  if(auto l = iCtx.fWidgetsWindow.begin())
  {
    // Show list of widgets
    if(ImGui::BeginTabBar("Widgets & Decals", ImGuiTabBarFlags_None))
    {
      fPanel.editOrderView(iCtx);
      ImGui::EndTabBar();
    }
  }
}

//------------------------------------------------------------------------
// PanelState::renderPanel
//------------------------------------------------------------------------
void PanelState::renderPanel(AppContext &iCtx)
{
  auto windowPadding = ImGui::GetStyle().WindowPadding;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{});
  if(auto l = iCtx.fPanelWindow.begin())
  {
    auto &canvas = iCtx.getPanelCanvas();
    auto dpiScale = Application::GetCurrent().getCurrentFontDpiScale();
    canvas.begin(fPanel.getSize(), {iCtx.getZoom(), iCtx.isZoomFitContent(), Panel::kZoomMin * dpiScale, Panel::kZoomMax * dpiScale});
    fPanel.draw(iCtx, canvas, windowPadding);
    iCtx.setZoom(canvas.end());
  }
  ImGui::PopStyleVar();
}

//------------------------------------------------------------------------
// PanelState::renderPanelWidgets
//------------------------------------------------------------------------
void PanelState::renderPanelWidgets(AppContext &iCtx)
{
  if(auto l = iCtx.fPanelWidgetsWindow.begin())
  {
    fPanel.editView(iCtx);
  }
}

//------------------------------------------------------------------------
// PanelState::renderProperties
//------------------------------------------------------------------------
void PanelState::renderProperties(AppContext &iCtx)
{
  if(auto l = iCtx.fPropertiesWindow.begin())
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
          if(ReGui::ResetButton())
            iCtx.fPropertyManager->removeFromWatchlist(path);
          ImGui::SameLine();
          ImGui::TextWrapped("%s", path.c_str());
          if(ReGui::ShowQuickView())
          {
            ReGui::ToolTip([&iCtx, &path] {
              ImGui::TextUnformatted(iCtx.getPropertyInfo(path).c_str());
            });
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
}


}