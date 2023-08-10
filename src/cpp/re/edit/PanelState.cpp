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
    fAllowedWidgetTypes[def.fType] = true;
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
      {
        auto key = node->getKey();
        if(!key.empty())
        {
          iCtx.loadTexture(key, node->fNumFrames);
          if(node->fOriginalPath)
            iCtx.loadTexture(*node->fOriginalPath, node->fNumFrames);
          fPanel.initBackgroundKey(key, node->fOriginalPath, node->fEffects);
        }
        else
          RE_EDIT_LOG_WARNING("Empty node path for panel %s", node->fName);
      }
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
        {
          iCtx.loadTexture(key, node->fNumFrames);
          if(node->fOriginalPath)
            iCtx.loadTexture(*node->fOriginalPath, node->fNumFrames);
          widget->initTextureKey(key, node->fOriginalPath, node->fEffects);
        }
        else
          RE_EDIT_LOG_WARNING("Empty node path for widget %s", node->fName);
      }

      if(w->fGraphics.fHitBoundaries)
        widget->setHitBoundaries(*w->fGraphics.fHitBoundaries);

      widget->setPositionAction(node->fPosition);
      widget->setNameAction(node->fName);
    }


    fPanel.addWidgetAction(std::move(widget));
  }

  // handle decals: decals are all the nodes that have not been assigned to a widget
  for(auto &name: iPanelNodes->getDecalNames(widgetNames))
  {
    auto node = iPanelNodes->findNodeByName(name).value(); // we know that the node must exist

    if(node.hasKey())
    {
      auto widget = Widget::panel_decal(name);
      widget->setPositionAction(node.fPosition);

      auto key = node.getKey();
      if(!key.empty())
      {
        iCtx.loadTexture(key, node.fNumFrames);
        if(node.fOriginalPath)
          iCtx.loadTexture(*node.fOriginalPath, node.fNumFrames);
        widget->initTextureKey(key, node.fOriginalPath, node.fEffects);
      }
      else
        RE_EDIT_LOG_WARNING("Empty node path for decal %s", name);

      fPanel.addWidgetAction(std::move(widget));
    }
  }

  iCtx.enableUndo();

  iCtx.fCurrentPanelState = currentPanelState;
}

//------------------------------------------------------------------------
// PanelState::beforeRender
//------------------------------------------------------------------------
void PanelState::beforeRender(AppContext &iCtx)
{
  fPanel.beforeEachFrame(iCtx);
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
  auto windowBg = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{});
  if(auto l = iCtx.fPanelWindow.begin())
  {
    auto const regionSize = ImGui::GetContentRegionAvail();
    auto &renderTexture = iCtx.getPanelCanvasRenderTexture(regionSize);
    auto const renderRegionSize = renderTexture.renderSize();
    auto const textureSize = renderTexture.rlTextureSize();

    // Implementation notes:
    // * the canvas renders into renderTexture (from {0,0} to {renderRegionSize.x, renderRegionSize.y})
    // * renderTexture is potentially much bigger (textureSize) because we don't shrink it for efficiency reasons (it
    //   gets reused every frame (iCtx.getPanelCanvasRenderTexture(regionSize))), hence the division
    // * renderTexture is flipped vertically hence the Y shenanigans
    // * it seems out of order because first, we write the Image (since it needs to be below everything), and then
    //   we generate its content. But it works because, ImGui::Image only enqueues the fact that the image associated
    //   to the texture must be rendered and, by the time the action takes place, the texture has been populated

    constexpr auto uv0 = ImVec2{0, 1};
    auto uv1 = renderRegionSize / textureSize;
    uv1.y = 1 - uv1.y; // need to flip Y

    auto cp = ImGui::GetCursorScreenPos();
    ImGui::Image(renderTexture.asImTextureID(), regionSize, uv0, uv1);
    ImGui::SetCursorScreenPos(cp); // restore cursor position (Image moves it)

    auto &canvas = iCtx.getPanelCanvas();
    auto dpiScale = Application::GetCurrent().getCurrentFontDpiScale();
    BeginTextureMode(renderTexture.asRLRenderTexture()); // all raylib calls will render into renderTexture
    canvas.begin(fPanel.getSize(),
                 renderTexture.scale(),
                 {iCtx.getZoom(), iCtx.isZoomFitContent(), Panel::kZoomMin * dpiScale, Panel::kZoomMax * dpiScale},
                 windowBg);
    fPanel.draw(iCtx, canvas, windowPadding);
    iCtx.setZoom(canvas.end());
    EndTextureMode(); // finished rendering into renderTexture
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
    fPanel.visibilityPropertiesView(iCtx);
  }
}


}