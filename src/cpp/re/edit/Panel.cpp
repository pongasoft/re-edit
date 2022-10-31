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
#include "ReGui.h"
#include "Constants.h"
#include "LoggingManager.h"

namespace re::edit {

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
std::shared_ptr<Widget> Panel::getWidget(int id) const
{
  auto const &w = fWidgets.at(id);
  RE_EDIT_INTERNAL_ASSERT(w != nullptr);
  return w;
}

//------------------------------------------------------------------------
// Panel::draw
//------------------------------------------------------------------------
void Panel::draw(AppContext &iCtx)
{
  ImVec2 backgroundScreenPosition;
  auto const cp = ImGui::GetCursorScreenPos();
  auto &io = ImGui::GetIO();
  auto texture = fGraphics.findTexture();
  ImVec2 clickableArea = ImGui::GetContentRegionAvail();
  auto backgroundSize = getSize() * iCtx.fZoom;
  clickableArea = {std::max(clickableArea.x, backgroundSize.x), std::max(clickableArea.y, backgroundSize.y)};

  if(texture)
    iCtx.TextureItem(texture);
  else
    iCtx.RectFilledItem(ImVec2{}, getSize(), iCtx.getUserPreferences().fWidgetErrorColor);

  backgroundScreenPosition = ImGui::GetItemRectMin(); // accounts for scrollbar!

  // we use an invisible button to capture mouse events
  ImGui::SetCursorScreenPos(cp); // TextureItem moves the cursor so we restore it
  ImGui::InvisibleButton("canvas", clickableArea, ImGuiButtonFlags_MouseButtonLeft);

  auto const mousePos = (ImGui::GetMousePos() - backgroundScreenPosition) / iCtx.fZoom; // accounts for scrollbars
  if(fShiftMouseDrag)
  {
    if(ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
      fShiftMouseDrag = std::nullopt;
    }
    else
    {
      fShiftMouseDrag->fCurrentPosition = mousePos;
      if(fShiftMouseDrag->fInitialPosition.x != fShiftMouseDrag->fCurrentPosition.x ||
         fShiftMouseDrag->fInitialPosition.y != fShiftMouseDrag->fCurrentPosition.y)
      {
        selectWidgets(iCtx, fShiftMouseDrag->fInitialPosition, fShiftMouseDrag->fCurrentPosition);
      }
    }

  }
  else if(fMouseDrag)
  {
    if(ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
      fMouseDrag = std::nullopt;
      endMoveWidgets(iCtx, mousePos);
    }
    else
    {
      bool shouldMoveWidgets = false;
      auto grid = ImGui::GetIO().KeyAlt ? ImVec2{1.0f, 1.0f} : iCtx.fGrid;
      fMouseDrag->fCurrentPosition = mousePos;
      if(std::abs(fMouseDrag->fLastUpdatePosition.x - fMouseDrag->fCurrentPosition.x) >= grid.x)
      {
        fMouseDrag->fLastUpdatePosition.x = fMouseDrag->fCurrentPosition.x;
        shouldMoveWidgets = true;
      }
      if(std::abs(fMouseDrag->fLastUpdatePosition.y - fMouseDrag->fCurrentPosition.y) >= grid.y)
      {
        fMouseDrag->fLastUpdatePosition.y = fMouseDrag->fCurrentPosition.y;
        shouldMoveWidgets = true;
      }
      if(shouldMoveWidgets)
        moveWidgets(iCtx, fMouseDrag->fLastUpdatePosition);
    }
  }
  else if(ImGui::IsItemClicked(ImGuiMouseButton_Left))
  {
    if(io.KeyShift)
      fShiftMouseDrag = MouseDrag{mousePos, mousePos, mousePos};
    else
    {
      fMouseDrag = MouseDrag{mousePos, mousePos, mousePos};
      selectWidget(iCtx, mousePos, ReGui::IsSingleSelectKey(io));
    }
  }

  ImGui::SetCursorScreenPos(cp); // InvisibleButton moves the cursor so we restore it

  // always draw decals first
  drawWidgets(iCtx, fDecalsOrder);

  // then draws the widgets
  drawWidgets(iCtx, fWidgetsOrder);

  // then the cable origin
  drawCableOrigin(iCtx);

  auto selectedWidgets = getSelectedWidgets();

  if(ImGui::BeginPopupContextItem())
  {
    if(!fPopupLocation)
      fPopupLocation = ImGui::GetMousePos() - backgroundScreenPosition; // accounts for scrollbars
    auto widgetLocation = *fPopupLocation / iCtx.fZoom;
    if(renderSelectedWidgetsMenu(iCtx, selectedWidgets, widgetLocation))
      ImGui::Separator();
    renderAddWidgetMenu(iCtx, widgetLocation);
    ImGui::EndPopup();
  }
  else
    fPopupLocation = std::nullopt;

  if(fMouseDrag && !selectedWidgets.empty() && (!ReGui::AnySpecialKey() || io.KeyAlt))
  {
    auto min = selectedWidgets[0]->getTopLeft();
    auto max = selectedWidgets[0]->getBottomRight();

    if(selectedWidgets.size() > 1)
    {
      std::for_each(selectedWidgets.begin() + 1, selectedWidgets.end(), [&min, &max](auto c) {
        auto pos = c->getTopLeft();
        if(pos.x < min.x)
          min.x = pos.x;
        if(pos.y < min.y)
          min.y = pos.y;
        pos = c->getBottomRight();
        if(pos.x > max.x)
          max.x = pos.x;
        if(pos.y > max.y)
          max.y = pos.y;
      });
    }

    auto frameSize = getSize();
    auto color = ImGui::GetColorU32({1,1,0,0.5});
    iCtx.drawLine({0, min.y}, {frameSize.x, min.y}, color);
    iCtx.drawLine({min.x, 0}, {min.x, frameSize.y}, color);
    iCtx.drawLine({0, max.y}, {frameSize.x, max.y}, color);
    iCtx.drawLine({max.x, 0}, {max.x, frameSize.y}, color);
  }

  if(fShiftMouseDrag)
  {
    auto color = ImGui::GetColorU32({1,1,0,1});
    iCtx.drawRect(fShiftMouseDrag->fInitialPosition, fShiftMouseDrag->fCurrentPosition - fShiftMouseDrag->fInitialPosition, color);
  }

  auto logging = LoggingManager::instance();

  if(logging->isShowDebug())
  {
    logging->debug("Key[Ctrl]", "%s", fmt::Bool::to_chars(io.KeyCtrl));
    logging->debug("Key[Shift]", "%s", fmt::Bool::to_chars(io.KeyShift));
    logging->debug("Key[Super]", "%s", fmt::Bool::to_chars(io.KeySuper));
    auto mousePos = ImGui::GetMousePos() - backgroundScreenPosition; // accounts for scrollbars
    logging->debug("MouseDown", "%s | %fx%f (%fx%f)", ImGui::IsMouseDown(ImGuiMouseButton_::ImGuiMouseButton_Left) ? "true" : "false", mousePos.x, mousePos.y, backgroundScreenPosition.x, backgroundScreenPosition.y);
  }
}

//------------------------------------------------------------------------
// Panel::drawWidgets
//------------------------------------------------------------------------
void Panel::drawWidgets(AppContext &iCtx, std::vector<int> const &iOrder)
{
  for(auto id: iOrder)
  {
    auto &w = fWidgets[id];
    w->draw(iCtx);
  }
}

//------------------------------------------------------------------------
// Panel::drawCableOrigin
//------------------------------------------------------------------------
void Panel::drawCableOrigin(AppContext &iCtx)
{
  static constexpr auto kCableOriginSize = 10.0f;
  if(fShowCableOrigin && fCableOrigin)
  {
    iCtx.drawLine({ fCableOrigin->x - kCableOriginSize, fCableOrigin->y - kCableOriginSize},
                  { fCableOrigin->x + kCableOriginSize, fCableOrigin->y + kCableOriginSize},
                  iCtx.getUserPreferences().fSelectedWidgetColor);
    iCtx.drawLine({ fCableOrigin->x - kCableOriginSize, fCableOrigin->y + kCableOriginSize},
                  { fCableOrigin->x + kCableOriginSize, fCableOrigin->y - kCableOriginSize},
                  iCtx.getUserPreferences().fSelectedWidgetColor);
  }
}


//------------------------------------------------------------------------
// Panel::renderAddWidgetMenu
//------------------------------------------------------------------------
void Panel::renderAddWidgetMenu(AppContext &iCtx, ImVec2 const &iPosition)
{
  if(ImGui::BeginMenu("Add Widget"))
  {
    iCtx.renderAddWidgetMenuView(iPosition);
    ImGui::EndMenu();
  }
  if(ImGui::MenuItem("Add Decal"))
  {
    auto widget = Widget::panel_decal();
    widget->setPosition(iPosition);
    addWidget(iCtx, std::move(widget));
  }
}

//------------------------------------------------------------------------
// Panel::renderWidgetMenu
//------------------------------------------------------------------------
void Panel::renderWidgetMenu(AppContext &iCtx, std::shared_ptr<Widget> const &iWidget)
{
  if(ImGui::MenuItem(fmt::printf("%s %s",
                                 iWidget->isSelected() ? "Unselect" : "Select",
                                 iWidget->getName()).c_str()))
    toggleWidgetSelection(iWidget->getId(), true);
  if(ImGui::MenuItem(fmt::printf(ReGui_Prefix(ReGui_Icon_Duplicate, "Duplicate %s"),
                                 iWidget->getName()).c_str()))
    addWidget(iCtx, iWidget->copy());
  if(ImGui::MenuItem(fmt::printf("Delete %s",
                                 iWidget->getName()).c_str()))
    deleteWidget(iCtx, iWidget->getId());

  if(iWidget->canBeShown())
  {
    ImGui::Separator();
    iWidget->renderShowMenu(iCtx);
  }
}

//------------------------------------------------------------------------
// Panel::renderSelectedWidgetsMenu
//------------------------------------------------------------------------
bool Panel::renderSelectedWidgetsMenu(AppContext &iCtx,
                                      std::vector<std::shared_ptr<Widget>> const &iSelectedWidgets,
                                      std::optional<ImVec2> iPosition)
{
  bool needSeparator = false;

  std::shared_ptr<Widget> widget{};

  if(iPosition)
  {
    widget = findWidgetOnTopAt(*iPosition);
    if(widget)
    {
      renderWidgetMenu(iCtx, widget);
      needSeparator = true;
    }
  }
  if(!iSelectedWidgets.empty())
  {
    if(iSelectedWidgets.size() == 1)
    {
      if(iSelectedWidgets[0] != widget)
      {
        if(needSeparator)
          ImGui::Separator();

        renderWidgetMenu(iCtx, iSelectedWidgets[0]);
      }
    }
    else
    {
      if(needSeparator)
        ImGui::Separator();

      if(ImGui::MenuItem("Clear Selection"))
        clearSelection();
      if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Duplicate, "Duplicate Widgets")))
      {
        duplicateWidgets(iCtx, iSelectedWidgets);
      }
      if(ImGui::MenuItem("Delete Widgets"))
      {
        deleteWidgets(iCtx, iSelectedWidgets);
      }
    }
    needSeparator = true;
  }
  return needSeparator;
}

