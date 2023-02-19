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

#include <re/mock/fmt.h>
#include "Errors.h"
#include "Panel.h"
#include "PanelState.h"
#include "ReGui.h"
#include "Constants.h"
#include "LoggingManager.h"
#include "imgui_internal.h"

namespace re::edit {

namespace dnd
{
constexpr auto kWidget = "DNDW";
}

using namespace re::mock;

//------------------------------------------------------------------------
// kWidgetTypeToNames
//------------------------------------------------------------------------
std::map<WidgetType, char const *> kWidgetTypeToNames = []() {
  std::map<WidgetType, char const *> res{};
  for(auto const &def: kAllWidgetDefs)
    res[def.fType] = def.fName;
  return res;
}();

//------------------------------------------------------------------------
// toString
//------------------------------------------------------------------------
char const *toString(WidgetType iWidgetType)
{
  return kWidgetTypeToNames.at(iWidgetType);
}

//------------------------------------------------------------------------
// Panel::draw
//------------------------------------------------------------------------
Panel::Panel(PanelType iType) :
  fType{iType},
  fNodeName{fmt::printf("Panel_%s_bg", toString(iType))},
  fCableOrigin{fType == PanelType::kFoldedBack ? std::optional<ImVec2>({kDevicePixelWidth / 2.0f, kFoldedDevicePixelHeight / 2.0f}) : std::nullopt },
  fDisableSampleDropOnPanel{ fType == PanelType::kFront ? std::optional<bool>(false) : std::nullopt}
{
  setDeviceHeightRU(1);
}

//------------------------------------------------------------------------
// Panel::toString
//------------------------------------------------------------------------
char const *Panel::toString(PanelType iType)
{
  switch(iType)
  {
    case PanelType::kFront: return "front";
    case PanelType::kFoldedFront: return "folded_front";
    case PanelType::kBack: return "back";
    case PanelType::kFoldedBack: return "folded_back";
    default:
      RE_EDIT_FAIL("Not reached");
  }
}

//------------------------------------------------------------------------
// Panel::getWidget
//------------------------------------------------------------------------
Widget *Panel::getWidget(int id) const
{
  auto const &w = fWidgets.at(id);
  RE_EDIT_INTERNAL_ASSERT(w != nullptr);
  return w.get();
}

//------------------------------------------------------------------------
// Panel::findWidget
//------------------------------------------------------------------------
Widget *Panel::findWidget(int id) const
{
  auto iter = fWidgets.find(id);
  if(iter == fWidgets.end())
    return nullptr;
  else
    return iter->second.get();
}

//------------------------------------------------------------------------
// Panel::draw
//------------------------------------------------------------------------
void Panel::draw(AppContext &iCtx, ReGui::Canvas &iCanvas, ImVec2 const &iPopupWindowPadding)
{
  // rails are always below
  if(iCtx.fShowRackRails)
    drawRails(iCtx, iCanvas);

  if(iCtx.fPanelRendering != AppContext::EPanelRendering::kNone)
    drawPanel(iCtx, iCanvas);

  // always draw decals first
  drawWidgets(iCtx, iCanvas, fDecalsOrder);

  // then draws the widgets
  drawWidgets(iCtx, iCanvas, fWidgetsOrder);

  // then the cable origin
  drawCableOrigin(iCtx, iCanvas);

  // draw the fold button
  if(iCtx.hasFoldedPanels() && iCtx.fShowFoldButton)
  {
    iCanvas.addTexture(iCtx.getBuiltInTexture(BuiltIns::kFoldButton.fKey).get(),
                       kFoldButtonPos,
                       isPanelOfType(fType, kPanelTypeAnyUnfolded) ? 0 : 2);
  }

  if(fSelectWidgetsAction)
  {
    auto color = ImGui::GetColorU32({1,1,0,1});
    iCanvas.addRect(fSelectWidgetsAction->fInitialPosition, fSelectWidgetsAction->fCurrentPosition - fSelectWidgetsAction->fInitialPosition, color);
  }

  iCanvas.makeResponsive();

  auto const mousePos = iCanvas.getCanvasMousePos();

  auto selectedRect = dnz().fSelectedRect;

  if(fMoveWidgetsAction && selectedRect && (!ReGui::AnySpecialKey() || ImGui::GetIO().KeyAlt))
  {
    auto frameSize = getSize();
    auto color = ImGui::GetColorU32({1,1,0,0.5});
    iCanvas.addHorizontalLine(selectedRect->Min, color);
    iCanvas.addVerticalLine(selectedRect->Min, color);
    iCanvas.addHorizontalLine(selectedRect->Max, color);
    iCanvas.addVerticalLine(selectedRect->Max, color);
  }

  if(fSelectWidgetsAction)
  {
    handleSelectWidgetsAction(iCtx, mousePos);
  }
  else if(fMoveWidgetsAction)
  {
    handleMoveWidgetsAction(iCtx, mousePos);
  }
  else if(fMoveCanvasAction)
  {
    handleMoveCanvasAction(iCtx, iCanvas);
  }
  else if(ImGui::IsItemClicked(ImGuiMouseButton_Left))
  {
    handleLeftMouseClick(iCtx, mousePos);
  }
  else if(iCanvas.isHovered() && iCanvas.canReceiveInput() && ImGui::IsKeyDown(ImGuiKey_Space))
  {
    iCtx.setMouseCursorNextFrame(ImGuiMouseCursor_Hand);
  }

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, iPopupWindowPadding);

  if(iCanvas.canReceiveInput())
    handleCanvasInputs(iCtx, iCanvas);

  if(ImGui::BeginPopupContextItem())
  {
    if(!fPopupLocation)
      fPopupLocation = mousePos;
    renderPanelMenus(iCtx, *fPopupLocation);
    ImGui::EndPopup();
  }
  else
    fPopupLocation = std::nullopt;
  ImGui::PopStyleVar();


//  auto logging = LoggingManager::instance();
//
//  if(logging->isShowDebug())
//  {
//    logging->debug("Key[Ctrl]", "%s", fmt::Bool::to_chars(io.KeyCtrl));
//    logging->debug("Key[Shift]", "%s", fmt::Bool::to_chars(io.KeyShift));
//    logging->debug("Key[Super]", "%s", fmt::Bool::to_chars(io.KeySuper));
//    auto mousePos = ImGui::GetMousePos() - backgroundScreenPosition; // accounts for scrollbars
//    logging->debug("MouseDown", "%s | %fx%f (%fx%f)", ImGui::IsMouseDown(ImGuiMouseButton_::ImGuiMouseButton_Left) ? "true" : "false", mousePos.x, mousePos.y, backgroundScreenPosition.x, backgroundScreenPosition.y);
//  }
}

//------------------------------------------------------------------------
// Panel::drawRails
//------------------------------------------------------------------------
void Panel::drawRails(AppContext const &iCtx, ReGui::Canvas const &iCanvas) const
{
  auto rails = iCtx.getBuiltInTexture(BuiltIns::kRackRails.fKey);
  int leftFrame = 0;
  int rightFrame = 1;
  if(!isPanelOfType(fType, kPanelTypeAnyFront))
  {
    leftFrame = 2;
    rightFrame = 3;
  }
  auto heightRU = isPanelOfType(fType, kPanelTypeAnyUnfolded) ? fDeviceHeightRU : 1;
  auto leftPos = ImVec2{};
  auto rightPos = ImVec2{kDevicePixelWidth - rails->frameWidth(), 0};
  auto increment = ImVec2{ 0, rails->frameHeight() };
  for(int i = 0 ; i < heightRU; i++, leftPos += increment, rightPos += increment)
  {
    iCanvas.addTexture(rails.get(), leftPos, leftFrame);
    iCanvas.addTexture(rails.get(), rightPos, rightFrame);
  }
}

