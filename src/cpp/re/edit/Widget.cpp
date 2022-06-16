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

#include "Widget.h"
#include <re/mock/fmt.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace re::edit::widget::attribute {

//------------------------------------------------------------------------
// escapeString
//------------------------------------------------------------------------
std::string escapeString(std::string const &s)
{
  return re::mock::fmt::printf("\"%s\"", s);
}

//------------------------------------------------------------------------
// String::getValueAsLua
//------------------------------------------------------------------------
std::string String::getValueAsLua() const { return escapeString(fValue); }

//------------------------------------------------------------------------
// PropertyPathList::getValueAsLua
//------------------------------------------------------------------------
std::string PropertyPathList::getValueAsLua() const
{
  std::vector<std::string> l{};
  std::transform(fValue.begin(), fValue.end(), std::back_inserter(l), escapeString);
  return re::mock::fmt::printf("{ %s }", re::mock::stl::join_to_string(l));
}

//------------------------------------------------------------------------
// String::editView
//------------------------------------------------------------------------
void String::editView(EditContext &iCtx)
{
  if(ImGui::InputText(fName.c_str(), &fValue))
    fProvided = true;
}

//------------------------------------------------------------------------
// Bool::editView
//------------------------------------------------------------------------
void Bool::editView(EditContext &iCtx)
{
  if(ImGui::Checkbox(fName.c_str(), &fValue))
    fProvided = true;
}

//------------------------------------------------------------------------
// PropertyPath::editView
//------------------------------------------------------------------------
void PropertyPath::editView(EditContext &iCtx)
{
  resetView(iCtx);
  ImGui::SameLine();
  if(ImGui::BeginCombo(fName.c_str(), fValue.c_str()))
  {
    for(auto &p: iCtx.getPropertyNames())
    {
      auto const isSelected = p == fValue;
      if(ImGui::Selectable(p.c_str(), isSelected))
      {
        fProvided = true;
        fValue = p;
      }
      if(isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
}

//------------------------------------------------------------------------
// PropertyPathList::editView
//------------------------------------------------------------------------
void PropertyPathList::editView(EditContext &iCtx)
{
  int deleteItemIdx = -1;
  for(int i = 0; i < fValue.size(); i++)
  {
    if(ImGui::Button("-"))
      deleteItemIdx = i;
    ImGui::SameLine();
    auto &value = fValue[i];
    ImGui::PushID(i);
    if(ImGui::BeginCombo(fName.c_str(), value.c_str()))
    {
      for(auto &p: iCtx.getPropertyNames())
      {
        auto const isSelected = p == value;
        if(ImGui::Selectable(p.c_str(), isSelected))
        {
          fProvided = true;
          value = p;
        }
        if(isSelected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    ImGui::PopID();
  }

  if(deleteItemIdx >= 0)
    fValue.erase(fValue.begin() + deleteItemIdx);

  resetView(iCtx);
  ImGui::SameLine();

  ImGui::PushID(static_cast<int>(fValue.size()));

  std::string newValue{};
  if(ImGui::BeginCombo(fName.c_str(), newValue.c_str()))
  {
    for(auto &p: iCtx.getPropertyNames())
    {
      auto const isSelected = p == newValue;
      if(ImGui::Selectable(p.c_str(), isSelected))
      {
        fProvided = true;
        fValue.emplace_back(p);
      }
      if(isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  ImGui::PopID();
}

//------------------------------------------------------------------------
// StaticStringList::editView
//------------------------------------------------------------------------
void StaticStringList::editView(EditContext &iCtx)
{
  resetView(iCtx);
  ImGui::SameLine();
  if(ImGui::BeginCombo(fName.c_str(), fValue.c_str()))
  {
    for(auto &p: fSelectionList)
    {
      auto const isSelected = p == fValue;
      if(ImGui::Selectable(p.c_str(), isSelected))
      {
        fProvided = true;
        fValue = p;
      }
      if(isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
}

}

namespace re::edit {

using namespace widget;
using namespace widget::attribute;

//------------------------------------------------------------------------
// Widget::editView
//------------------------------------------------------------------------
void Widget::editView(EditContext &iCtx)
{
  attribute_list_t atts{};

  for(auto &w: fAttributes)
  {
    ImGui::PushID(w->fName.c_str());
    w->editView(iCtx);
    w->device2D(atts);
    ImGui::PopID();
  }

  std::vector<std::string> l{};
  std::transform(atts.begin(), atts.end(), std::back_inserter(l), [](auto &att) {
    return re::mock::fmt::printf("%s = %s", att.fName, att.fValue);
  });
  re::mock::stl::join_to_string(l, "\n");
  ImGui::Text("%s", re::mock::stl::join_to_string(l, "\n").c_str());
}

//------------------------------------------------------------------------
// Widget::value
//------------------------------------------------------------------------
Widget *Widget::value()
{
  return addAttribute(Attribute::build<PropertyPath>("value", ""));
}

//------------------------------------------------------------------------
// Widget::value_switch
//------------------------------------------------------------------------
Widget *Widget::value_switch()
{
  return addAttribute(Attribute::build<PropertyPath>("value_switch", ""));
}

//------------------------------------------------------------------------
// Widget::values
//------------------------------------------------------------------------
Widget *Widget::values()
{
  return addAttribute(Attribute::build<PropertyPathList>("values", {}));
}

//------------------------------------------------------------------------
// Widget::show_remote_box
//------------------------------------------------------------------------
Widget *Widget::show_remote_box()
{
  return addAttribute(Attribute::build<Bool>("show_remote_box", true));
}

//------------------------------------------------------------------------
// Widget::show_automation_rect
//------------------------------------------------------------------------
Widget *Widget::show_automation_rect()
{
  return addAttribute(Attribute::build<Bool>("show_automation_rect", true));
}

//------------------------------------------------------------------------
// Widget::tooltip_position
//------------------------------------------------------------------------
Widget *Widget::tooltip_position()
{
  static std::vector<std::string> tooltipPositions = { "bottom_left", "bottom", "bottom_right", "right", "top_right", "top", "top_left", "left", "no_tooltip"};
  auto attribute = Attribute::build<StaticStringList>("tooltip_position", "");
  attribute->fSelectionList = tooltipPositions;
  return addAttribute(std::move(attribute));
}

//------------------------------------------------------------------------
// Widget::analog_knob
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::analog_knob(Panel iPanel)
{
  auto w = std::make_unique<Widget>(iPanel);
  w ->value()
    ->value_switch()
    ->values()
    ->show_remote_box()
    ->show_automation_rect()
    ->tooltip_position();

  return w;
}


}


namespace re::edit::widget {

//------------------------------------------------------------------------
// Attribute::device2D
//------------------------------------------------------------------------
void Attribute::device2D(attribute_list_t &oAttributes) const
{
  if(fProvided)
    oAttributes.emplace_back(attribute_t{fName, getValueAsLua()});
}

}