//------------------------------------------------------------------------
// Panel::addWidget
//------------------------------------------------------------------------
int Panel::addWidget(AppContext &iCtx, std::shared_ptr<Widget> iWidget, bool iMakeSelected)
{
  RE_EDIT_INTERNAL_ASSERT(iWidget != nullptr);

  if(iCtx.isUndoEnabled())
    iCtx.addUndoAction(createWidgetsUndoAction(fmt::printf("Add %s", re::edit::toString(iWidget->fType))));

  auto const id = fWidgetCounter++;

  iWidget->init(id);

  if(iWidget->isPanelDecal())
    fDecalsOrder.emplace_back(id);
  else
    fWidgetsOrder.emplace_back(id);

  iWidget->markEdited();
  fEdited = true;

  fWidgets[id] = std::move(iWidget);

  if(iMakeSelected)
  {
    clearSelection();
    selectWidget(id, true);
  }

  return id;
}

//------------------------------------------------------------------------
// Panel::duplicateWidgets
//------------------------------------------------------------------------
void Panel::duplicateWidgets(AppContext &iCtx, std::vector<std::shared_ptr<Widget>> const &iWidgets)
{
  if(iCtx.isUndoEnabled())
    iCtx.addUndoAction(createWidgetsUndoAction(fmt::printf("Duplicate %d widgets", iWidgets.size())));

  iCtx.withUndoDisabled([this, &iCtx, &iWidgets](){
    for(auto const &w: iWidgets)
      addWidget(iCtx, w->copy());
  });
}