//------------------------------------------------------------------------
// Panel::drawPanel
//------------------------------------------------------------------------
void Panel::drawPanel(AppContext const &iCtx, ReGui::Canvas const &iCanvas) const
{
  auto texture = fGraphics.hasValidTexture() ? fGraphics.getTexture() : nullptr;

  if(iCtx.fPanelRendering == AppContext::EPanelRendering::kBorder)
  {
    iCanvas.addRect(ImVec2{}, getSize(), ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_Text]));
  }
  else
  {
    if(texture)
    {
      auto textureColor = iCtx.fPanelRendering == AppContext::EPanelRendering::kXRay ?
                          ReGui::GetColorU32(kXRayColor) :
                          ReGui::GetColorU32(kWhiteColor);
      iCanvas.addTexture(texture, {}, 0, ReGui::kTransparentColorU32, textureColor);
    }
    else
      iCanvas.addRectFilled(ImVec2{}, getSize(), iCtx.getUserPreferences().fWidgetErrorColor);
  }
}

//------------------------------------------------------------------------
// Panel::drawWidgets
//------------------------------------------------------------------------
void Panel::drawWidgets(AppContext &iCtx, ReGui::Canvas &iCanvas, std::vector<int> const &iOrder)
{
  for(auto id: iOrder)
  {
    auto &w = fWidgets[id];
    w->draw(iCtx, iCanvas);
  }
}

//------------------------------------------------------------------------
// Panel::drawCableOrigin
//------------------------------------------------------------------------
void Panel::drawCableOrigin(AppContext &iCtx, ReGui::Canvas &iCanvas)
{
  static constexpr auto kCableOriginSize = 10.0f;
  if(fShowCableOrigin && fCableOrigin)
  {
    iCanvas.addLine({ fCableOrigin->x - kCableOriginSize, fCableOrigin->y - kCableOriginSize},
                  { fCableOrigin->x + kCableOriginSize, fCableOrigin->y + kCableOriginSize},
                  iCtx.getUserPreferences().fSelectedWidgetColor);
    iCanvas.addLine({ fCableOrigin->x - kCableOriginSize, fCableOrigin->y + kCableOriginSize},
                    { fCableOrigin->x + kCableOriginSize, fCableOrigin->y - kCableOriginSize},
                    iCtx.getUserPreferences().fSelectedWidgetColor);
  }
}

//------------------------------------------------------------------------
// Panel::handleLeftMouseClick
//------------------------------------------------------------------------
void Panel::handleLeftMouseClick(AppContext &iCtx, ReGui::Canvas::canvas_pos_t const &iMousePos)
{
  auto &io = ImGui::GetIO();
  bool moveCanvasAction = false;

  if(io.KeyShift)
  {
    fSelectWidgetsAction = MouseDrag{iMousePos};
    selectWidget(iCtx, iMousePos, true);
  }
  else if(ImGui::IsKeyDown(ImGuiKey_Space))
  {
    moveCanvasAction = true;
  }
  else
  {
    if(selectWidget(iCtx, iMousePos, ReGui::IsSingleSelectKey(io)))
    {
      fMoveWidgetsAction = MouseDrag{iMousePos};
      fWidgetMove = WidgetMove{iMousePos};
    }
    else
      moveCanvasAction = true;
  }

  if(moveCanvasAction)
  {
    // Implementation note: we must use screen position because due to scrolling while dragging, mousePos actually
    // changes, so it is never stable!
    auto screenMousePos = ImGui::GetMousePos();
    fMoveCanvasAction = MouseDrag{screenMousePos};
    iCtx.setMouseCursorNextFrame(ImGuiMouseCursor_Hand);
  }
}

//------------------------------------------------------------------------
// Panel::handleShiftMouseDrag
//------------------------------------------------------------------------
void Panel::handleSelectWidgetsAction(AppContext &iCtx, ReGui::Canvas::canvas_pos_t const &iMousePos)
{
  if(ImGui::IsMouseReleased(ImGuiMouseButton_Left))
  {
    fSelectWidgetsAction = std::nullopt;
  }
  else
  {
    fSelectWidgetsAction->fCurrentPosition = iMousePos;
    if(fSelectWidgetsAction->fInitialPosition.x != fSelectWidgetsAction->fCurrentPosition.x ||
       fSelectWidgetsAction->fInitialPosition.y != fSelectWidgetsAction->fCurrentPosition.y)
    {
      selectWidgets(iCtx, fSelectWidgetsAction->fInitialPosition, fSelectWidgetsAction->fCurrentPosition);
    }
  }
}

//------------------------------------------------------------------------
// Panel::handleMoveWidgetsAction
//------------------------------------------------------------------------
void Panel::handleMoveWidgetsAction(AppContext &iCtx, ReGui::Canvas::canvas_pos_t const &iMousePos)
{
  if(ImGui::IsMouseReleased(ImGuiMouseButton_Left))
  {
    fMoveWidgetsAction = std::nullopt;
    endMoveWidgets(iCtx);
  }
  else
  {
    bool shouldMoveWidgets = false;
    auto grid = ImGui::GetIO().KeyAlt ? ImVec2{1.0f, 1.0f} : iCtx.fGrid;
    fMoveWidgetsAction->fCurrentPosition = iMousePos;
    if(std::abs(fMoveWidgetsAction->fLastUpdatePosition.x - fMoveWidgetsAction->fCurrentPosition.x) >= grid.x)
    {
      fMoveWidgetsAction->fLastUpdatePosition.x = fMoveWidgetsAction->fCurrentPosition.x;
      shouldMoveWidgets = true;
    }
    if(std::abs(fMoveWidgetsAction->fLastUpdatePosition.y - fMoveWidgetsAction->fCurrentPosition.y) >= grid.y)
    {
      fMoveWidgetsAction->fLastUpdatePosition.y = fMoveWidgetsAction->fCurrentPosition.y;
      shouldMoveWidgets = true;
    }
    if(shouldMoveWidgets)
      moveWidgets(iCtx, fMoveWidgetsAction->fCurrentPosition, grid);
  }
}

//------------------------------------------------------------------------
// Panel::handleMoveCanvasAction
//------------------------------------------------------------------------
void Panel::handleMoveCanvasAction(AppContext &iCtx, ReGui::Canvas &iCanvas)
{
  iCtx.setMouseCursorNextFrame(ImGuiMouseCursor_Hand);
  if(ImGui::IsMouseReleased(ImGuiMouseButton_Left))
  {
    fMoveCanvasAction = std::nullopt;
  }
  else
  {
    fMoveCanvasAction->fCurrentPosition = ImGui::GetMousePos();
    iCanvas.moveByDeltaScreenPos(fMoveCanvasAction->fCurrentPosition - fMoveCanvasAction->fLastUpdatePosition);
  }

  fMoveCanvasAction->fLastUpdatePosition = fMoveCanvasAction->fCurrentPosition;
}

