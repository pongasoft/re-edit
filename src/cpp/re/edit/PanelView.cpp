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
    iCtx.TextureItem(fBackground.get());
    backgroundScreenPosition = ImGui::GetItemRectMin(); // accounts for scrollbar!
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
  ImGui::SetCursorScreenPos(cp); // TextureItem moves the cursor so we restore it
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

  if(ImGui::Begin("Control"))
  {
    ImGui::SliderFloat("zoom", &iCtx.getZoom(), 0.25f, 1.5f);
    if(selectedControls.size() == 1)
    {
      auto selectedControl = selectedControls[0];
      auto pos = selectedControl->getPosition();
      auto x = static_cast<int>(std::round(pos.x));
      ImGui::InputInt("x", &x, 1, 5);
      auto y = static_cast<int>(std::round(pos.y));
      ImGui::InputInt("y", &y, 1, 5);
      if(x != static_cast<int>(std::round(pos.x)) || y != static_cast<int>(std::round(pos.y)))
        selectedControl->setPosition({static_cast<float>(x), static_cast<float>(y)});

      auto texture = selectedControl->getTexture();
      if(texture->numFrames() > 2)
      {
        ImGui::SliderInt("Frame", &selectedControl->getFrameNumber(), 0, texture->numFrames() - 1);
      }
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
// PanelView::getSelectedControl
//------------------------------------------------------------------------
ControlView *PanelView::getSelectedControl() const
{
  if(std::count_if(fControls.begin(), fControls.end(), [](auto const &p) { return p.second->isSelected(); }) == 1)
    return std::find_if(fControls.begin(), fControls.end(), [](auto const &p) { return p.second->isSelected(); })->second.get();
  else
    return nullptr;
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
      std::for_each(fControls.begin(), fControls.end(), [&delta](auto const &p) {
        if(p.second->isSelected())
          p.second->move(delta);
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
  std::for_each(fControls.begin(), fControls.end(), [](auto const &p) {
    auto &control = p.second;
    if(control->isSelected())
    {
      auto position = control->getPosition();
      position.x = std::round(position.x);
      position.y = std::round(position.y);
      control->setPosition(position);
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


}