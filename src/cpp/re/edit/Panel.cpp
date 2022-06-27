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

#include "Panel.h"
#include "ReGui.h"

namespace re::edit {

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
      if(ImGui::IsMouseReleased(ImGuiMouseButton_::ImGuiMouseButton_Left))
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
    } else if(ImGui::IsItemClicked(ImGuiMouseButton_::ImGuiMouseButton_Left))
    {
      fMouseDrag = MouseDrag{mousePos, mousePos};
      auto &io = ImGui::GetIO();
      dragState = "onPressed / " + std::to_string(io.KeyShift);
      selectWidget(mousePos / iCtx.fZoom, io.KeyShift);
    }
  }
  ImGui::SetCursorScreenPos(cp); // InvisibleButton moves the cursor so we restore it
  for(auto &widget: fWidgets)
  {
    auto &w = widget.second;
    w->draw(iCtx);
  }

  auto selectedWidgets = getSelectedWidgets();

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
}

//------------------------------------------------------------------------
// Panel::addWidget
//------------------------------------------------------------------------
int Panel::addWidget(std::unique_ptr<Widget> iWidget)
{
  return fWidgets.add(std::move(iWidget));
}

//------------------------------------------------------------------------
// Panel::selectWidget
//------------------------------------------------------------------------
void Panel::selectWidget(ImVec2 const &iPosition, bool iMultiple)
{
  auto ci = std::find_if(fWidgets.begin(), fWidgets.end(), [&iPosition](auto const &p) { return p.second->contains(iPosition); });
  if(ci == fWidgets.end())
  {
    for(auto &p: fWidgets)
      p.second->setSelected(false);
  }
  else
  {
    auto &widget = ci->second;
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
// Panel::moveWidgets
//------------------------------------------------------------------------
void Panel::moveWidgets(ImVec2 const &iPosition)
{
  if(fLastMovePosition)
  {
    auto delta = iPosition - fLastMovePosition.value();
    if(delta.x != 0 || delta.y != 0)
    {
      std::for_each(fWidgets.begin(), fWidgets.end(), [&delta, this](auto const &p) {
        auto &widget = p.second;
        if(widget->isSelected())
        {
          widget->move(delta);
          checkWidgetForError(*widget);
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
  std::for_each(fWidgets.begin(), fWidgets.end(), [this](auto const &p) {
    auto &widget = p.second;
    if(widget->isSelected())
    {
      auto position = widget->getPosition();
      position.x = std::round(position.x);
      position.y = std::round(position.y);
      widget->setPosition(position);
      checkWidgetForError(*widget);
    }
  });

  fLastMovePosition = std::nullopt;
}

//------------------------------------------------------------------------
// Panel::getSelectedWidgets
//------------------------------------------------------------------------
std::vector<Widget *> Panel::getSelectedWidgets() const
{
  std::vector<Widget *> c{};
  std::for_each(fWidgets.begin(), fWidgets.end(), [&c](auto const &p) {
    auto &widget = p.second;
    if(widget->isSelected())
      c.emplace_back(widget.get());
  });
  return c;
}

//------------------------------------------------------------------------
// Panel::checkWidgetForError
//------------------------------------------------------------------------
void Panel::checkWidgetForError(Widget &iWidget)
{
  auto max = fGraphics.getSize();
  auto p = iWidget.getTopLeft();
  if(p.x < 0 || p.y < 0 || p.x > max.x || p.y > max.y)
  {
    iWidget.setError(true);
    return;
  }
  p = iWidget.getBottomRight();
  if(p.x < 0 || p.y < 0 || p.x > max.x || p.y > max.y)
  {
    iWidget.setError(true);
    return;
  }
  iWidget.setError(false);
}

//------------------------------------------------------------------------
// Panel::editView
//------------------------------------------------------------------------
void Panel::editView(EditContext &iCtx)
{
  auto selectedWidgets = getSelectedWidgets(); // TODO duplicate call... optimize!!!

  if(ImGui::Begin(getEditViewWindowName().c_str()))
  {
    auto size = selectedWidgets.size();
    switch(size)
    {
      case 0:
        fGraphics.editView(iCtx);
        break;

      case 1:
        selectedWidgets[0]->editView(iCtx);
        break;

      default:
      {
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
  }
  ImGui::End();

}

//------------------------------------------------------------------------
// Panel::getEditViewWindowName
//------------------------------------------------------------------------
std::string Panel::getEditViewWindowName() const
{
  return re::mock::fmt::printf("%s Widgets", getName());
}

//------------------------------------------------------------------------
// Panel::getName
//------------------------------------------------------------------------
char const *Panel::getName() const
{
  switch(fType)
  {
    case Type::kFront: return "Front";
    case Type::kFoldedFront: return "Folded Front";
    case Type::kBack: return "Back";
    case Type::kFoldedBack: return "Folded Back";
    default:
      RE_MOCK_FAIL("Not reached");
  }
}


}