//------------------------------------------------------------------------
// Panel::handleCanvasInputs
//------------------------------------------------------------------------
void Panel::handleCanvasInputs(AppContext &iCtx, ReGui::Canvas &iCanvas)
{
  constexpr float deltaAmount = 100.0f;

  if(ReGui::AnySpecialKey())
    return;

  // canvas scroll (arrows)
  ImVec2 delta{};
  if(ImGui::IsKeyPressed(ImGuiKey_RightArrow, true))
    delta.x = -deltaAmount;
  if(ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true))
    delta.x = deltaAmount;
  if(ImGui::IsKeyPressed(ImGuiKey_UpArrow, true))
    delta.y = deltaAmount;
  if(ImGui::IsKeyPressed(ImGuiKey_DownArrow, true))
    delta.y = -deltaAmount;
  if(delta.x != 0 || delta.y != 0)
    iCanvas.moveByDeltaCanvasPos(delta);

  // canvas center (C key)
  if(ImGui::IsKeyPressed(ImGuiKey_C, false))
    iCanvas.centerContent();

  // canvas zoom to fit (F key)
  if(ImGui::IsKeyPressed(ImGuiKey_F, false))
    iCanvas.zoomToFit();

  // toggle Widget X-Ray (X key)
  if(ImGui::IsKeyPressed(ImGuiKey_X, false))
    iCtx.toggleWidgetRenderingXRay();

  // toggle Widget Border (B key)
  if(ImGui::IsKeyPressed(ImGuiKey_B, false))
    iCtx.toggleWidgetBorder();

  // toggle rails (R key)
  if(ImGui::IsKeyPressed(ImGuiKey_R, false))
    iCtx.toggleRails();

  // toggle select/unselect ALL (A key)
  if(ImGui::IsKeyPressed(ImGuiKey_A, false))
    toggleSelectAll();

  // canvas zoom (mouse wheel)
  if(iCanvas.isHovered())
  {
    // Quick View (Q key)
    if(ReGui::IsQuickView())
    {
      auto w = findWidgetOnTopAt(iCanvas.getCanvasMousePos());
      if(w)
        ReGui::ToolTip([this, w] { renderWidgetValues(w); });
    }

    auto mouseVerticalWheel = ImGui::GetIO().MouseWheel;
    if(mouseVerticalWheel != 0)
    {
      auto zoom = 1.0f - mouseVerticalWheel * 0.05f;
      iCanvas.zoomBy(zoom, iCanvas.getCanvasMousePos());
    }
  }
}

//------------------------------------------------------------------------
// Panel::renderPanelWidgetMenu
//------------------------------------------------------------------------
bool Panel::renderPanelWidgetMenu(AppContext &iCtx, ImVec2 const &iPosition)
{
  ImGui::SeparatorText("Panel");

  auto res = false;

  auto alt = ImGui::GetIO().KeyAlt;

  ImGui::BeginDisabled(dnz().fSelectedWidgets.empty());
  if(ImGui::MenuItem("Unselect All"))
  {
    clearSelection();
    res |= true;
  }
  ImGui::EndDisabled();

  if(ImGui::MenuItem(alt ? "Select All (+ " ReGui_Icon_Hidden_Widget ")": "Select All"))
  {
    selectAll(alt);
    res |= true;
  }

  if(ImGui::BeginMenu(alt ? "Select By Type (+ " ReGui_Icon_Hidden_Widget ")" : "Select By Type"))
  {
    res |= renderSelectWidgetsByTypeMenuItems(dnz().fSortedByNameWidgets, alt);
    ImGui::EndMenu();
  }

  auto disabled = ReGui::BeginDisabled(!iCtx.isClipboardWidgetAllowedForPanel(fType));
  if(ImGui::MenuItem("Paste"))
  {
    if(iCtx.pasteFromClipboard(*this, iPosition))
      fEdited = true;
    res |= true;
  }

  if(!disabled)
    iCtx.renderClipboardTooltip();

  ImGui::EndDisabled();

  if(ImGui::BeginMenu("Add Widget"))
  {
    res |= iCtx.renderWidgetDefMenuItems(fType, [this, &iCtx, &iPosition](WidgetDef const &iDef) { addWidget(iCtx, iDef, iPosition); });
    ImGui::EndMenu();
  }
  if(ImGui::MenuItem("Add Decal"))
  {
    auto widget = Widget::panel_decal();
    widget->setPositionFromCenter(iPosition);
    addWidget(iCtx, std::move(widget), true);
    res |= true;
  }

  if(ImGui::MenuItem("Reset Visibility"))
  {
    resetAllWidgetsVisibility(iCtx);
  }

  return false;
}

//------------------------------------------------------------------------
// Panel::renderWidgetMenu
//------------------------------------------------------------------------
bool Panel::renderWidgetMenu(AppContext &iCtx, Widget *iWidget)
{
  bool res = false;

  ImGui::SeparatorText(iWidget->getName().c_str());
  if(ImGui::MenuItem(iWidget->isSelected() ? "Unselect" : "Select"))
  {
    toggleWidgetSelection(iWidget->getId(), true);
    res |= true;
  }

  if(ImGui::MenuItem("Copy"))
    iCtx.copyToClipboard(iWidget);

  if(ImGui::BeginMenu("Copy Value"))
  {
    for(auto &att: iWidget->fAttributes)
    {
      if(ImGui::MenuItem(att->toValueString().c_str()))
        iCtx.copyToClipboard(iWidget, att->fId);
    }
    ImGui::EndMenu();
  }

  auto disabled = ReGui::BeginDisabled(!iCtx.isClipboardMatchesType(clipboard::DataType::kWidget | clipboard::DataType::kWidgetAttribute));
  if(ImGui::MenuItem("Paste"))
  {
    if(iCtx.pasteFromClipboard(iWidget))
    {
      fEdited = true;
      res |= true;
    }
  }

  if(!disabled)
    iCtx.renderClipboardTooltip();

  ImGui::EndDisabled();

  if(ImGui::MenuItem("Delete"))
  {
    deleteWidgets(iCtx, {iWidget});
    res |= true;
  }

  iWidget->renderVisibilityMenu(iCtx);

  return res;
}

//------------------------------------------------------------------------
// Panel::renderPanelMenus
//------------------------------------------------------------------------
bool Panel::renderPanelMenus(AppContext &iCtx, std::optional<ImVec2> iPosition)
{
  bool res = false;

  Widget *widget{};

  if(iPosition)
    widget = findWidgetOnTopAt(*iPosition);

  if(widget)
  {
    if(!dnz().fSelectedWidgets.empty())
    {
      if(!widget->isSelected())
        res |= renderWidgetMenu(iCtx, widget);
      else
        res |= renderSelectedWidgetsMenu(iCtx);
    }
    else
      res |= renderWidgetMenu(iCtx, widget);
  }
  else
  {
    res |= renderPanelWidgetMenu(iCtx, iPosition ? *iPosition : getCenter());
  }

  return res;
}

//------------------------------------------------------------------------
// Panel::renderSelectedWidgetsMenu
//------------------------------------------------------------------------
bool Panel::renderSelectedWidgetsMenu(AppContext &iCtx, std::vector<Widget *> const &iWidgets)
{
  if(iWidgets.empty())
    return false;

  bool res = false;

  if(iWidgets.size() == 1)
    res |= renderWidgetMenu(iCtx, iWidgets[0]);
  else
  {
    ImGui::SeparatorText(fmt::printf("Selected Widgets (%ld)", iWidgets.size()).c_str());

    if(ImGui::MenuItem("Unselect"))
    {
      clearSelection();
      res |= true;
    }

    if(ImGui::MenuItem("Copy"))
      iCtx.copyToClipboard(iWidgets);

    auto disabled = ReGui::BeginDisabled(!iCtx.isClipboardMatchesType(clipboard::DataType::kWidget | clipboard::DataType::kWidgetAttribute));
    if(ImGui::MenuItem("Paste"))
    {
      if(iCtx.pasteFromClipboard(iWidgets))
      {
        fEdited = true;
        res |= true;
      }
    }

    if(!disabled)
      iCtx.renderClipboardTooltip();

    ImGui::EndDisabled();

    if(ImGui::MenuItem("Delete"))
    {
      deleteWidgets(iCtx, iWidgets);
      res |= true;
    }

    if(ImGui::BeginMenu("Visibility"))
    {
      if(ImGui::MenuItem("Show"))
      {
        setWidgetsVisibility(iCtx, iWidgets, widget::Visibility::kManualVisible);
        res |= true;
      }
      if(ImGui::MenuItem("Hide"))
      {
        setWidgetsVisibility(iCtx, iWidgets, widget::Visibility::kManualHidden);
        res |= true;
      }
      if(ImGui::MenuItem("Reset"))
      {
        setWidgetsVisibility(iCtx, iWidgets, widget::Visibility::kByProperty);
        res |= true;
      }
      ImGui::EndMenu();
    }

    if(ImGui::BeginMenu(fmt::printf("Widgets (%ld)", iWidgets.size()).c_str()))
    {
      for(auto &w: iWidgets)
      {
        if(ImGui::BeginMenu(w->getName().c_str()))
        {
          res |= renderWidgetMenu(iCtx, w);
          ImGui::EndMenu();
        }
      }
      ImGui::EndMenu();
    }
  }

  return res;
}

