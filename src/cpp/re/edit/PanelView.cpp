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

#include "PanelView.h"

namespace re::edit {

//------------------------------------------------------------------------
// PanelView::draw
//------------------------------------------------------------------------
void PanelView::draw(DrawContext &iCtx)
{
  std::string dragState{"N/A"};

  ImVec2 backgroundScreenPosition;
  auto const cp = ImGui::GetCursorScreenPos();
  if(fBackground)
  {
    ImVec2 clickableArea = ImGui::GetContentRegionAvail();
    auto backgroundSize = fBackground->frameSize() * iCtx.getZoom();
    clickableArea = {std::max(clickableArea.x, backgroundSize.x), std::max(clickableArea.y, backgroundSize.y)};

    iCtx.TextureItem(fBackground.get());
    backgroundScreenPosition = ImGui::GetItemRectMin(); // accounts for scrollbar!

    // we use an invisible button to capture mouse events
    ImGui::SetCursorScreenPos(cp); // TextureItem moves the cursor so we restore it
    ImGui::InvisibleButton("canvas", clickableArea, ImGuiButtonFlags_MouseButtonLeft);

    auto mousePos = ImGui::GetMousePos() - backgroundScreenPosition; // accounts for scrollbars
    if(fMouseDrag)
    {
      if(ImGui::IsMouseReleased(ImGuiMouseButton_::ImGuiMouseButton_Left))
      {
        fMouseDrag = std::nullopt;
        dragState = "onRelease";
        endMoveWidgets(mousePos / iCtx.getZoom());
      }
      else
      {
        fMouseDrag->fCurrentPosition = mousePos;
        if(fMouseDrag->fInitialPosition.x != fMouseDrag->fCurrentPosition.x ||
           fMouseDrag->fInitialPosition.y != fMouseDrag->fCurrentPosition.y)
        {
          dragState = "onDrag";
          moveWidgets(mousePos / iCtx.getZoom());
        }
        else
          dragState = "waiting for drag";
      }
    } else if(ImGui::IsItemClicked(ImGuiMouseButton_::ImGuiMouseButton_Left))
    {
      fMouseDrag = MouseDrag{mousePos, mousePos};
      auto &io = ImGui::GetIO();
      dragState = "onPressed / " + std::to_string(io.KeyShift);
      selectWidget(mousePos / iCtx.getZoom(), io.KeyShift);
    }
  }
  ImGui::SetCursorScreenPos(cp); // InvisibleButton moves the cursor so we restore it
  for(auto &widget: fWidgets)
  {
    auto &w = widget.second;
    w->getView().draw(iCtx);
  }

  auto selectedWidgets = getSelectedWidgets();

  if(fMouseDrag && !selectedWidgets.empty())
  {
    auto min = selectedWidgets[0]->getView().getTopLeft();
    auto max = selectedWidgets[0]->getView().getBottomRight();

    std::for_each(selectedWidgets.begin() + 1, selectedWidgets.end(), [&min, &max](auto c) {
      auto pos = c->getView().getTopLeft();
      if(pos.x < min.x)
        min.x = pos.x;
      if(pos.y < min.y)
        min.y = pos.y;
      pos = c->getView().getBottomRight();
      if(pos.x > max.x)
        max.x = pos.x;
      if(pos.y > max.y)
        max.y = pos.y;
    });

    auto frameSize = fBackground->frameSize();
    auto color = ImGui::GetColorU32({1,1,0,0.5});
    iCtx.drawLine({0, min.y}, {frameSize.x, min.y}, color);
    iCtx.drawLine({min.x, 0}, {min.x, frameSize.y}, color);
    iCtx.drawLine({0, max.y}, {frameSize.x, max.y}, color);
    iCtx.drawLine({max.x, 0}, {max.x, frameSize.y}, color);
  }

  if(ImGui::Begin("Debug"))
  {
    auto &io = ImGui::GetIO();
    ImGui::Text("Shift=%s", io.KeyShift ? "true" : "false");
    auto mousePos = ImGui::GetMousePos() - backgroundScreenPosition; // accounts for scrollbars
    ImGui::Text("MouseDown=%s | %fx%f (%fx%f)", ImGui::IsMouseDown(ImGuiMouseButton_::ImGuiMouseButton_Left) ? "true" : "false", mousePos.x, mousePos.y, backgroundScreenPosition.x, backgroundScreenPosition.y);
    if(fMouseDrag)
      ImGui::Text("dragState=%s | fDragStart=%fx%f", dragState.c_str(), fMouseDrag->fCurrentPosition.x, fMouseDrag->fCurrentPosition.y);
    else
      ImGui::Text("dragState=%s", dragState.c_str());
  }
  ImGui::End();

  if(ImGui::Begin("Widgets"))
  {
    ImGui::SliderFloat("zoom", &iCtx.getZoom(), 0.25f, 1.5f);
    ImGui::Checkbox("Show Widget Border", &iCtx.getUserPreferences().fShowWidgetBorder);
  }
  ImGui::End();
}

//------------------------------------------------------------------------
// PanelView::addWidget
//------------------------------------------------------------------------
int PanelView::addWidget(std::unique_ptr<Widget> iWidget)
{
  return fWidgets.add(std::move(iWidget));
}

//------------------------------------------------------------------------
// PanelView::selectWidget
//------------------------------------------------------------------------
void PanelView::selectWidget(ImVec2 const &iPosition, bool iMultiple)
{
  auto ci = std::find_if(fWidgets.begin(), fWidgets.end(), [&iPosition](auto const &p) { return p.second->getView().contains(iPosition); });
  if(ci == fWidgets.end())
  {
    for(auto &p: fWidgets)
      p.second->getView().setSelected(false);
  }
  else
  {
    auto &widget = ci->second;
    if(iMultiple)
    {
      if(!widget->getView().isSelected())
      {
        fLastMovePosition = iPosition;
      }
      widget->getView().toggleSelection();
    }
    else
    {
      fLastMovePosition = iPosition;
      if(!widget->getView().isSelected())
      {
        for(auto &p: fWidgets)
          p.second->getView().setSelected(false);
        widget->getView().setSelected(true);
      }
    }
  }
}

//------------------------------------------------------------------------
// PanelView::moveWidgets
//------------------------------------------------------------------------
void PanelView::moveWidgets(ImVec2 const &iPosition)
{
  if(fLastMovePosition)
  {
    auto delta = iPosition - fLastMovePosition.value();
    if(delta.x != 0 || delta.y != 0)
    {
      std::for_each(fWidgets.begin(), fWidgets.end(), [&delta, this](auto const &p) {
        auto &widget = p.second;
        if(widget->getView().isSelected())
        {
          widget->getView().move(delta);
          checkWidgetForError(*widget);
        }
      });
    }
    fLastMovePosition = iPosition;
  }
}

//------------------------------------------------------------------------
// PanelView::endMoveWidgets
//------------------------------------------------------------------------
void PanelView::endMoveWidgets(ImVec2 const &iPosition)
{
  std::for_each(fWidgets.begin(), fWidgets.end(), [this](auto const &p) {
    auto &widget = p.second;
    auto view = widget->getView();
    if(view.isSelected())
    {
      auto position = view.getPosition();
      position.x = std::round(position.x);
      position.y = std::round(position.y);
      view.setPosition(position);
      checkWidgetForError(*widget);
    }
  });

  fLastMovePosition = std::nullopt;
}

//------------------------------------------------------------------------
// PanelView::getSelectedWidgets
//------------------------------------------------------------------------
std::vector<Widget *> PanelView::getSelectedWidgets() const
{
  std::vector<Widget *> c{};
  std::for_each(fWidgets.begin(), fWidgets.end(), [&c](auto const &p) {
    auto &widget = p.second;
    if(widget->getView().isSelected())
      c.emplace_back(widget.get());
  });
  return c;
}

//------------------------------------------------------------------------
// PanelView::checkWidgetForError
//------------------------------------------------------------------------
void PanelView::checkWidgetForError(Widget &iWidget)
{
  auto &view = iWidget.getView();
  auto max = fBackground->frameSize();
  auto p = view.getTopLeft();
  if(p.x < 0 || p.y < 0 || p.x > max.x || p.y > max.y)
  {
    view.setError(true);
    return;
  }
  p = view.getBottomRight();
  if(p.x < 0 || p.y < 0 || p.x > max.x || p.y > max.y)
  {
    view.setError(true);
    return;
  }
  view.setError(false);
}

//------------------------------------------------------------------------
// PanelView::editView
//------------------------------------------------------------------------
void PanelView::editView(EditContext &iCtx)
{
  auto selectedWidgets = getSelectedWidgets(); // TODO duplicate call... optimize!!!

  if(ImGui::Begin("Widgets"))
  {
    for(auto widget : selectedWidgets)
    {
      ImGui::Separator();
      ImGui::PushID(widget);
      widget->editView(iCtx);
      ImGui::PopID();
    }
  }
  ImGui::End();

}


}