//------------------------------------------------------------------------
// Panel::replaceWidget
//------------------------------------------------------------------------
std::shared_ptr<Widget> Panel::replaceWidget(int iWidgetId, std::shared_ptr<Widget> iWidget)
{
  RE_EDIT_INTERNAL_ASSERT(iWidget != nullptr);
  RE_EDIT_INTERNAL_ASSERT(fWidgets.find(iWidgetId) != fWidgets.end());

  iWidget->init(iWidgetId);
  iWidget->fSelected = fWidgets[iWidgetId]->isSelected();
  iWidget->markEdited();
  fEdited = true;
  std::swap(fWidgets[iWidgetId], iWidget);
  fSelectedWidgets = std::nullopt;
  return iWidget;
}

//------------------------------------------------------------------------
// Panel::deleteWidget
//------------------------------------------------------------------------
std::pair<std::shared_ptr<Widget>, int> Panel::deleteWidget(AppContext &iCtx, int id)
{
  if(iCtx.isUndoEnabled())
    iCtx.addUndoAction(createWidgetsUndoAction(fmt::printf("Delete %s", getWidget(id)->getName())));

  std::shared_ptr<Widget> widget{};
  unselectWidget(id);
  // we need to extract the widget from the map before removing it so that we can return it!
  std::swap(fWidgets.at(id), widget);
  fWidgets.erase(id);
  if(widget->isPanelDecal())
  {
    auto iter = std::find(fDecalsOrder.begin(), fDecalsOrder.end(), id);
    RE_EDIT_INTERNAL_ASSERT(iter != fDecalsOrder.end());
    auto order = iter - fDecalsOrder.begin();
    fDecalsOrder.erase(iter);
    return {std::move(widget), static_cast<int>(order)};
  }
  else
  {
    auto iter = std::find(fWidgetsOrder.begin(), fWidgetsOrder.end(), id);
    RE_EDIT_INTERNAL_ASSERT(iter != fWidgetsOrder.end());
    auto order = iter - fWidgetsOrder.begin();
    fWidgetsOrder.erase(iter);
    return {std::move(widget), static_cast<int>(order)};
  }
}