//------------------------------------------------------------------------
// Panel::renderSelectWidgetsByTypeMenuItems
//------------------------------------------------------------------------
bool Panel::renderSelectWidgetsByTypeMenuItems(std::vector<Widget *> const &iWidgets, bool iIncludeHiddenWidgets)
{
  bool res = false;
  WidgetTypeArray<int> byTypeCount{};
  for(auto w: iWidgets)
  {
    if(!w->isHidden() || iIncludeHiddenWidgets)
      byTypeCount[w->getType()]++;
  }
  for(auto const &def: kAllWidgetDefs)
  {
    auto const count = byTypeCount[def.fType];
    if(count > 0)
    {
      if(ImGui::MenuItem(fmt::printf("%s [%d]", def.fName, count).c_str()))
      {
        Widget::selectByType(iWidgets, def.fType, iIncludeHiddenWidgets);
        res = true;
      }
    }
  }
  return res;
}

//------------------------------------------------------------------------
// Panel::renderWidgetsMenu
//------------------------------------------------------------------------
std::size_t Panel::renderWidgetsMenu(AppContext &iCtx, std::vector<Widget *> const &iWidgets)
{
  static std::vector<Widget *> kSelectedWidgets{};
  std::copy_if(iWidgets.begin(), iWidgets.end(), std::back_inserter(kSelectedWidgets), [](auto w) { return w->isSelected(); });
  ImGui::PushID("List");
  ImGui::SeparatorText("List");
  ImGui::BeginDisabled(kSelectedWidgets.empty());
  if(ImGui::MenuItem("Unselect All"))
  {
    for(auto w: iWidgets)
      w->unselect();
  }
  ImGui::EndDisabled();

  ImGui::BeginDisabled(kSelectedWidgets.size() == iWidgets.size());
  if(ImGui::MenuItem("Select All"))
  {
    for(auto w: iWidgets)
      w->select();
  }
  ImGui::EndDisabled();

  if(ImGui::BeginMenu("Select By Type"))
  {
    renderSelectWidgetsByTypeMenuItems(iWidgets, true);
    ImGui::EndMenu();
  }

  if(ImGui::BeginMenu("Visibility"))
  {
    if(ImGui::MenuItem("Show"))
    {
      setWidgetsVisibility(iCtx, iWidgets, widget::Visibility::kManualVisible);
    }
    if(ImGui::MenuItem("Hide"))
    {
      setWidgetsVisibility(iCtx, iWidgets, widget::Visibility::kManualHidden);
    }
    if(ImGui::MenuItem("Reset"))
    {
      setWidgetsVisibility(iCtx, iWidgets, widget::Visibility::kByProperty);
    }
    ImGui::EndMenu();
  }

  ImGui::PopID();

  if(!kSelectedWidgets.empty())
  {
    renderSelectedWidgetsMenu(iCtx, kSelectedWidgets);
  }
  auto res = kSelectedWidgets.size();
  kSelectedWidgets.clear();
  return res;
}

//------------------------------------------------------------------------
// Panel::findWidgetOnTopAt
//------------------------------------------------------------------------
Widget *Panel::findWidgetOnTopAt(std::vector<int> const &iOrder, ImVec2 const &iPosition) const
{
  auto ci = std::find_if(iOrder.rbegin(), iOrder.rend(), [this, &iPosition](auto const id) {
    auto const &w = fWidgets.at(id);
    return !w->isHidden() && w->contains(iPosition);
  });

  if(ci == iOrder.rend())
    return nullptr;
  else
    return findWidget(*ci);
}

//------------------------------------------------------------------------
// Panel::findWidgetOnTopAt
//------------------------------------------------------------------------
Widget *Panel::findWidgetOnTopAt(ImVec2 const &iPosition) const
{
  auto widget = findWidgetOnTopAt(fWidgetsOrder, iPosition);
  if(!widget)
    widget = findWidgetOnTopAt(fDecalsOrder, iPosition);
  return widget;
}

//------------------------------------------------------------------------
// Panel::selectWidget
//------------------------------------------------------------------------
bool Panel::selectWidget(AppContext &iCtx, ImVec2 const &iPosition, bool iMultiSelectKey)
{
  auto widget = findWidgetOnTopAt(iPosition);
  if(!widget)
  {
    if(!iMultiSelectKey)
      clearSelection();
    return false;
  }
  else
  {
    if(iMultiSelectKey)
    {
      toggleWidgetSelection(widget->getId(), true);
    }
    else
    {
      if(!widget->isSelected())
        selectWidget(widget->getId(), false);
    }
    return true;
  }
}

//------------------------------------------------------------------------
// Panel::selectWidget
//------------------------------------------------------------------------
void Panel::selectWidget(int id, bool iMultiple)
{
  auto widget = getWidget(id);

  if(!iMultiple)
  {
    clearSelection();
    widget->fSelected = true;
  }
  else
  {
    widget->fSelected = true;
  }
}

//------------------------------------------------------------------------
// Panel::selectWidgets
//------------------------------------------------------------------------
void Panel::selectWidgets(std::set<int> const &iWidgetIds, bool iAddToSelection)
{
  if(!iAddToSelection)
    clearSelection();

  for(auto id: iWidgetIds)
  {
    if(auto w = findWidget(id))
      w->select();
  }
}

//------------------------------------------------------------------------
// Panel::toggleWidgetSelection
//------------------------------------------------------------------------
void Panel::toggleWidgetSelection(int id, bool iMultiple)
{
  auto widget = getWidget(id);
  if(widget->isSelected())
    unselectWidgetAction(id);
  else
    selectWidget(id, iMultiple);
}

//------------------------------------------------------------------------
// Panel::selectAll
//------------------------------------------------------------------------
void Panel::selectAll(bool iIncludeHiddenWidgets)
{
  for(auto &[id, w]: fWidgets)
  {
    w->setSelected(iIncludeHiddenWidgets || !w->isHidden());
  }
}

//------------------------------------------------------------------------
// Panel::selectAll
//------------------------------------------------------------------------
void Panel::toggleSelectAll(bool iIncludeHiddenWidgets)
{
  if(dnz().fSelectedWidgets.empty())
    selectAll(iIncludeHiddenWidgets);
  else
    clearSelection();
}

//------------------------------------------------------------------------
// Panel::selectByType
//------------------------------------------------------------------------
void Panel::selectByType(WidgetType iType, bool iIncludeHiddenWidgets)
{
  for(auto &[id, w]: fWidgets)
  {
    if(w->getType() == iType && (iIncludeHiddenWidgets || !w->isHidden()))
      w->select();
  }
}

//------------------------------------------------------------------------
// Panel::clearSelection
//------------------------------------------------------------------------
void Panel::clearSelection()
{
  for(auto &p: fWidgets)
    p.second->unselect();
}


//------------------------------------------------------------------------
// Panel::getSelectedWidgetIds
//------------------------------------------------------------------------
std::set<int> Panel::getSelectedWidgetIds() const
{
  std::set<int> ids{};
  for(auto &[id, w]: fWidgets)
  {
    if(w->isSelected())
      ids.emplace(w->getId());
  }
  return ids;
}

//------------------------------------------------------------------------
// PanelState::markEdited
//------------------------------------------------------------------------
void Panel::markEdited()
{
  fEdited = true;
  fGraphics.markEdited();
  for(auto &[n, widget]: fWidgets)
    widget->markEdited();
}

