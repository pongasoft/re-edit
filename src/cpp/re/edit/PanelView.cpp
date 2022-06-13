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
        endMoveControls(mousePos / iCtx.getZoom());
      }
      else
      {
        fMouseDrag->fCurrentPosition = mousePos;
        if(fMouseDrag->fInitialPosition.x != fMouseDrag->fCurrentPosition.x ||
           fMouseDrag->fInitialPosition.y != fMouseDrag->fCurrentPosition.y)
        {
          dragState = "onDrag";
          moveControls(mousePos / iCtx.getZoom());
        }
        else
          dragState = "waiting for drag";
      }
    } else if(ImGui::IsItemClicked(ImGuiMouseButton_::ImGuiMouseButton_Left))
    {
      fMouseDrag = MouseDrag{mousePos, mousePos};
      auto &io = ImGui::GetIO();
      dragState = "onPressed / " + std::to_string(io.KeyShift);
      selectControl(mousePos / iCtx.getZoom(), io.KeyShift);
    }
  }
  ImGui::SetCursorScreenPos(cp); // InvisibleButton moves the cursor so we restore it
  for(auto &control: fControls)
  {
    auto &w = control.second;
    w->draw(iCtx);
  }

  auto selectedControls = getSelectedControls();

  if(fMouseDrag && !selectedControls.empty())
  {
    auto min = selectedControls[0]->getTopLeft();
    auto max = selectedControls[0]->getBottomRight();

    std::for_each(selectedControls.begin() + 1, selectedControls.end(), [&min, &max](auto c) {
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

  if(ImGui::Begin("Controls"))
  {
    ImGui::SliderFloat("zoom", &iCtx.getZoom(), 0.25f, 1.5f);
    ImGui::Checkbox("Show Control Border", &iCtx.getUserPreferences().fShowControlBorder);
    for(auto control : selectedControls)
    {
      ImGui::Separator();
      ImGui::PushID(control);
      control->renderEdit();
      ImGui::PopID();
    }
  }
  ImGui::End();
}

//------------------------------------------------------------------------
// PanelView::addControl
//------------------------------------------------------------------------
int PanelView::addControl(std::unique_ptr<ControlView> iControl)
{
  return fControls.add(std::move(iControl));
}

//------------------------------------------------------------------------
// PanelView::selectControl
//------------------------------------------------------------------------
void PanelView::selectControl(ImVec2 const &iPosition, bool iMultiple)
{
  auto ci = std::find_if(fControls.begin(), fControls.end(), [&iPosition](auto const &p) { return p.second->contains(iPosition); });
  if(ci == fControls.end())
  {
    for(auto &p: fControls)
      p.second->setSelected(false);
  }
  else
  {
    auto &control = ci->second;
    if(iMultiple)
    {
      if(!control->isSelected())
      {
        fLastMovePosition = iPosition;
      }
      control->toggleSelection();
    }
    else
    {
      fLastMovePosition = iPosition;
      if(!control->isSelected())
      {
        for(auto &p: fControls)
          p.second->setSelected(false);
        control->setSelected(true);
      }
    }
  }
}

//------------------------------------------------------------------------
// PanelView::moveControls
//------------------------------------------------------------------------
void PanelView::moveControls(ImVec2 const &iPosition)
{
  if(fLastMovePosition)
  {
    auto delta = iPosition - fLastMovePosition.value();
    if(delta.x != 0 || delta.y != 0)
    {
      std::for_each(fControls.begin(), fControls.end(), [&delta, this](auto const &p) {
        auto &control = p.second;
        if(control->isSelected())
        {
          control->move(delta);
          checkControlForError(*control);
        }
      });
    }
    fLastMovePosition = iPosition;
  }
}

//------------------------------------------------------------------------
// PanelView::endMoveControls
//------------------------------------------------------------------------
void PanelView::endMoveControls(ImVec2 const &iPosition)
{
  std::for_each(fControls.begin(), fControls.end(), [this](auto const &p) {
    auto &control = p.second;
    if(control->isSelected())
    {
      auto position = control->getPosition();
      position.x = std::round(position.x);
      position.y = std::round(position.y);
      control->setPosition(position);
      checkControlForError(*control);
    }
  });

  fLastMovePosition = std::nullopt;
}

//------------------------------------------------------------------------
// PanelView::getSelectedControls
//------------------------------------------------------------------------
std::vector<ControlView *> PanelView::getSelectedControls() const
{
  std::vector<ControlView *> c{};
  std::for_each(fControls.begin(), fControls.end(), [&c](auto const &p) {
    auto &control = p.second;
    if(control->isSelected())
      c.emplace_back(control.get());
  });
  return c;
}

//------------------------------------------------------------------------
// PanelView::checkControlForError
//------------------------------------------------------------------------
void PanelView::checkControlForError(ControlView &iControl)
{
  auto max = fBackground->frameSize();
  auto p = iControl.getTopLeft();
  if(p.x < 0 || p.y < 0 || p.x > max.x || p.y > max.y)
  {
    iControl.setError(true);
    return;
  }
  p = iControl.getBottomRight();
  if(p.x < 0 || p.y < 0 || p.x > max.x || p.y > max.y)
  {
    iControl.setError(true);
    return;
  }
  iControl.setError(false);
}


}