//------------------------------------------------------------------------
// Panel::deleteWidgets
//------------------------------------------------------------------------
void Panel::deleteWidgets(AppContext &iCtx, std::vector<std::shared_ptr<Widget>> const &iWidgets)
{
  if(iCtx.isUndoEnabled())
    iCtx.addUndoAction(createWidgetsUndoAction(fmt::printf("Delete %d widgets", iWidgets.size())));

  iCtx.withUndoDisabled([this, &iCtx, &iWidgets](){
    for(auto const &w: iWidgets)
      deleteWidget(iCtx, w->getId());
  });
}


//------------------------------------------------------------------------
// Panel::findWidgetOnTopAt
//------------------------------------------------------------------------
std::shared_ptr<Widget> Panel::findWidgetOnTopAt(std::vector<int> const &iOrder, ImVec2 const &iPosition) const
{
  auto ci = std::find_if(iOrder.rbegin(), iOrder.rend(), [this, &iPosition](auto const id) {
    auto const &w = fWidgets.at(id);
    return !w->isHidden() && w->contains(iPosition);
  });

  if(ci == iOrder.rend())
    return nullptr;
  else
    return fWidgets.at(*ci);
}

//------------------------------------------------------------------------
// Panel::findWidgetOnTopAt
//------------------------------------------------------------------------
std::shared_ptr<Widget> Panel::findWidgetOnTopAt(ImVec2 const &iPosition) const
{
  auto widget = findWidgetOnTopAt(fWidgetsOrder, iPosition);
  if(!widget)
    widget = findWidgetOnTopAt(fDecalsOrder, iPosition);
  return widget;
}

//------------------------------------------------------------------------
// Panel::selectWidget
//------------------------------------------------------------------------
void Panel::selectWidget(AppContext &iCtx, ImVec2 const &iPosition, bool iMultiSelectKey)
{
  auto widget = findWidgetOnTopAt(iPosition);
  if(!widget)
  {
    if(!iMultiSelectKey)
      clearSelection();
  }
  else
  {
    if(iMultiSelectKey)
    {
      if(!widget->isSelected())
        fLastMovePosition = iPosition;
      toggleWidgetSelection(widget->getId(), true);
    }
    else
    {
      fLastMovePosition = iPosition;
      if(!widget->isSelected())
        selectWidget(widget->getId(), false);
    }
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
    fSelectedWidgets = std::nullopt;
  }
  else
  {
    if(widget->fSelected)
      unselectWidget(id);
    widget->fSelected = true;
    fSelectedWidgets = std::nullopt;
  }
}

//------------------------------------------------------------------------
// Panel::toggleWidgetSelection
//------------------------------------------------------------------------
void Panel::toggleWidgetSelection(int id, bool iMultiple)
{
  auto widget = getWidget(id);
  if(widget->isSelected())
    unselectWidget(id);
  else
    selectWidget(id, iMultiple);
}

//------------------------------------------------------------------------
// Panel::unselectWidget
//------------------------------------------------------------------------
void Panel::unselectWidget(int id)
{
  auto widget = getWidget(id);
  widget->fSelected = false;
  fSelectedWidgets = std::nullopt;
}

//------------------------------------------------------------------------
// Panel::clearSelection
//------------------------------------------------------------------------
void Panel::clearSelection()
{
  for(auto &p: fWidgets)
    p.second->fSelected = false;
  fSelectedWidgets = std::nullopt;
}

//------------------------------------------------------------------------
// Panel::moveWidgets
//------------------------------------------------------------------------
void Panel::moveWidgets(AppContext &iCtx, ImVec2 const &iPosition)
{
  if(fLastMovePosition)
  {
    auto delta = iPosition - fLastMovePosition.value();
    if(delta.x != 0 || delta.y != 0)
    {
      auto selectedWidgets = getSelectedWidgets();
      if(iCtx.beginUndoTx(fmt::printf("Move %d widgets", selectedWidgets.size()), &fLastMovePosition))
      {
        for(auto &widget: selectedWidgets)
          iCtx.addUndoWidgetChange(widget.get(), fmt::printf("Move %s", widget->getName()));
        iCtx.commitUndoTx();
      }

      for(auto &widget: selectedWidgets)
      {
        widget->move(delta);
      }
    }
    fLastMovePosition = iPosition;
  }
}

//------------------------------------------------------------------------
// Panel::endMoveWidgets
//------------------------------------------------------------------------
void Panel::endMoveWidgets(AppContext &iCtx, ImVec2 const &iPosition)
{
  for(auto &widget: getSelectedWidgets())
  {
    auto position = widget->getPosition();
    position.x = std::round(position.x);
    position.y = std::round(position.y);
    widget->setPosition(position);
  }

  iCtx.resetUndoMergeKey();

  fLastMovePosition = std::nullopt;
}