//------------------------------------------------------------------------
// PanelState::resetEdited
//------------------------------------------------------------------------
void Panel::resetEdited()
{
  fEdited = false;
  fGraphics.resetEdited();
  for(auto &[n, widget]: fWidgets)
    widget->resetEdited();
}

//------------------------------------------------------------------------
// PanelState::checkForErrors
//------------------------------------------------------------------------
bool Panel::checkForErrors(AppContext &iCtx)
{
  if(fEdited)
  {
    fUserError.clear();
    fWidgetNameHashes.clear();
    if(fGraphics.checkForErrors(iCtx))
      addAllErrors("graphics", fGraphics);

    for(auto &[n, widget]: fWidgets)
    {
      if(widget->checkForErrors(iCtx))
        addAllErrors(widget->getName(), *widget);
      auto [_, inserted] = fWidgetNameHashes.emplace(widget->getNameHash());
      if(!inserted)
        fUserError.add("Duplicate widget names [%s]", widget->getName());
    }

    fEdited = false;
  }

  return hasErrors();
}

//------------------------------------------------------------------------
// Panel::computeUniqueWidgetNameForCopy
//------------------------------------------------------------------------
std::string Panel::computeUniqueWidgetNameForCopy(std::string const &iOriginalName) const
{
  StringWithHash sw{fmt::printf("%s Copy", iOriginalName)};

  if(fWidgetNameHashes.find(sw.hash()) == fWidgetNameHashes.end())
    return sw.value();

  int i = 2;
  for(;; i++)
  {
    StringWithHash sw2{fmt::printf("%s Copy (%d)", iOriginalName, i)};
    if(fWidgetNameHashes.find(sw2.hash()) == fWidgetNameHashes.end())
      return sw2.value();
  }
}

//------------------------------------------------------------------------
// Panel::editView
//------------------------------------------------------------------------
void Panel::editView(AppContext &iCtx)
{
  if(ImGui::Begin("Panel Widgets"))
  {
//
//    ImGui::SliderFloat("Width", &kItemWidth, 0, ImGui::GetContentRegionAvail().x);
    ImGui::PushItemWidth(iCtx.fItemWidth);
//    ImGui::Text("region = %f | itemWidth = %f", ImGui::GetContentRegionAvail().x, kItemWidth);

    auto size = dnz().fSelectedWidgets.size();
    switch(size)
    {
      case 0:
      {
        editNoSelectionView(iCtx);
        break;
      }

      case 1:
      {
        editSingleSelectionView(iCtx, dnz().fSelectedWidgets[0]);
        break;
      }

      default:
      {
        editMultiSelectionView(iCtx);
        break;
      }
    }

    ImGui::PopItemWidth();
  }
  ImGui::End();

}


//------------------------------------------------------------------------
// Panel::renderWidgetValues
//------------------------------------------------------------------------
void Panel::renderWidgetValues(Widget const *iWidget)
{
  ImGui::SeparatorText(fmt::printf("%s [%s]", iWidget->getName(), re::edit::toString(iWidget->getType())).c_str());
  for(auto &att: iWidget->fAttributes)
  {
    ImGui::TextUnformatted(att->toValueString().c_str());
  }
}

//------------------------------------------------------------------------
// Panel::editNoSelectionView
//------------------------------------------------------------------------
void Panel::editNoSelectionView(AppContext &iCtx)
{
  ImGui::PushID("Panel");

  if(ReGui::MenuButton())
    ImGui::OpenPopup("Menu");

  if(ImGui::BeginPopup("Menu"))
  {
    renderPanelWidgetMenu(iCtx);
    ImGui::EndPopup();
  }

  ImGui::SameLine();
  ImGui::Text("%s panel", toString(fType));

  errorViewSameLine();

  ImGui::PushID("graphics");
  fGraphics.editView(iCtx);
  fEdited |= fGraphics.isEdited();
  fGraphics.errorViewSameLine();
  ImGui::PopID();

  if(fCableOrigin)
  {
    if(ImGui::TreeNode("Cable Origin"))
    {
      fShowCableOrigin = true;
      auto cableOrigin = *fCableOrigin;
      ReGui::InputInt("x", &cableOrigin.x, 1, 5);
      ReGui::InputInt("y", &cableOrigin.y, 1, 5);
      if(*fCableOrigin != cableOrigin)
      {
        setCableOrigin(cableOrigin);
      }
      ImGui::TreePop();
    }
    else
      fShowCableOrigin = false;
  }

  if(fDisableSampleDropOnPanel)
  {
    if(ImGui::TreeNode("Options"))
    {
      bool b = *fDisableSampleDropOnPanel;
      if(ImGui::Checkbox("disable_sample_drop_on_panel", &b))
      {
        setPanelOptions(b);
      }
      ImGui::TreePop();
    }
  }

  if(ImGui::TreeNode("hdgui2D"))
  {
    auto windowSize = ImGui::GetWindowSize();
    ImGui::PushTextWrapPos(windowSize.x);
    ImGui::TextUnformatted(hdgui2D().c_str());
    ImGui::PopTextWrapPos();
    ImGui::TreePop();
  }
  if(ImGui::TreeNode("device2D"))
  {
    auto windowSize = ImGui::GetWindowSize();
    ImGui::PushTextWrapPos(windowSize.x);
    ImGui::TextUnformatted(device2D().c_str());
    ImGui::PopTextWrapPos();
    ImGui::TreePop();
  }
  ImGui::PopID();
}

//------------------------------------------------------------------------
// Panel::editSingleSelectionView
//------------------------------------------------------------------------
void Panel::editSingleSelectionView(AppContext &iCtx, Widget *iWidget)
{
  if(ReGui::MenuButton())
    ImGui::OpenPopup("Menu");

  if(ImGui::BeginPopup("Menu"))
  {
    renderWidgetMenu(iCtx, iWidget);
    ImGui::EndPopup();
  }

  ImGui::SameLine();
  auto type = re::edit::toString(iWidget->getType());
  if(ImGui::BeginCombo("type", type))
  {
    for(auto const &def: iCtx.getPanelState(fType)->getAllowedWidgets())
    {
      if(ImGui::Selectable(def.fName, iWidget->getType() == def.fType))
      {
        if(iWidget->getType() != def.fType)
          transmuteWidget(iCtx, iWidget, def);
      }
    }
    ImGui::EndCombo();
  }

  ImGui::SameLine();

  iWidget->renderVisibilityToggle(iCtx);

  iWidget->errorViewSameLine();

  iWidget->editView(iCtx);

  if(iWidget->isEdited())
    fEdited = true;
}


//------------------------------------------------------------------------
// Panel::editMultiSelectionView
//------------------------------------------------------------------------
void Panel::editMultiSelectionView(AppContext &iCtx)
{
  if(ReGui::MenuButton())
    ImGui::OpenPopup("Menu");

  if(ImGui::BeginPopup("Menu"))
  {
    renderSelectedWidgetsMenu(iCtx);
    ImGui::EndPopup();
  }

  ImGui::SameLine();
  ImGui::Text("%ld selected", dnz().fSelectedWidgets.size());

  auto [min, max] = *dnz().fSelectedRect;

  auto editedMin = min;

  ReGui::InputInt("x", &editedMin.x, 1, 5);
  ReGui::InputInt("y", &editedMin.y, 1, 5);

  auto delta = editedMin - min;
  if(delta.x != 0 || delta.y != 0)
  {
    moveWidgets(delta);
  }

  ImGui::SeparatorText("Alignment");
  constexpr ImVec2 smallButtonSize{80, 0};
  ImVec2 bigButtonSize{smallButtonSize.x * 2.0f + ImGui::GetStyle().ItemSpacing.x, 0};

  if(ImGui::Button("Top", bigButtonSize))
  {
    alignWidgets(iCtx, WidgetAlignment::kTop);
  }

  if(ImGui::Button("Left", smallButtonSize))
  {
    alignWidgets(iCtx, WidgetAlignment::kLeft);
  }

  ImGui::SameLine();

  if(ImGui::Button("Right", smallButtonSize))
  {
    alignWidgets(iCtx, WidgetAlignment::kRight);
  }

  if(ImGui::Button("Bottom", bigButtonSize))
  {
    alignWidgets(iCtx, WidgetAlignment::kBottom);
  }
}

