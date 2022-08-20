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

#include "AppContext.h"
#include "Widget.h"
#include "PanelState.h"

namespace re::edit {

//------------------------------------------------------------------------
// AppContext::AppContext
//------------------------------------------------------------------------
AppContext::AppContext() :
  fFrontPanel(std::make_unique<PanelState>(PanelType::kFront)),
  fFoldedFrontPanel(std::make_unique<PanelState>(PanelType::kFoldedFront)),
  fBackPanel(std::make_unique<PanelState>(PanelType::kBack)),
  fFoldedBackPanel(std::make_unique<PanelState>(PanelType::kFoldedBack))
{}

//------------------------------------------------------------------------
// AppContext::initPanels
//------------------------------------------------------------------------
void AppContext::initPanels(std::string const &iDevice2DFile, std::string const &iHDGui2DFile)
{
  auto d2d = lua::Device2D::fromFile(iDevice2DFile);
  auto hdg = lua::HDGui2D::fromFile(iHDGui2DFile);
  fFrontPanel->initPanel(*this, d2d->front(), hdg->front());
  fFoldedFrontPanel->initPanel(*this, d2d->folded_front(), hdg->folded_front());
  fBackPanel->initPanel(*this, d2d->back(), hdg->back());
  fFoldedBackPanel->initPanel(*this, d2d->folded_back(), hdg->folded_back());
}

//------------------------------------------------------------------------
// AppContext::render
//------------------------------------------------------------------------
void AppContext::render()
{
  if(ImGui::BeginTabBar("Panels", ImGuiTabBarFlags_None))
  {
    fFrontPanel->render(*this);
    fBackPanel->render(*this);
    fFoldedFrontPanel->render(*this);
    fFoldedBackPanel->render(*this);
    ImGui::EndTabBar();
  }
}

//------------------------------------------------------------------------
// AppContext::renderAddWidgetMenuView
//------------------------------------------------------------------------
void AppContext::renderAddWidgetMenuView(ImVec2 const &iPosition)
{
  for(auto const &def: fCurrentPanelState->fWidgetDefs)
  {
    if(ImGui::MenuItem(def.fName))
    {
      auto widget = def.fFactory();
      widget->setPosition(iPosition);
      widget->setSelected(true);
      fCurrentPanelState->fPanel.addWidget(*this, std::move(widget));
    }
  }
}

//------------------------------------------------------------------------
// AppContext::getPanelState
//------------------------------------------------------------------------
PanelState *AppContext::getPanelState(PanelType iType) const
{
  switch(iType)
  {
    case PanelType::kFront:
      return fFrontPanel.get();
    case PanelType::kBack:
      return fBackPanel.get();
    case PanelType::kFoldedFront:
      return fFoldedFrontPanel.get();
    case PanelType::kFoldedBack:
      return fFoldedBackPanel.get();

    default:
      RE_EDIT_FAIL("should not be here");
  }
}

//------------------------------------------------------------------------
// AppContext::getPanel
//------------------------------------------------------------------------
Panel *AppContext::getPanel(PanelType iType) const
{
  return &getPanelState(iType)->fPanel;
}

//------------------------------------------------------------------------
// AppContext::getPanelSize
//------------------------------------------------------------------------
ImVec2 AppContext::getPanelSize() const
{
  return fCurrentPanelState->fPanel.getSize();
}

//------------------------------------------------------------------------
// AppContext::TextureItem
//------------------------------------------------------------------------
void AppContext::TextureItem(Texture const *iTexture, ImVec2 const &iPosition, int iFrameNumber, const ImVec4& iBorderCol) const
{
  iTexture->Item(iPosition, fZoom, iFrameNumber, iBorderCol);
}

//------------------------------------------------------------------------
// AppContext::drawTexture
//------------------------------------------------------------------------
void AppContext::drawTexture(Texture const *iTexture, ImVec2 const &iPosition, int iFrameNumber, const ImVec4& iBorderCol) const
{
  iTexture->draw(iPosition, fZoom, iFrameNumber, iBorderCol);
}

//------------------------------------------------------------------------
// AppContext::drawRect
//------------------------------------------------------------------------
void AppContext::drawRect(ImVec2 const &iPosition, ImVec2 const &iSize, ImU32 iColor) const
{
  auto const cp = ImGui::GetCursorScreenPos();
  ImVec2 pos(cp + iPosition * fZoom);
  auto drawList = ImGui::GetWindowDrawList();
  drawList->AddRect(pos, {pos.x + (iSize.x * fZoom), pos.y + (iSize.y * fZoom)}, iColor);
}

//------------------------------------------------------------------------
// AppContext::drawRectFilled
//------------------------------------------------------------------------
void AppContext::drawRectFilled(ImVec2 const &iPosition,
                                 ImVec2 const &iSize,
                                 ImU32 iColor,
                                 float iRounding,
                                 ImDrawFlags iFlags) const
{
  auto const cp = ImGui::GetCursorScreenPos();
  ImVec2 pos(cp + iPosition * fZoom);
  auto drawList = ImGui::GetWindowDrawList();
  drawList->AddRectFilled(pos, {pos.x + (iSize.x * fZoom), pos.y + (iSize.y * fZoom)}, iColor, iRounding, iFlags);
}

//------------------------------------------------------------------------
// AppContext::drawLine
//------------------------------------------------------------------------
void AppContext::drawLine(const ImVec2& iP1, const ImVec2& iP2, ImU32 iColor, float iThickness) const
{
  auto const cp = ImGui::GetCursorScreenPos();
  auto drawList = ImGui::GetWindowDrawList();
  drawList->AddLine(cp + iP1 * fZoom, cp + iP2 * fZoom, iColor, iThickness);
}

//------------------------------------------------------------------------
// AppContext::addUndoAttributeChange
//------------------------------------------------------------------------
void AppContext::addUndoAttributeChange(widget::Attribute *iAttribute)
{
  RE_EDIT_INTERNAL_ASSERT(fCurrentWidget != nullptr);
  RE_EDIT_INTERNAL_ASSERT(fCurrentPanelState != nullptr);

  auto panelType = fCurrentPanelState->getType();
  auto widgetId = fCurrentWidget->getId();
  std::shared_ptr<Widget> w = fCurrentWidget->clone();
  auto action = std::make_shared<WidgetUndoAction>();
  action->fFrame = fCurrentFrame;
  action->fDescription = re::mock::fmt::printf("%s.%s updated", fCurrentWidget->getName(), iAttribute->fName);
  action->fWidgetId = widgetId;
  action->fAttributeId = iAttribute->fId;
  action->fLambda = [panelType, widgetId, widget = std::move(w)](AppContext &iCtx) {
    auto w = iCtx.getPanel(panelType)->replaceWidget(widgetId, widget);
    return std::make_shared<LambdaRedoAction>([panelType, widgetId, w2 = std::move(w)](AppContext &iCtx) {
      iCtx.getPanel(panelType)->replaceWidget(widgetId, w2);
    });
  };

  fUndoManager->addUndoAction(std::move(action));
}

}