//------------------------------------------------------------------------
// Panel::getSelectedWidgets
//------------------------------------------------------------------------
std::vector<std::shared_ptr<Widget>> Panel::getSelectedWidgets() const
{
  if(fSelectedWidgets)
    return *fSelectedWidgets;
  std::vector<std::shared_ptr<Widget>> c{};
  for(auto &[id, w]: fWidgets)
  {
    if(w->isSelected())
      c.emplace_back(w);
  }
  fSelectedWidgets = c;
  return c;
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
    if(fGraphics.checkForErrors(iCtx))
      addAllErrors("graphics", fGraphics);

    for(auto &[n, widget]: fWidgets)
    {
      if(widget->checkForErrors(iCtx))
        addAllErrors(widget->getName(), *widget);
    }

    fEdited = false;
  }

  return hasErrors();
}

//------------------------------------------------------------------------
// PanelState::computeIsHidden
//------------------------------------------------------------------------
void Panel::computeIsHidden(AppContext &iCtx)
{
  for(auto &[n, widget]: fWidgets)
    widget->computeIsHidden(iCtx);
}

//------------------------------------------------------------------------
// Panel::editView
//------------------------------------------------------------------------
void Panel::editView(AppContext &iCtx)
{
  auto selectedWidgets = getSelectedWidgets();

  if(ImGui::Begin("Panel Widgets"))
  {
//
//    ImGui::SliderFloat("Width", &kItemWidth, 0, ImGui::GetContentRegionAvail().x);
    ImGui::PushItemWidth(iCtx.fItemWidth);
//    ImGui::Text("region = %f | itemWidth = %f", ImGui::GetContentRegionAvail().x, kItemWidth);

    auto size = selectedWidgets.size();
    switch(size)
    {
      case 0:
      {
        editNoSelectionView(iCtx);
        break;
      }

      case 1:
      {
        editSingleSelectionView(iCtx, selectedWidgets[0]);
        break;
      }

      default:
      {
        editMultiSelectionView(iCtx, selectedWidgets);
        break;
      }
    }

    ImGui::PopItemWidth();
  }
  ImGui::End();

}