//------------------------------------------------------------------------
// Panel::MultiSelectionList::init
//------------------------------------------------------------------------
void Panel::OrderSelectionList::init(Panel &iPanel)
{
  auto &ids = iPanel.getOrder(fType);
  for(auto id: ids)
    fWidgetSelectionList.emplace_back(iPanel.findWidget(id));
}

//------------------------------------------------------------------------
// Panel::MultiSelectionList::editView
//------------------------------------------------------------------------
void Panel::OrderSelectionList::editView(AppContext &iCtx, Panel &iPanel)
{
  fWidgetSelectionList.popupMenuView(iCtx, iPanel);

  ImGui::SameLine();
  if(ImGui::Button("Up  "))
    iPanel.changeSelectedWidgetsOrder(iCtx, fType, Direction::kUp);
  ImGui::SameLine();
  if(ImGui::Button("Down"))
    iPanel.changeSelectedWidgetsOrder(iCtx, fType, Direction::kDown);

  ImGui::Separator();

  if(ImGui::BeginChild("Content"))
    fWidgetSelectionList.editView(iCtx, iPanel);
  ImGui::EndChild();
}

//------------------------------------------------------------------------
// Panel::editOrderView
//------------------------------------------------------------------------
void Panel::editOrderView(AppContext &iCtx)
{
  if(ImGui::BeginTabItem("All"))
  {
    static WidgetSelectionList kAllList{};
    for(auto w: dnz().fSortedByNameWidgets)
    {
      kAllList.emplace_back(w);
    }
    kAllList.popupMenuView(iCtx, *this);
    ImGui::Separator();
    if(ImGui::BeginChild("Content"))
      kAllList.editView(iCtx, *this);
    ImGui::EndChild();
    kAllList.clear();
    ImGui::EndTabItem();
  }

  if(ImGui::BeginTabItem("Widgets"))
  {
    fWidgetsSelectionList.init(*this);
    fWidgetsSelectionList.editView(iCtx, *this);
    fWidgetsSelectionList.clear();
    ImGui::EndTabItem();
  }

  if(ImGui::BeginTabItem("Decals"))
  {
    fDecalsSelectionList.init(*this);
    fDecalsSelectionList.editView(iCtx, *this);
    fDecalsSelectionList.clear();
    ImGui::EndTabItem();
  }
}

//------------------------------------------------------------------------
// Panel::WidgetSelectionList::editView
//------------------------------------------------------------------------
void Panel::WidgetSelectionList::editView(AppContext &iCtx, Panel &iPanel)
{
  for(auto widget: getWidgets())
  {
    ImGui::PushID(widget->getId());

    widget->renderVisibilityToggle(iCtx);

    ImGui::SameLine();

    auto const hidden = widget->isHidden();
    if(hidden)
    {
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().DisabledAlpha / 2.0f);
    }

    if(ImGui::Selectable(widget->getName().c_str(), widget->isSelected()))
    {
      auto io = ImGui::GetIO();
      handleClick(iPanel, widget, io.KeyShift, ReGui::IsSingleSelectKey(io));
    }

    // this is a shortcut/optimization since the only drop target (at the moment) is in the lists in the properties
    // window for a widget that has a visibility attribute
    if(iCtx.isPropertiesWindowVisible() && widget->hasVisibilityAttribute())
    {
      if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
      {
        auto id = widget->getId();
        ImGui::SetDragDropPayload(dnd::kWidget, &id, sizeof(int));
        ImGui::Text("Widget [%s]", widget->getName().c_str());
        ImGui::EndDragDropSource();
      }
    }

    if(hidden)
      ImGui::PopStyleVar();

    if(ImGui::BeginPopupContextItem())
    {
      iPanel.renderWidgetMenu(iCtx, widget);
      ImGui::EndPopup();
    }
    else if(ReGui::ShowQuickView())
    {
      ReGui::ToolTip([&iPanel, widget] { iPanel.renderWidgetValues(widget); });
    }

    widget->errorViewSameLine();
    ImGui::PopID();
  }
}

//------------------------------------------------------------------------
// Panel::WidgetSelectionList::menuView
//------------------------------------------------------------------------
void Panel::WidgetSelectionList::menuView(AppContext &iCtx, Panel &iPanel)
{
  iPanel.renderWidgetsMenu(iCtx, fWidgets);
}

//------------------------------------------------------------------------
// Panel::WidgetSelectionList::menuView
//------------------------------------------------------------------------
void Panel::WidgetSelectionList::popupMenuView(AppContext &iCtx, Panel &iPanel)
{
  if(ReGui::MenuButton())
    ImGui::OpenPopup("Menu");

  if(ImGui::BeginPopup("Menu"))
  {
    menuView(iCtx, iPanel);
    ImGui::EndPopup();
  }
}

//------------------------------------------------------------------------
// Panel::WidgetSelectionList::handleClick
//------------------------------------------------------------------------
void Panel::WidgetSelectionList::handleClick(Panel &iPanel, Widget *iWidget, bool iRangeSelectKey, bool iMultiSelectKey)
{
  auto id = iWidget->getId();

  // when iMultiSelectKey is held => multiple selection
  if(iMultiSelectKey)
  {
    if(!iWidget->isSelected())
    {
      iWidget->select();
      fLastSelected = id;
    }
    else
    {
      iWidget->unselect();
      fLastSelected = std::nullopt;
    }
    return;
  }

  auto &list = getWidgets();

  // when iRangeSelectKey is held => add all properties between fLastSelected and this one
  if(iRangeSelectKey && fLastSelected && std::find_if(list.begin(), list.end(), [id = *fLastSelected] (auto *w){ return w->getId() == id; }) != list.end())
  {
    bool copy = false;
    for(auto elt: list)
    {
      if(id != *fLastSelected && (elt->getId() == id || elt->getId() == *fLastSelected))
      {
        copy = !copy;
        elt->select();
      }
      else if(copy)
        elt->select();
    }

    fLastSelected = id;
    return;
  }

  // neither shift nor control is held => single selection (deselect all others)
  auto wasSelected = iWidget->fSelected;
  iPanel.clearSelection();
  if(wasSelected)
  {
    fLastSelected = std::nullopt;
  }
  else
  {
    iWidget->fSelected = true;
    fLastSelected = id;
  }
}

//------------------------------------------------------------------------
// struct VisibilityProperty
//------------------------------------------------------------------------
struct VisibilityProperty
{
  void add(int iValue, Widget *iWidget)
  {
    getOrCreate(iValue).emplace_back(iWidget);
  }

  //! only clears the underlying arrays
  void clear()
  {
    for(auto &[id, v]: fValues)
      v.clear();
  }

  //! gets rid of entries with no values
  bool prune()
  {
    for(auto i = fValues.cbegin(); i != fValues.cend(); )
    {
      i = i->second.empty() ? fValues.erase(i) : std::next(i);
    }
    return fValues.empty();
  }

  std::map<int, Panel::WidgetSelectionList> &getValues() { return fValues; }
  bool hasValue(int iValue) const { return fValues.find(iValue) != fValues.end(); }
  Panel::WidgetSelectionList &getList(int iValue) { return fValues[iValue]; }

