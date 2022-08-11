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
  fNodeName{re::mock::fmt::printf("Panel_%s_bg", toString(iType))},
  fCableOrigin{fType == PanelType::kFoldedBack ? std::optional<ImVec2>({kDevicePixelWidth / 2.0f, kFoldedDevicePixelHeight / 2.0f}) : std::nullopt }
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
    case PanelType::kFoldedBack: return "folded_Back";
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
void Panel::draw(DrawContext &iCtx)
{
  std::string dragState{"N/A"};

  ImVec2 backgroundScreenPosition;
  auto const cp = ImGui::GetCursorScreenPos();
  if(fGraphics.hasTexture())
  {
    auto texture = fGraphics.getTexture();
    ImVec2 clickableArea = ImGui::GetContentRegionAvail();
    auto backgroundSize = texture->frameSize() * iCtx.fZoom;
    clickableArea = {std::max(clickableArea.x, backgroundSize.x), std::max(clickableArea.y, backgroundSize.y)};

    iCtx.TextureItem(texture);
    backgroundScreenPosition = ImGui::GetItemRectMin(); // accounts for scrollbar!

    // we use an invisible button to capture mouse events
    ImGui::SetCursorScreenPos(cp); // TextureItem moves the cursor so we restore it
    ImGui::InvisibleButton("canvas", clickableArea, ImGuiButtonFlags_MouseButtonLeft);

    auto mousePos = ImGui::GetMousePos() - backgroundScreenPosition; // accounts for scrollbars
    if(fMouseDrag)
    {
      if(ImGui::IsMouseReleased(ImGuiMouseButton_Left))
      {
        fMouseDrag = std::nullopt;
        dragState = "onRelease";
        endMoveWidgets(mousePos / iCtx.fZoom);
      }
      else
      {
        fMouseDrag->fCurrentPosition = mousePos;
        if(fMouseDrag->fInitialPosition.x != fMouseDrag->fCurrentPosition.x ||
           fMouseDrag->fInitialPosition.y != fMouseDrag->fCurrentPosition.y)
        {
          dragState = "onDrag";
          moveWidgets(mousePos / iCtx.fZoom);
        }
        else
          dragState = "waiting for drag";
      }
    } else if(ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
      fMouseDrag = MouseDrag{mousePos, mousePos};
      auto &io = ImGui::GetIO();
      dragState = "onPressed / " + std::to_string(io.KeyShift);
      selectWidget(iCtx, mousePos / iCtx.fZoom, io.KeyShift);
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
    auto widgetLocation = (ImGui::GetItemRectMin() - cp) / iCtx.fZoom;
    if(renderSelectedWidgetsMenu(selectedWidgets, widgetLocation))
      ImGui::Separator();
    renderAddWidgetMenu(iCtx, widgetLocation);
    ImGui::EndPopup();
  }

  if(fMouseDrag && !selectedWidgets.empty())
  {
    auto min = selectedWidgets[0]->getTopLeft();
    auto max = selectedWidgets[0]->getBottomRight();

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

    auto frameSize = fGraphics.getSize();
    auto color = ImGui::GetColorU32({1,1,0,0.5});
    iCtx.drawLine({0, min.y}, {frameSize.x, min.y}, color);
    iCtx.drawLine({min.x, 0}, {min.x, frameSize.y}, color);
    iCtx.drawLine({0, max.y}, {frameSize.x, max.y}, color);
    iCtx.drawLine({max.x, 0}, {max.x, frameSize.y}, color);
  }

  auto logging = LoggingManager::instance();

  if(logging->isShowDebug())
  {
    auto &io = ImGui::GetIO();
    logging->debug("Shift", "%s", io.KeyShift ? "true" : "false");
    auto mousePos = ImGui::GetMousePos() - backgroundScreenPosition; // accounts for scrollbars
    logging->debug("MouseDown", "%s | %fx%f (%fx%f)", ImGui::IsMouseDown(ImGuiMouseButton_::ImGuiMouseButton_Left) ? "true" : "false", mousePos.x, mousePos.y, backgroundScreenPosition.x, backgroundScreenPosition.y);
    if(fMouseDrag)
      logging->debug("dragState", "%s | fDragStart=%fx%f", dragState.c_str(), fMouseDrag->fCurrentPosition.x, fMouseDrag->fCurrentPosition.y);
    else
      logging->debug("dragState", "%s", dragState.c_str());
  }
}

//------------------------------------------------------------------------
// Panel::drawWidgets
//------------------------------------------------------------------------
void Panel::drawWidgets(DrawContext &iCtx, std::vector<int> const &iOrder)
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
void Panel::drawCableOrigin(DrawContext &iCtx)
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
void Panel::renderAddWidgetMenu(EditContext &iCtx, ImVec2 const &iPosition)
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
    addWidget(std::move(widget));
  }
}