//------------------------------------------------------------------------
// Panel::editNoSelectionView
//------------------------------------------------------------------------
void Panel::editNoSelectionView(AppContext &iCtx)
{
  fEdited = false;

  ImGui::PushID("Panel");

  if(ReGui::MenuButton())
    ImGui::OpenPopup("Menu");

  if(ImGui::BeginPopup("Menu"))
  {
    renderAddWidgetMenu(iCtx);
    ImGui::EndPopup();
  }

  ImGui::SameLine();
  ImGui::Text("%s panel", toString(fType));

  errorViewSameLine();

  fGraphics.editView(iCtx);
  fEdited |= fGraphics.isEdited();
  fGraphics.errorViewSameLine();

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
        iCtx.addOrMergeUndoLambda(&fCableOrigin, *fCableOrigin, cableOrigin,
                                  fmt::printf("Update cable_origin"),
                                  [panelType = fType](UndoAction *iAction, auto const &iValue)
                                  {
                                    auto panel = AppContext::GetCurrent().getPanel(panelType);
                                    panel->setCableOrigin(iValue);
                                  });
        fCableOrigin = cableOrigin;
        fEdited = true;
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
        iCtx.addOrMergeUndoLambda(&fDisableSampleDropOnPanel, *fDisableSampleDropOnPanel, b,
                                  fmt::printf("Update disable_sample_drop_on_panel"),
                                  [panelType = fType](UndoAction *iAction, auto const &iValue)
                                  {
                                    auto panel = AppContext::GetCurrent().getPanel(panelType);
                                    panel->fDisableSampleDropOnPanel = iValue;
                                    panel->fEdited = true;
                                  });
        fDisableSampleDropOnPanel = b;
        fEdited = true;
      }
      ImGui::TreePop();
    }
  }

  if(ImGui::TreeNode("hdgui2D"))
  {
    auto windowSize = ImGui::GetWindowSize();
    ImGui::PushTextWrapPos(windowSize.x);
    ImGui::TextUnformatted(hdgui2D(iCtx).c_str());
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
void Panel::editSingleSelectionView(AppContext &iCtx, std::shared_ptr<Widget> const &iWidget)
{
  fEdited = false;

  if(ReGui::MenuButton())
    ImGui::OpenPopup("Menu");

  if(ImGui::BeginPopup("Menu"))
  {
    renderWidgetMenu(iCtx, iWidget);
    ImGui::EndPopup();
  }

  ImGui::SameLine();
  ImGui::Text("%s", re::edit::toString(iWidget->getType()));

  if(iWidget->isHidden())
  {
    ImGui::SameLine();
    ImGui::Text(ReGui::kHiddenWidgetIcon);
  }

  iWidget->errorViewSameLine();

  iWidget->editView(iCtx);

  if(iWidget->isEdited())
    fEdited = true;
}


//------------------------------------------------------------------------
// Panel::editMultiSelectionView
//------------------------------------------------------------------------
void Panel::editMultiSelectionView(AppContext &iCtx, std::vector<std::shared_ptr<Widget>> const &iSelectedWidgets)
{
  fEdited = false;

  if(ReGui::MenuButton())
    ImGui::OpenPopup("Menu");

  if(ImGui::BeginPopup("Menu"))
  {
    renderSelectedWidgetsMenu(iCtx, iSelectedWidgets);
    ImGui::EndPopup();
  }

  ImGui::SameLine();
  ImGui::Text("%ld selected", iSelectedWidgets.size());

  auto min = iSelectedWidgets[0]->getTopLeft();
  auto max = iSelectedWidgets[0]->getBottomRight();

  std::for_each(iSelectedWidgets.begin() + 1, iSelectedWidgets.end(), [&min, &max](auto c) {
    auto pos = c->getTopLeft();
    if(pos.x < min.x)
      min.x = pos.x;
    if(pos.y < min.y)
      min.y = pos.y;
    pos = c->getBottomRight();
    if(pos.x > max.x)
      max.x = pos.x;
    if(pos.y > max.y)
      max.y = pos.y;
  });

  auto editedMin = min;

  if(ReGui::InputInt("x", &editedMin.x, 1, 5))
  {
    fEdited = true;
    if(iCtx.beginUndoTx("Move Widgets", this))
    {
      for(auto &widget: getSelectedWidgets())
        iCtx.addUndoWidgetChange(widget.get(), fmt::printf("Move %s", widget->getName()));
      iCtx.commitUndoTx();
    }
  }

  if(ReGui::InputInt("y", &editedMin.y, 1, 5))
  {
    fEdited = true;
    if(iCtx.beginUndoTx("Move Widgets", this))
    {
      for(auto &widget: getSelectedWidgets())
        iCtx.addUndoWidgetChange(widget.get(), fmt::printf("Move %s", widget->getName()));
      iCtx.commitUndoTx();
    }
  }

  auto delta = editedMin - min;
  if(delta.x != 0 || delta.y != 0)
  {
    fEdited = true;
    for(auto &w: iSelectedWidgets)
      w->move(delta);
  }

  if(ImGui::TreeNode("Alignment"))
  {
    constexpr ImVec2 smallButtonSize{80, 0};
    ImVec2 bigButtonSize{smallButtonSize.x * 2.0f + ImGui::GetStyle().ItemSpacing.x, 0};

    if(ImGui::Button("Top", bigButtonSize))
    {
      fEdited = true;
      iCtx.beginUndoTx("Align Widgets Top");
      for(auto &w: iSelectedWidgets)
      {
        iCtx.addUndoWidgetChange(w.get(), fmt::printf("Align %s Top", w->getName()));
        auto position = w->getPosition();
        w->setPosition({position.x, min.y});
      }
      iCtx.commitUndoTx();
    }

    if(ImGui::Button("Left", smallButtonSize))
    {
      fEdited = true;
      iCtx.beginUndoTx("Align Widgets Left");
      for(auto &w: iSelectedWidgets)
      {
        iCtx.addUndoWidgetChange(w.get(), fmt::printf("Align %s Left", w->getName()));
        auto position = w->getPosition();
        w->setPosition({min.x, position.y});
      }
      iCtx.commitUndoTx();
    }

    ImGui::SameLine();

    if(ImGui::Button("Right", smallButtonSize))
    {
      fEdited = true;
      iCtx.beginUndoTx("Align Widgets Right");
      for(auto &w: iSelectedWidgets)
      {
        iCtx.addUndoWidgetChange(w.get(), fmt::printf("Align %s Right", w->getName()));
        auto position = w->getPosition();
        w->setPosition({max.x - w->getSize().x, position.y});
      }
      iCtx.commitUndoTx();
    }

    if(ImGui::Button("Bottom", bigButtonSize))
    {
      fEdited = true;
      iCtx.beginUndoTx("Align Widgets Bottom");
      for(auto &w: iSelectedWidgets)
      {
        iCtx.addUndoWidgetChange(w.get(), fmt::printf("Align %s Bottom", w->getName()));
        auto position = w->getPosition();
        w->setPosition({position.x, max.y - w->getSize().y});
      }
      iCtx.commitUndoTx();
    }

    ImGui::TreePop();
  }
}

//------------------------------------------------------------------------
// Panel::MultiSelectionList::getSelectedWidgets
//------------------------------------------------------------------------
std::vector<std::shared_ptr<Widget>> Panel::MultiSelectionList::getSelectedWidgets() const
{
  std::vector<std::shared_ptr<Widget>> sublist{};
  sublist.reserve(fList.size());
  for(auto id: fList)
  {
    auto widget = fPanel.getWidget(id);
    if(widget->isSelected())
      sublist.emplace_back(widget);
  }
  return sublist;
}

//------------------------------------------------------------------------
// Panel::MultiSelectionList::editView
//------------------------------------------------------------------------
void Panel::MultiSelectionList::handleClick(std::shared_ptr<Widget> const &iWidget, bool iRangeSelectKey, bool iMultiSelectKey)
{
  auto id = iWidget->getId();

  // when iMultiSelectKey is held => multiple selection
  if(iMultiSelectKey)
  {
    if(!iWidget->isSelected())
    {
      iWidget->fSelected = true;
      fPanel.fSelectedWidgets = std::nullopt;
      fLastSelected = id;
    }
    else
    {
      iWidget->fSelected = false;
      fPanel.fSelectedWidgets = std::nullopt;
      fLastSelected = std::nullopt;
    }
    return;
  }

  // when iRangeSelectKey is held => add all properties between fLastSelected and this one
  if(iRangeSelectKey && fLastSelected && std::find(fList.begin(), fList.end(), *fLastSelected) != fList.end())
  {
    bool copy = false;
    for(auto elt: fList)
    {
      if(id != *fLastSelected && (elt == id || elt == *fLastSelected))
      {
        copy = !copy;
        fPanel.getWidget(elt)->fSelected = true;
      }
      else if(copy)
        fPanel.getWidget(elt)->fSelected = true;
    }

    fPanel.fSelectedWidgets = std::nullopt;
    fLastSelected = id;
    return;
  }

  // neither shift nor control is held => single selection (deselect all others)
  auto wasSelected = iWidget->fSelected;
  fPanel.clearSelection();
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
// Panel::MultiSelectionList::editView
//------------------------------------------------------------------------
void Panel::MultiSelectionList::editView(AppContext &iCtx)
{
  if(ImGui::Button("All"))
    selectAll();
  ImGui::SameLine();
  if(ImGui::Button("Clr"))
    clearSelection();
  ImGui::SameLine();
  if(ImGui::Button("Up "))
    moveSelectionUp();
  ImGui::SameLine();
  if(ImGui::Button("Dwn"))
    moveSelectionDown();
  ImGui::SameLine();
  if(ImGui::Button("Dup"))
    duplicateSelection(iCtx);
  ImGui::SameLine();
  if(ImGui::Button("Del"))
    deleteSelection(iCtx);

  ImGui::Separator();

  if(ImGui::BeginChild("List", ImVec2{}, false, ImGuiWindowFlags_HorizontalScrollbar))
  {
    for(auto id: fList)
    {
      auto widget = fPanel.getWidget(id);
      if(ImGui::Selectable(widget->getName().c_str(), widget->isSelected(), ImGuiSelectableFlags_AllowDoubleClick))
      {
        auto io = ImGui::GetIO();
        if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
          widget->showIfHidden(iCtx);
        else
          handleClick(widget, io.KeyShift, ReGui::IsSingleSelectKey(io));
      }
      if(ImGui::BeginPopupContextItem())
      {
        fPanel.renderWidgetMenu(iCtx, widget);
        ImGui::EndPopup();
      }
      if(widget->isHidden())
      {
        ImGui::SameLine();
        ImGui::Text(ReGui::kHiddenWidgetIcon);
      }
      widget->errorViewSameLine();
    }
  }
  ImGui::EndChild();
}

//------------------------------------------------------------------------
// Panel::MultiSelectionList::moveSelectionUp
//------------------------------------------------------------------------
void Panel::MultiSelectionList::moveSelectionUp()
{
  auto selectedWidgets = getSelectedWidgets();

  for(auto &w: selectedWidgets)
  {
    auto iter = std::find(fList.begin(), fList.end(), w->getId());

    // already at the top
    if(iter == fList.begin())
      break; // abort loop

    std::swap(*(iter - 1), *iter);
  }

}

//------------------------------------------------------------------------
// Panel::MultiSelectionList::moveSelectionDown
//------------------------------------------------------------------------
void Panel::MultiSelectionList::moveSelectionDown()
{
  auto selectedWidgets = getSelectedWidgets();

  // we need to iterate backward
  for(auto i = selectedWidgets.rbegin(); i != selectedWidgets.rend(); i++)
  {
    auto widget = *i;

    auto iter = std::find(fList.begin(), fList.end(), widget->getId());

    // already at the bottom
    if(iter + 1 == fList.end())
      break; // abort loop

    std::swap(*(iter + 1), *iter);
  }

}

//------------------------------------------------------------------------
// Panel::MultiSelectionList::selectAll
//------------------------------------------------------------------------
void Panel::MultiSelectionList::selectAll()
{
  fPanel.clearSelection();
  for(auto id: fList)
    fPanel.getWidget(id)->fSelected = true;
  fLastSelected = std::nullopt;
}

//------------------------------------------------------------------------
// Panel::MultiSelectionList::clearSelection
//------------------------------------------------------------------------
void Panel::MultiSelectionList::clearSelection()
{
  fPanel.clearSelection();
  fLastSelected = std::nullopt;
}

//------------------------------------------------------------------------
// Panel::MultiSelectionList::duplicateSelection
//------------------------------------------------------------------------
void Panel::MultiSelectionList::duplicateSelection(AppContext &iCtx)
{
  fPanel.duplicateWidgets(iCtx, getSelectedWidgets());
  fLastSelected = std::nullopt;
}

//------------------------------------------------------------------------
// Panel::MultiSelectionList::deleteSelection
//------------------------------------------------------------------------
void Panel::MultiSelectionList::deleteSelection(AppContext &iCtx)
{
  fPanel.deleteWidgets(iCtx, getSelectedWidgets());
  fLastSelected = std::nullopt;
}

//------------------------------------------------------------------------
// Panel::editOrderView
//------------------------------------------------------------------------
void Panel::editOrderView(AppContext &iCtx)
{
  if(ImGui::BeginTabItem("Widgets"))
  {
    fWidgetsSelectionList.editView(iCtx);
    ImGui::EndTabItem();
  }

  if(ImGui::BeginTabItem("Decals"))
  {
    fDecalsSelectionList.editView(iCtx);
    ImGui::EndTabItem();
  }
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
std::string Panel::hdgui2D(AppContext &iCtx) const
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
    s << fmt::printf("%s[#%s + 1] = %s\n", arrayName, arrayName, w->hdgui2D(iCtx));
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

  if(!fDecalsOrder.empty())
  {
    s << "re_edit = re_edit or {}\n";
    s << fmt::printf("re_edit.%s = { decals = {} }\n", panelName);
    int index = 1;
    for(auto id: fDecalsOrder)
    {
      auto const &w = fWidgets.at(id);
      s << fmt::printf("%s[%d] = %s -- %s\n", panelName, index, w->device2D(), w->fName);
      s << fmt::printf("re_edit.%s.decals[%d] = \"%s\"\n", panelName, index, w->fName);
      index++;
    }
  }

  s << fmt::printf("%s[\"%s\"] = %s\n", panelName, fNodeName, fGraphics.device2D());
  for(auto id: fWidgetsOrder)
  {
    auto const &w = fWidgets.at(id);
    s << fmt::printf("%s[\"%s\"] = %s\n", panelName, w->fName, w->device2D());
  }
  if(fCableOrigin)
    s << fmt::printf("%s[\"CableOrigin\"] = {offset = {%d, %d}}\n", panelName,
                     static_cast<int>(fCableOrigin->x), static_cast<int>(fCableOrigin->y));

  return s.str();
}

//------------------------------------------------------------------------
// Panel::setDeviceHeightRU
//------------------------------------------------------------------------
void Panel::setDeviceHeightRU(int iDeviceHeightRU)
{
  fDeviceHeightRU = iDeviceHeightRU;
  auto h = isPanelOfType(fType, kPanelTypeAnyUnfolded) ? toPixelHeight(fDeviceHeightRU) : kFoldedDevicePixelHeight;
  fSize.y = static_cast<float>(h);
  fGraphics.fFilter = [h](FilmStrip const &f) {
    return f.width() == kDevicePixelWidth && f.height() == h;
  };
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
// Panel::freezeWidgets
//------------------------------------------------------------------------
std::shared_ptr<Panel::PanelWidgets> Panel::freezeWidgets() const
{
  std::unique_ptr<Panel::PanelWidgets> ws{new Panel::PanelWidgets()};
  ws->fWidgets = fWidgets;
  ws->fWidgetsOrder = fWidgetsOrder;
  ws->fDecalsOrder = fDecalsOrder;
  return ws;
}

//------------------------------------------------------------------------
// Panel::thawWidgets
//------------------------------------------------------------------------
std::shared_ptr<Panel::PanelWidgets> Panel::thawWidgets(std::shared_ptr<PanelWidgets> const &iPanelWidgets)
{
  std::unique_ptr<Panel::PanelWidgets> ws{new Panel::PanelWidgets()};
  ws->fWidgets = std::move(fWidgets);
  ws->fWidgetsOrder = std::move(fWidgetsOrder);
  ws->fDecalsOrder = std::move(fDecalsOrder);
  fWidgets = iPanelWidgets->fWidgets;
  fWidgetsOrder = iPanelWidgets->fWidgetsOrder;
  fDecalsOrder = iPanelWidgets->fDecalsOrder;
  fSelectedWidgets = std::nullopt;
  return ws;
}

//------------------------------------------------------------------------
// Panel::createWidgetsUndoAction
//------------------------------------------------------------------------
std::shared_ptr<UndoAction> Panel::createWidgetsUndoAction(std::string const &iDescription) const
{
  auto pw = freezeWidgets();
  auto action = UndoAction::createFromLambda([pw = std::move(pw)](UndoAction *iAction) {
    auto w = AppContext::GetCurrent().getPanel(iAction->fPanelType)->thawWidgets(pw);
    return RedoAction::createFromLambda([w2 = std::move(w)](RedoAction *iAction) {
      AppContext::GetCurrent().getPanel(iAction->fUndoAction->fPanelType)->thawWidgets(w2);
    });
  });
  action->fDescription = iDescription;
  action->fPanelType = fType;
  return action;

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
    if(!w->isSelected() && w->overlaps(topLeft, bottomRight))
    {
      w->fSelected = true;
      fSelectedWidgets = std::nullopt;
    }
  }
}

//------------------------------------------------------------------------
// Panel::reloadTextures
//------------------------------------------------------------------------
void Panel::reloadTextures()
{
  fGraphics.fDNZTexture = nullptr;
  for(auto &[id, w]: fWidgets)
  {
    w->fGraphics->fDNZTexture = nullptr;
  }
  markEdited();
}


}