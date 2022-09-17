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
void AppContext::TextureItem(Texture const *iTexture,
                             ImVec2 const &iPosition,
                             int iFrameNumber,
                             ImU32 iBorderColor,
                             ImU32 iTextureColor) const
{
  iTexture->Item(iPosition, fZoom, iFrameNumber, iBorderColor, iTextureColor);
}

//------------------------------------------------------------------------
// AppContext::drawTexture
//------------------------------------------------------------------------
void AppContext::drawTexture(Texture const *iTexture,
                             ImVec2 const &iPosition,
                             int iFrameNumber,
                             ImU32 iBorderColor,
                             ImU32 iTextureColor) const
{
  iTexture->draw(iPosition, fZoom, iFrameNumber, iBorderColor, iTextureColor);
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
// AppContext::addUndoAction
//------------------------------------------------------------------------
void AppContext::addUndoAction(std::shared_ptr<UndoAction> iAction)
{
  iAction->fFrame = fCurrentFrame;
  if(fCurrentPanelState)
    iAction->fPanelType = fCurrentPanelState->getType();
  if(fUndoTransaction)
  {
    if(!iAction->fMergeKey)
      iAction->fMergeKey = fUndoTransaction->fMergeKey;
    fUndoTransaction->add(std::move(iAction));
  }
  else
    fUndoManager->addUndoAction(std::move(iAction));
}

//------------------------------------------------------------------------
// AppContext::populateLambdaUndoAction
//------------------------------------------------------------------------
void AppContext::populateWidgetUndoAction(WidgetUndoAction *iAction, Widget const *iWidget)
{
  RE_EDIT_INTERNAL_ASSERT(fCurrentPanelState != nullptr);
  RE_EDIT_INTERNAL_ASSERT(iWidget != nullptr);

  iAction->fWidget = iWidget->clone();
  iAction->fWidgetId = iWidget->getId();
}

//------------------------------------------------------------------------
// AppContext::computeUpdateDescription
//------------------------------------------------------------------------
std::string AppContext::computeUpdateDescription(Widget const *iWidget, widget::Attribute const *iAttribute)
{
  RE_EDIT_INTERNAL_ASSERT(iWidget != nullptr);

  if(iAttribute)
    return fmt::printf("Update %s.%s", iWidget->getName(), iAttribute->fName);
  else
    return fmt::printf("Update %s", iWidget->getName());
}

//------------------------------------------------------------------------
// AppContext::computeResetDescription
//------------------------------------------------------------------------
std::string AppContext::computeResetDescription(Widget const *iWidget, widget::Attribute const *iAttribute)
{
  RE_EDIT_INTERNAL_ASSERT(iWidget != nullptr);

  if(iAttribute)
    return fmt::printf("Reset %s.%s", iWidget->getName(), iAttribute->fName);
  else
    return fmt::printf("Reset %s", iWidget->getName());
}

//------------------------------------------------------------------------
// AppContext::addUndoWidgetChange
//------------------------------------------------------------------------
void AppContext::addUndoWidgetChange(Widget const *iWidget, std::string iDescription)
{
  auto action = std::make_unique<WidgetUndoAction>();
  action->fWidgetId = iWidget->getId();
  action->fDescription = std::move(iDescription);
  populateWidgetUndoAction(action.get(), iWidget);
  addUndoAction(std::move(action));
}

//------------------------------------------------------------------------
// AppContext::beginUndoTx
//------------------------------------------------------------------------
bool AppContext::beginUndoTx(std::string const &iDescription, void *iMergeKey)
{
  RE_EDIT_INTERNAL_ASSERT(fUndoTransaction == nullptr); // no support for nested transactions

  auto last = fUndoManager->getLastUndoAction();

  if(iMergeKey != nullptr && last && last->getMergeKey() == iMergeKey)
  {
    return false;
  }
  else
  {
    fUndoTransaction = std::make_unique<CompositeUndoAction>();
    fUndoTransaction->fFrame = fCurrentFrame;
    if(fCurrentPanelState)
      fUndoTransaction->fPanelType = fCurrentPanelState->getType();
    fUndoTransaction->fDescription = iDescription;
    fUndoTransaction->fMergeKey = iMergeKey;
    return true;
  }
}

//------------------------------------------------------------------------
// AppContext::commitUndoTx
//------------------------------------------------------------------------
void AppContext::commitUndoTx()
{
  RE_EDIT_INTERNAL_ASSERT(fUndoTransaction != nullptr);
  fUndoManager->addUndoAction(std::move(fUndoTransaction));
}

//------------------------------------------------------------------------
// AppContext::resetUndoMergeKey
//------------------------------------------------------------------------
void AppContext::resetUndoMergeKey()
{
  auto last = fUndoManager->getLastUndoAction();
  if(last)
    last->resetMergeKey();
}

//------------------------------------------------------------------------
// AppContext::init
//------------------------------------------------------------------------
void AppContext::init(lua::Config const &iConfig)
{
  fNativeWindowWidth = iConfig.fNativeWindowWidth;
  fNativeWindowHeight = iConfig.fNativeWindowHeight;
  fShowPanel = iConfig.fShowPanel;
  fShowPanelWidgets = iConfig.fShowPanelWidgets;
  fShowProperties = iConfig.fShowProperties;
  fShowWidgets = iConfig.fShowWidgets;
  fFontManager->requestNewFont({"JetBrains Mono Regular", BuiltInFont::kJetBrainsMonoRegular, iConfig.fFontSize});
  fGrid = iConfig.fGrid;
//  fShowBorder = static_cast<ShowBorder>(iConfig.fShowBorder);
//  fShowCustomDisplay = static_cast<ShowCustomDisplay>(iConfig.fShowCustomDisplay);
//  fShowSampleDropZone = static_cast<ShowSampleDropZone>(iConfig.fShowSampleDropZone);
}

//------------------------------------------------------------------------
// AppContext::getLuaConfig
//------------------------------------------------------------------------
std::string AppContext::getLuaConfig() const
{
  std::stringstream s{};

  s << fmt::printf("re_edit[\"native_window_width\"] = %d\n", fNativeWindowWidth);
  s << fmt::printf("re_edit[\"native_window_height\"] = %d\n", fNativeWindowHeight);
  s << fmt::printf("re_edit[\"show_panel\"] = %s\n", fmt::Bool::to_chars(fShowPanel));
  s << fmt::printf("re_edit[\"show_panel_widgets\"] = %s\n", fmt::Bool::to_chars(fShowPanelWidgets));
  s << fmt::printf("re_edit[\"show_properties\"] = %s\n", fmt::Bool::to_chars(fShowProperties));
  s << fmt::printf("re_edit[\"show_widgets\"] = %s\n", fmt::Bool::to_chars(fShowWidgets));
  s << fmt::printf("re_edit[\"font_size\"] = %d\n", static_cast<int>(fFontManager->getCurrentFont().fSize));
  s << fmt::printf("re_edit[\"grid\"] = { %d, %d }\n", static_cast<int>(fGrid.x), static_cast<int>(fGrid.y));
//  s << fmt::printf("re_edit[\"show_border\"] = %d\n", static_cast<int>(fShowBorder));
//  s << fmt::printf("re_edit[\"show_custom_display\"] = %d\n", static_cast<int>(fShowCustomDisplay));
//  s << fmt::printf("re_edit[\"show_sample_drop_zone\"] = %d\n", static_cast<int>(fShowSampleDropZone));

  return s.str();
}

//------------------------------------------------------------------------
// AppContext::onNativeWindowPositionChange
//------------------------------------------------------------------------
void AppContext::onNativeWindowPositionChange(int x, int y, float iScreenScale)
{
  if(fTextureManager->getScreenScale() != iScreenScale)
  {
    fTextureManager->setScreenScale(iScreenScale);
    fFontManager->requestFontScaleChange();
  }
}

}