//------------------------------------------------------------------------
// Panel::renderWidgetMenu
//------------------------------------------------------------------------
void Panel::renderWidgetMenu(std::shared_ptr<Widget> const &iWidget)
{
  if(ImGui::MenuItem(re::mock::fmt::printf("%s %s",
                                           iWidget->isSelected() ? "Unselect" : "Select",
                                           iWidget->getName()).c_str()))
    iWidget->toggleSelection();
  if(ImGui::MenuItem(re::mock::fmt::printf("Duplicate %s",
                                           iWidget->getName()).c_str()))
    addWidget(iWidget->copy());
  if(ImGui::MenuItem(re::mock::fmt::printf("Delete %s",
                                           iWidget->getName()).c_str()))
    deleteWidget(iWidget->getId());
}

//------------------------------------------------------------------------
// Panel::renderSelectedWidgetsMenu
//------------------------------------------------------------------------
bool Panel::renderSelectedWidgetsMenu(std::vector<std::shared_ptr<Widget>> const &iSelectedWidgets,
                                      std::optional<ImVec2> iPosition)
{
  bool needSeparator = false;

  std::shared_ptr<Widget> widget{};

  if(iPosition)
  {
    widget = findWidgetOnTopAt(*iPosition);
    if(widget)
    {
      renderWidgetMenu(widget);
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

        renderWidgetMenu(iSelectedWidgets[0]);
      }
    }
    else
    {
      if(needSeparator)
        ImGui::Separator();

      if(ImGui::MenuItem("Clear Selection"))
        clearSelection();
      if(ImGui::MenuItem("Duplicate Widgets"))
      {
        for(auto const &w: iSelectedWidgets)
          addWidget(w->copy());
      }
      if(ImGui::MenuItem("Delete Widgets"))
      {
        for(auto const &w: iSelectedWidgets)
          deleteWidget(w->getId());
      }
    }
    needSeparator = true;
  }
  return needSeparator;
}

//------------------------------------------------------------------------
// Panel::addWidget
//------------------------------------------------------------------------
int Panel::addWidget(std::shared_ptr<Widget> iWidget)
{
  RE_EDIT_INTERNAL_ASSERT(iWidget != nullptr);

  auto const id = fWidgetCounter++;

  iWidget->init(id);

  if(iWidget->isPanelDecal())
    fDecalsOrder.emplace_back(id);
  else
    fWidgetsOrder.emplace_back(id);

  fWidgets[id] = std::move(iWidget);

  return id;
}