  void resetVisibility()
  {
    for(auto &[id, v]: fValues)
    {
      for(auto w: v.getWidgets())
        w->setVisibility(widget::Visibility::kByProperty);
    }
  }

private:
  Panel::WidgetSelectionList &getOrCreate(int iValue)
  {
    auto iter = fValues.find(iValue);
    if(iter == fValues.end())
      fValues[iValue] = {};
    return fValues[iValue];
  }

private:
  std::map<int, Panel::WidgetSelectionList> fValues{};
};

//------------------------------------------------------------------------
// struct VisibilityProperties
//------------------------------------------------------------------------
struct VisibilityProperties
{
  //! only clears the underlying arrays
  void clear()
  {
    for(auto &[p, vp]: fProperties)
      vp.clear();
  }

  //! Add the attribute in the proper bucket
  void add(widget::attribute::Visibility *iAttribute)
  {
    if(iAttribute)
    {
      auto &path = iAttribute->fSwitch.fValue;
      if(!path.empty())
      {
        auto id = iAttribute->getParent()->getId();
        auto &vp = getOrCreate(path);
        for(auto value: iAttribute->fValues.fValue)
        {
          vp.add(value, iAttribute->getParent());
        }
      }
    }
  }

  //! gets rid of entries with no values
  void prune()
  {
    for(auto i = fProperties.begin(); i != fProperties.end(); )
    {
      i = i->second.prune() ? fProperties.erase(i) : std::next(i);
    }
  }

  inline bool empty() const { return fProperties.empty(); }

  std::map<std::string, VisibilityProperty> &getProperties() { return fProperties; }

private:
  VisibilityProperty &getOrCreate(std::string const &iPropertyPath)
  {
    auto iter = fProperties.find(iPropertyPath);
    if(iter == fProperties.end())
      fProperties[iPropertyPath] = {};
    return fProperties[iPropertyPath];
  }

private:
  std::map<std::string, VisibilityProperty> fProperties{};
};

namespace impl {

//------------------------------------------------------------------------
// impl::removeFromGroupMenuItem
//------------------------------------------------------------------------
static void removeFromGroupMenuItem(AppContext &iCtx,
                                    std::vector<Widget *> const &iWidgets,
                                    std::size_t selectedCount,
                                    std::string const &iPath, int iValue)
{
  if(selectedCount > 0)
  {
    ImGui::SeparatorText("Visibility");
    auto name = fmt::printf("Remove [%d] widgets from %s = %d", selectedCount, iPath, iValue);
    if(ImGui::MenuItem(name.c_str()))
    {
      iCtx.beginUndoTx(name);
      for(auto w: iWidgets)
      {
        if(w->isSelected())
          w->removeVisibility(iPath, iValue);
      }
      iCtx.commitUndoTx();
    }
  }
};

}

//------------------------------------------------------------------------
// Panel::visibilityPropertiesView
//------------------------------------------------------------------------
void Panel::visibilityPropertiesView(AppContext &iCtx)
{
  static auto const handleDrop = [](Panel &iPanel, std::string const &iPath, int iValue) {
    if(ImGui::BeginDragDropTarget())
    {
      if(const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dnd::kWidget))
      {
        RE_EDIT_INTERNAL_ASSERT(payload->DataSize == sizeof(int));
        int id = *(const int *) payload->Data;
        auto widget = iPanel.findWidget(id);
        if(widget)
        {
          if(ImGui::GetIO().KeyAlt)
            widget->addVisibility(iPath, iValue);
          else
            widget->setVisibility(iPath, iValue);
        }
      }
      ImGui::EndDragDropTarget();
    }
  };
  
  bool compactTab = true;

  if(ImGui::BeginTabBar("Visibility", ImGuiTabBarFlags_None))
  {
    if(ImGui::BeginTabItem("Compact"))
    {
      compactTab = true;
      ImGui::EndTabItem();
    }
    if(ImGui::BeginTabItem("Expanded"))
    {
      compactTab = false;
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  if(ImGui::BeginChild("Content"))
  {
    // Implementation note: in order to avoid allocations over and over from frame to frame, we reuse
    // VisibilityProperties which gets cleared at the end of this call (so it is empty) while keeping the
    // allocated arrays
    static VisibilityProperties props{};

    for(auto w: dnz().fSortedByNameWidgets)
    {
      props.add(w->fVisibilityAttribute);
    }
    props.prune();

    if(props.empty())
    {
      ImGui::TextUnformatted("no visibility properties");
    }
    else
    {
      if(compactTab)
      {
        for(auto &[path, prop]: props.getProperties())
        {
          ImGui::PushID(path.c_str());
          auto currentValue = iCtx.getPropertyValueAsInt(path);
          ImGui::SeparatorText(path.c_str());
          if(ReGui::MenuButton())
            ImGui::OpenPopup("Menu");

          if(ImGui::BeginPopup("Menu"))
          {
            if(prop.hasValue(currentValue))
            {
              auto const &widgets = prop.getList(currentValue).getWidgets();
              auto selectedCount = renderWidgetsMenu(iCtx, widgets);
              impl::removeFromGroupMenuItem(iCtx, widgets, selectedCount, path, currentValue);
            }
            else
              renderWidgetsMenu(iCtx, std::vector<Widget *>{});
            ImGui::EndPopup();
          }
          ImGui::SameLine();
          ImGui::BeginGroup();

          auto &localPath = path; // lambda requirement :(
          auto &localProp = prop; // lambda requirement :(
          iCtx.propertyEditViewAsInt(path, [&iCtx, &localPath, &localProp](auto newValue) {
            iCtx.beginUndoTx(fmt::printf("Set Visibility by Property %s = %d", localPath, newValue));
            iCtx.setPropertyValueAsInt(localPath, newValue);
            localProp.resetVisibility();
            iCtx.commitUndoTx();
          });

          if(prop.hasValue(currentValue))
          {
            prop.getList(currentValue).editView(iCtx, *this);
          }

          ImGui::EndGroup();
          handleDrop(*this, path, currentValue);
          ImGui::PopID();
        }
      }
      else
      {
        for(auto &[path, prop]: props.getProperties())
        {
          ImGui::PushID(path.c_str());
          auto currentValue = iCtx.getPropertyValueAsInt(path);
          ImGui::SeparatorText(path.c_str());
          for(auto &[value, selectionList]: prop.getValues())
          {
            ImGui::PushID(value);

            if(ReGui::MenuButton())
              ImGui::OpenPopup("Menu");

            if(ImGui::BeginPopup("Menu"))
            {
              auto const &widgets = selectionList.getWidgets();
              auto selectedCount = renderWidgetsMenu(iCtx, widgets);
              impl::removeFromGroupMenuItem(iCtx, widgets, selectedCount, path, value);
              ImGui::EndPopup();
            }

            ImGui::SameLine();
            ImGui::BeginGroup();

            if(currentValue != value)
            {
              ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().DisabledAlpha / 2.0f);
            }

            if(ImGui::Selectable(re::edit::fmt::printf("value = %d", value).c_str(), currentValue == value))
            {
              iCtx.beginUndoTx(fmt::printf("Set Visibility by Property %s = %d", path, value));
              iCtx.setPropertyValueAsInt(path, value);
              prop.resetVisibility();
              iCtx.commitUndoTx();
            }

            if(currentValue != value)
            {
              ImGui::PopStyleVar();
            }

            ReGui::SpacingY();
            selectionList.editView(iCtx, *this);
            ImGui::EndGroup();
            handleDrop(*this, path, value);
            ReGui::SpacingY();

            ImGui::PopID();
          }
          ImGui::PopID();
        }
      }

    }


    props.clear();
  }
  ImGui::EndChild();
}

//------------------------------------------------------------------------
// Panel::getName
//------------------------------------------------------------------------
char const *Panel::getName() const
{
  switch(fType)
  {
    case PanelType::kFront: return "Front";
    case PanelType::kFoldedFront: return "Folded Front";
    case PanelType::kBack: return "Back";
    case PanelType::kFoldedBack: return "Folded Back";
    default:
      RE_EDIT_FAIL("Not reached");
  }
}