//------------------------------------------------------------------------
// Panel::deleteWidget
//------------------------------------------------------------------------
std::pair<std::shared_ptr<Widget>, int> Panel::deleteWidget(int id)
{
  std::shared_ptr<Widget> widget{};
  // we need to extract the widget from the map before removing it so that we can return it!
  std::swap(fWidgets.at(id), widget);
  fWidgets.erase(id);
  if(widget->isPanelDecal())
  {
    auto iter = std::find(fDecalsOrder.begin(), fDecalsOrder.end(), id);
    RE_EDIT_INTERNAL_ASSERT(iter != fDecalsOrder.end());
    auto order = iter - fDecalsOrder.begin();
    fDecalsOrder.erase(iter);
    return {std::move(widget), order};
  }
  else
  {
    auto iter = std::find(fWidgetsOrder.begin(), fWidgetsOrder.end(), id);
    RE_EDIT_INTERNAL_ASSERT(iter != fWidgetsOrder.end());
    auto order = iter - fWidgetsOrder.begin();
    fWidgetsOrder.erase(iter);
    return {std::move(widget), order};
  }
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
void Panel::selectWidget(DrawContext &iCtx, ImVec2 const &iPosition, bool iMultiple)
{
  auto widget = findWidgetOnTopAt(iPosition);
  if(!widget)
  {
    clearSelection();
  }
  else
  {
    if(iMultiple)
    {
      if(!widget->isSelected())
      {
        fLastMovePosition = iPosition;
      }
      widget->toggleSelection();
    }
    else
    {
      fLastMovePosition = iPosition;
      if(!widget->isSelected())
      {
        for(auto &p: fWidgets)
          p.second->setSelected(false);
        widget->setSelected(true);
      }
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
    for(auto &p: fWidgets)
    {
      auto w = p.second;
      if(w != widget)
        w->setSelected(false);
    }
  }

  widget->toggleSelection();
}

//------------------------------------------------------------------------
// Panel::unselectWidget
//------------------------------------------------------------------------
void Panel::unselectWidget(int id)
{
  auto widget = getWidget(id);
  widget->setSelected(false);
}

//------------------------------------------------------------------------
// Panel::clearSelection
//------------------------------------------------------------------------
void Panel::clearSelection()
{
  for(auto &p: fWidgets)
    p.second->setSelected(false);
}

//------------------------------------------------------------------------
// Panel::moveWidgets
//------------------------------------------------------------------------
void Panel::moveWidgets(ImVec2 const &iPosition)
{
  if(fLastMovePosition)
  {
    auto delta = iPosition - fLastMovePosition.value();
    if(delta.x != 0 || delta.y != 0)
    {
      std::for_each(fWidgets.begin(), fWidgets.end(), [&delta](auto const &p) {
        auto &widget = p.second;
        if(widget->isSelected())
        {
          widget->move(delta);
        }
      });
    }
    fLastMovePosition = iPosition;
  }
}

//------------------------------------------------------------------------
// Panel::endMoveWidgets
//------------------------------------------------------------------------
void Panel::endMoveWidgets(ImVec2 const &iPosition)
{
  std::for_each(fWidgets.begin(), fWidgets.end(), [](auto const &p) {
    auto &widget = p.second;
    if(widget->isSelected())
    {
      auto position = widget->getPosition();
      position.x = std::round(position.x);
      position.y = std::round(position.y);
      widget->setPosition(position);
    }
  });

  fLastMovePosition = std::nullopt;
}

//------------------------------------------------------------------------
// Panel::getSelectedWidgets
//------------------------------------------------------------------------
std::vector<std::shared_ptr<Widget>> Panel::getSelectedWidgets() const
{
  std::vector<std::shared_ptr<Widget>> c{};
  for(auto &[n, widget]: fWidgets)
  {
    if(widget->isSelected())
      c.emplace_back(widget);
  }
  return c;
}

//------------------------------------------------------------------------
// PanelState::checkForWidgetErrors
//------------------------------------------------------------------------
void Panel::checkForWidgetErrors(EditContext &iCtx)
{
  for(auto &[n, widget]: fWidgets)
    widget->checkForErrors(iCtx);
}

//------------------------------------------------------------------------
// PanelState::computeIsHidden
//------------------------------------------------------------------------
void Panel::computeIsHidden(DrawContext &iCtx)
{
  for(auto &[n, widget]: fWidgets)
    widget->computeIsHidden(iCtx);
}

//------------------------------------------------------------------------
// Panel::editView
//------------------------------------------------------------------------
void Panel::editView(EditContext &iCtx)
{
  auto selectedWidgets = getSelectedWidgets(); // TODO duplicate call... optimize!!!

  if(ImGui::Begin("Panel Widgets"))
  {
//    static float kItemWidth = 250.0f;
//
//    ImGui::SliderFloat("Width", &kItemWidth, 0, ImGui::GetContentRegionAvail().x);
//    ImGui::PushItemWidth(kItemWidth);
//    ImGui::Text("region = %f | itemWidth = %f", ImGui::GetContentRegionAvail().x, kItemWidth);

    auto size = selectedWidgets.size();
    switch(size)
    {
      case 0:
      {
        ImGui::PushID("Panel");

        if(ImGui::Button("."))
          ImGui::OpenPopup("Menu");

        if(ImGui::BeginPopup("Menu"))
        {
          renderAddWidgetMenu(iCtx);
          ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::Text("%s panel", toString(fType));

        fGraphics.editView(iCtx);

        if(fCableOrigin)
        {
          if(ImGui::TreeNode("Cable Origin"))
          {
            fShowCableOrigin = true;
            ReGui::InputInt("x", &fCableOrigin->x, 1, 5);
            ReGui::InputInt("y", &fCableOrigin->y, 1, 5);
            ImGui::TreePop();
          }
          else
            fShowCableOrigin = false;
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
        break;
      }

      case 1:
      {
        auto w = selectedWidgets[0];

        if(ImGui::Button("."))
          ImGui::OpenPopup("Menu");

        if(ImGui::BeginPopup("Menu"))
        {
          renderSelectedWidgetsMenu(selectedWidgets);
          ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::Text("%s", re::edit::toString(w->getType()));

        ImGui::SameLine();
        if(!w->errorView(iCtx))
          ImGui::NewLine();

        w->editView(iCtx);
        break;
      }

      default:
      {
        if(ImGui::Button("."))
          ImGui::OpenPopup("Menu");

        if(ImGui::BeginPopup("Menu"))
        {
          renderSelectedWidgetsMenu(selectedWidgets);
          ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::Text("%ld selected", selectedWidgets.size());

        auto min = selectedWidgets[0]->getTopLeft();

        std::for_each(selectedWidgets.begin() + 1, selectedWidgets.end(), [&min](auto c) {
          auto pos = c->getTopLeft();
          if(pos.x < min.x)
            min.x = pos.x;
          if(pos.y < min.y)
            min.y = pos.y;
        });

        auto editedMin = min;
        ReGui::InputInt("x", &editedMin.x, 1, 5);
        ImGui::SameLine();
        ImGui::PushID("x");
        if(ImGui::Button("="))
        {
          for(auto &w: selectedWidgets)
          {
            auto position = w->getPosition();
            w->setPosition({editedMin.x, position.y});
          }
        }
        ImGui::PopID();
        ReGui::InputInt("y", &editedMin.y, 1, 5);
        ImGui::SameLine();
        ImGui::PushID("y");
        if(ImGui::Button("="))
        {
          for(auto &w: selectedWidgets)
          {
            auto position = w->getPosition();
            w->setPosition({position.x, editedMin.y});
          }
        }
        ImGui::PopID();
        auto delta = editedMin - min;
        if(delta.x != 0 || delta.y != 0)
        {
          for(auto &w: selectedWidgets)
            w->move(delta);
        }

        break;
      }
    }

//    ImGui::PopItemWidth();
  }
  ImGui::End();

}

//------------------------------------------------------------------------
// Panel::editOrderView
//------------------------------------------------------------------------
void Panel::editOrderView(EditContext &iCtx)
{
  if(ImGui::BeginTabItem("Widgets"))
  {
    if(ImGui::BeginChild("Content"))
      editOrderView(getWidgetsOrder(), [this](int i1, int i2) { swapWidgets(i1, i2); });
    ImGui::EndChild();
    ImGui::EndTabItem();
  }

  if(ImGui::BeginTabItem("Decals"))
  {
    auto ids = getDecalsOrder();
    if(ImGui::BeginChild("Content"))
      editOrderView(ids, [this](int i1, int i2) { swapDecals(i1, i2); });
    ImGui::EndChild();
    ImGui::EndTabItem();
  }
}

//------------------------------------------------------------------------
// Panel::swapWidgets
//------------------------------------------------------------------------
void Panel::swapWidgets(int iIndex1, int iIndex2)
{
  std::swap(fWidgetsOrder[iIndex1], fWidgetsOrder[iIndex2]);
}

//------------------------------------------------------------------------
// Panel::swapDecals
//------------------------------------------------------------------------
void Panel::swapDecals(int iIndex1, int iIndex2)
{
  std::swap(fDecalsOrder[iIndex1], fDecalsOrder[iIndex2]);
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
std::string Panel::hdgui2D(EditContext &iCtx) const
{
  auto panelName = toString(fType);

  std::stringstream s{};
  s << "--------------------------------------------------------------------------\n";
  s << re::mock::fmt::printf("-- %s\n", panelName);
  s << "--------------------------------------------------------------------------\n";
  auto arrayName = re::mock::fmt::printf("%s_widgets", panelName);
  s << re::mock::fmt::printf("%s = {}\n", arrayName);
  for(auto id: fWidgetsOrder)
  {
    auto const &w = fWidgets.at(id);
    s << re::mock::fmt::printf("-- %s\n", w->getName());
    s << re::mock::fmt::printf("%s[#%s + 1] = %s\n", arrayName, arrayName, w->hdgui2D(iCtx));
  }

  if(fCableOrigin)
    s << re::mock::fmt::printf("%s = jbox.panel{ graphics = { node = \"%s\" }, cable_origin = { node = \"CableOrigin\" }, widgets = %s }\n", panelName, fNodeName, arrayName);
  else
    s << re::mock::fmt::printf("%s = jbox.panel{ graphics = { node = \"%s\" }, widgets = %s }\n", panelName, fNodeName, arrayName);

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
  s << re::mock::fmt::printf("-- %s\n", panelName);
  s << "--------------------------------------------------------------------------\n";
  s << re::mock::fmt::printf("%s = {}\n", panelName);
  int index = 1;
  for(auto id: fDecalsOrder)
  {
    auto const &w = fWidgets.at(id);
    s << re::mock::fmt::printf("%s[%d] = %s -- %s\n", panelName, index, w->device2D(), w->fName);
    index++;
  }
  s << re::mock::fmt::printf("%s[\"%s\"] = %s\n", panelName, fNodeName, fGraphics.device2D());
  for(auto id: fWidgetsOrder)
  {
    auto const &w = fWidgets.at(id);
    s << re::mock::fmt::printf("%s[\"%s\"] = %s\n", panelName, w->fName, w->device2D());
  }
  if(fCableOrigin)
    s << re::mock::fmt::printf("%s[\"CableOrigin\"] = {offset = {%d, %d}}\n", panelName,
                               static_cast<int>(fCableOrigin->x), static_cast<int>(fCableOrigin->y));

  return s.str();
}

//------------------------------------------------------------------------
// Panel::editOrderView
//------------------------------------------------------------------------
template<typename F>
void Panel::editOrderView(std::vector<int> const &iOrder, F iOnSwap)
{
  for(int n = 0; n < iOrder.size(); n++)
  {
    auto id = iOrder[n];
    auto const widget = getWidget(id);
    auto item = widget->getName();
    ImGui::Selectable(item.c_str(), widget->isSelected());

    if(ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
      auto &io = ImGui::GetIO();
      selectWidget(id, io.KeyShift);
    }

    if(ImGui::IsItemActive() && !ImGui::IsItemHovered())
    {
      int n_next = n + (ImGui::GetMouseDragDelta(0).y < 0.f ? -1 : 1);
      if(n_next >= 0 && n_next < iOrder.size())
      {
        iOnSwap(n, n_next);
        ImGui::ResetMouseDragDelta();
      }
    }
  }
}

//------------------------------------------------------------------------
// Panel::setSize
//------------------------------------------------------------------------
void Panel::setDeviceHeightRU(int iDeviceHeightRU)
{
  fDeviceHeightRU = iDeviceHeightRU;
  auto h = isPanelOfType(fType, kPanelTypeAnyUnfolded) ? toPixelHeight(fDeviceHeightRU) : kFoldedDevicePixelHeight;
  fGraphics.fFilter = [h](FilmStrip const &f) {
    return f.width() == kDevicePixelWidth && f.height() == h;
  };
}

}