//------------------------------------------------------------------------
// Panel::hdgui2D
//------------------------------------------------------------------------
std::string Panel::hdgui2D() const
{
  auto panelName = toString(fType);

  std::stringstream s{};
  s << "--------------------------------------------------------------------------\n";
  s << fmt::printf("-- %s\n", panelName);
  s << "--------------------------------------------------------------------------\n";
  auto arrayName = fmt::printf("%s_widgets", panelName);
  s << fmt::printf("%s = {}\n", arrayName);
  for(auto id: fWidgetsOrder)
  {
    auto const &w = fWidgets.at(id);
    s << fmt::printf("-- %s\n", w->getName());
    s << fmt::printf("%s[#%s + 1] = %s\n", arrayName, arrayName, w->hdgui2D());
  }

  char const *options = "";
  if(fDisableSampleDropOnPanel && *fDisableSampleDropOnPanel)
    options = R"( options = { "disable_sample_drop_on_panel" },)";

  char const *cableOrigin = "";
  if(fCableOrigin)
    cableOrigin = R"( cable_origin = { node = "CableOrigin" },)";

  s << fmt::printf("%s = jbox.panel{ graphics = { node = \"%s\" },%s%s widgets = %s }\n", panelName, fNodeName, options, cableOrigin, arrayName);

  return s.str();
}

//------------------------------------------------------------------------
// Panel::device2D
//------------------------------------------------------------------------
std::string Panel::device2D() const
{
  auto panelName = toString(fType);

  std::stringstream s{};
  s << "--------------------------------------------------------------------------\n";
  s << fmt::printf("-- %s\n", panelName);
  s << "--------------------------------------------------------------------------\n";
  s << fmt::printf("%s = {}\n", panelName);

  s << "\n-- Main panel\n";
  s << fmt::printf("%s[\"%s\"] = %s\n", panelName, fNodeName, fGraphics.device2D());

  if(!fDecalsOrder.empty())
  {
    s << "\n-- Decals\n";
    s << fmt::printf("re_edit.%s = { decals = {} }\n", panelName);
    int index = 1;
    for(auto id: fDecalsOrder)
    {
      auto const &w = fWidgets.at(id);
      s << fmt::printf("%s[%d] = %s -- %s\n", panelName, index, w->device2D(), w->getName());
      s << fmt::printf("re_edit.%s.decals[%d] = \"%s\"\n", panelName, index, w->getName());
      index++;
    }
  }


  s << "\n-- Widgets\n";
  for(auto id: fWidgetsOrder)
  {
    auto const &w = fWidgets.at(id);
    s << fmt::printf("%s[\"%s\"] = %s\n", panelName, w->getName(), w->device2D());
  }
  if(fCableOrigin)
  {
    s << "\n-- Cable Origin\n";
    s << fmt::printf("%s[\"CableOrigin\"] = { offset = { %d, %d } }\n", panelName,
                     static_cast<int>(fCableOrigin->x), static_cast<int>(fCableOrigin->y));
  }

  return s.str();
}

//------------------------------------------------------------------------
// Panel::collectUsedTexturePaths
//------------------------------------------------------------------------
void Panel::collectUsedTexturePaths(std::set<fs::path> &oPaths) const
{
  if(fGraphics.hasTexture())
    oPaths.emplace(fGraphics.getTexture()->getFilmStrip()->path());

  for(auto &[id, w]: fWidgets)
    w->collectUsedTexturePaths(oPaths);
}

//------------------------------------------------------------------------
// Panel::collectUsedTextureBuiltIns
//------------------------------------------------------------------------
void Panel::collectUsedTextureBuiltIns(std::set<FilmStrip::key_t> &oKeys) const
{
  for(auto &[id, w]: fWidgets)
    w->collectUsedTextureBuiltIns(oKeys);
}

//------------------------------------------------------------------------
// Panel::setDeviceHeightRU
//------------------------------------------------------------------------
void Panel::setDeviceHeightRU(int iDeviceHeightRU)
{
  fDeviceHeightRU = iDeviceHeightRU;
  auto h = isPanelOfType(fType, kPanelTypeAnyUnfolded) ? toPixelHeight(fDeviceHeightRU) : kFoldedDevicePixelHeight;
  fSize.y = static_cast<float>(h);
  fGraphics.fFilter = FilmStrip::bySizeFilter({kDevicePixelWidth, fSize.y});
  fGraphics.markEdited();
  fEdited = true;
}

//------------------------------------------------------------------------
// Panel::setOptions
//------------------------------------------------------------------------
void Panel::setOptions(std::vector<std::string> const &iOptions)
{
  if(iOptions.empty())
    return;

  if(fType != PanelType::kFront)
  {
    RE_EDIT_LOG_WARNING("'options' allowed only on front panel");
    return;
  }

  if(iOptions.size() != 1 || iOptions[0] != "disable_sample_drop_on_panel")
  {
    RE_EDIT_LOG_WARNING("only option possible is 'disable_sample_drop_on_panel'");
    return;
  }

  fDisableSampleDropOnPanel = true;
}

//------------------------------------------------------------------------
// Panel::selectWidgets
//------------------------------------------------------------------------
void Panel::selectWidgets(AppContext &iCtx, ImVec2 const &iPosition1, ImVec2 const &iPosition2)
{
  auto topLeft = iPosition1;
  auto bottomRight = iPosition2;
  if(topLeft.x > bottomRight.x)
    std::swap(topLeft.x, bottomRight.x);
  if(topLeft.y > bottomRight.y)
    std::swap(topLeft.y, bottomRight.y);

  for(auto &[id, w]: fWidgets)
  {
    if(!w->isSelected() && !w->isHidden() && w->overlaps(topLeft, bottomRight))
    {
      w->select();
    }
  }
}

//------------------------------------------------------------------------
// Panel::beforeEachFrame
//------------------------------------------------------------------------
void Panel::beforeEachFrame(AppContext &iCtx)
{
  computeDNZ(&iCtx);
}

//------------------------------------------------------------------------
// Panel::computeDNZ
//------------------------------------------------------------------------
void Panel::computeDNZ(AppContext *iCtx) const
{
  fDNZ.clear();

  for(auto &[_, w]: fWidgets)
  {
    if(iCtx)
      w->computeIsHidden(*iCtx);

    auto tl = w->getTopLeft();
    auto br = w->getBottomRight();

    fDNZ.fSortedByNameWidgets.emplace_back(w.get());

    if(w->isSelected())
    {
      fDNZ.fSelectedWidgets.emplace_back(w.get());
      if(fDNZ.fSelectedRect)
      {
        fDNZ.fSelectedRect->Min.x = std::min(fDNZ.fSelectedRect->Min.x, tl.x);
        fDNZ.fSelectedRect->Min.y = std::min(fDNZ.fSelectedRect->Min.y, tl.y);
        fDNZ.fSelectedRect->Max.x = std::max(fDNZ.fSelectedRect->Max.x, br.x);
        fDNZ.fSelectedRect->Max.y = std::max(fDNZ.fSelectedRect->Max.y, br.y);
      }
      else
      {
        fDNZ.fSelectedRect = ReGui::Rect{tl, br};
      }
    }
  }

  Widget::sortByName(fDNZ.fSortedByNameWidgets);

  fDNZ.markClean();
}

//------------------------------------------------------------------------
// PanelAction::getPanel
//------------------------------------------------------------------------
Panel *PanelAction::getPanel() const
{
  return AppContext::GetCurrent().getPanel(fPanelType);
}

//------------------------------------------------------------------------
// Panel::DNZ::clear
//------------------------------------------------------------------------
void Panel::DNZ::clear()
{
  fSelectedWidgets.clear();
  fSortedByNameWidgets.clear();
  fSelectedRect = std::nullopt;
}

}