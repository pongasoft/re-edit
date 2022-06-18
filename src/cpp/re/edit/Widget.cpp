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
#include <re/mock/Errors.h>
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
std::string UIText::getValueAsLua() const
{
  return re::mock::fmt::printf("jbox.ui_text(\"%s\")", fValue);
}

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
// DiscretePropertyValueList::getValueAsLua
//------------------------------------------------------------------------
std::string DiscretePropertyValueList::getValueAsLua() const
{
  std::vector<std::string> l{};
  std::transform(fValue.begin(), fValue.end(), std::back_inserter(l), [](auto i) { return std::to_string(i); } );
  return re::mock::fmt::printf("{ %s }", re::mock::stl::join_to_string(l));
}

//------------------------------------------------------------------------
// Values::hdgui2D
//------------------------------------------------------------------------
void Values::hdgui2D(Widget const &iWidget, attribute_list_t &oAttributes) const
{
  auto valueSwitch = iWidget.findValueSwitchValue();
  if(valueSwitch && !valueSwitch->empty())
    Attribute::hdgui2D(iWidget, oAttributes);
}

//------------------------------------------------------------------------
// VisibilityValues::hdgui2D
//------------------------------------------------------------------------
void VisibilityValues::hdgui2D(Widget const &iWidget, attribute_list_t &oAttributes) const
{
  auto visibilityValue = iWidget.findVisibilitySwitchValue();
  if(visibilityValue && !visibilityValue->empty())
    Attribute::hdgui2D(iWidget, oAttributes);
}

//------------------------------------------------------------------------
// String::editView
//------------------------------------------------------------------------
void String::editView(Widget &iWidget, EditContext const &iCtx)
{
  resetView(iWidget, iCtx);
  ImGui::SameLine();
  if(ImGui::InputText(fName.c_str(), &fValue))
    fProvided = true;
}

//------------------------------------------------------------------------
// Bool::editView
//------------------------------------------------------------------------
void Bool::editView(Widget &iWidget, EditContext const &iCtx)
{
  resetView(iWidget, iCtx);
  ImGui::SameLine();
  if(ImGui::Checkbox(fName.c_str(), &fValue))
    fProvided = true;
}

//------------------------------------------------------------------------
// PropertyPath::editView
//------------------------------------------------------------------------
void PropertyPath::editView(Widget &iWidget, EditContext const &iCtx)
{
  resetView(iWidget, iCtx);
  ImGui::SameLine();
  if(ImGui::BeginCombo(fName.c_str(), fValue.c_str()))
  {
    for(auto &p: iCtx.getPropertyNames(getPropertyKind()))
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
void PropertyPathList::editView(Widget &iWidget, EditContext const &iCtx)
{
  int deleteItemIdx = -1;
  for(int i = 0; i < fValue.size(); i++)
  {
    ImGui::PushID(i);
    if(ImGui::Button("-"))
      deleteItemIdx = i;
    ImGui::SameLine();
    auto &value = fValue[i];
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

  resetView(iWidget, iCtx);
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
// ValueSwitch::resetView
//------------------------------------------------------------------------
bool ValueSwitch::resetView(Widget &iWidget, EditContext const &iCtx)
{
  if(PropertyPath::resetView(iWidget, iCtx))
  {
    auto values = iWidget.findAttributeValue<Values>("values");
    RE_MOCK_INTERNAL_ASSERT(values != nullptr);
    if(values)
      values->clear();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------
// ValueSwitch::editView
//------------------------------------------------------------------------
void ValueSwitch::editView(Widget &iWidget, EditContext const &iCtx)
{
  PropertyPath::editView(iWidget, iCtx);
  auto value = iWidget.findValueValue();
  RE_MOCK_INTERNAL_ASSERT(value != nullptr);
  if(!fValue.empty() && !value->empty())
    fError = "Only one of [value] or [value_switch] must be provided";
  else
    fError = std::nullopt;
}

//------------------------------------------------------------------------
// Values::editView
//------------------------------------------------------------------------
void Values::editView(Widget &iWidget, EditContext const &iCtx)
{
  bool emptyError = false;
  auto valueSwitch = iWidget.findValueSwitchValue();
  if(valueSwitch)
  {
    auto stepCount = iCtx.getStepCount(*valueSwitch);
    if(stepCount > 1)
    {
      fValue.resize(stepCount);
      for(int i = 0; i < fValue.size(); i++)
      {
        auto &value = fValue[i];
        ImGui::PushID(i);
        if(ImGui::Button("X"))
          value = "";
        ImGui::SameLine();
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
        if(value.empty())
          emptyError = true;
      }
    }
  }
  if(emptyError)
    fError = "All values must be provided";
  else
    fError = std::nullopt;
}

//------------------------------------------------------------------------
// VisibilitySwitch::editView
//------------------------------------------------------------------------
void VisibilitySwitch::editView(Widget &iWidget, EditContext const &iCtx)
{
  PropertyPath::editView(iWidget, iCtx);
  if(!fValue.empty())
  {
    auto ptr = iWidget.findAttribute<VisibilityValues>("visibility_values");
    RE_MOCK_INTERNAL_ASSERT(ptr != nullptr);
    if(ptr->fValue.empty())
    {
      ptr->fValue.emplace_back(0);
      ptr->fProvided = true;
    }
  }
}

//------------------------------------------------------------------------
// VisibilityValues::editView
//------------------------------------------------------------------------
void VisibilityValues::editView(Widget &iWidget, EditContext const &iCtx)
{
  auto visibilityValue = iWidget.findVisibilitySwitchValue();
  if(visibilityValue)
  {
    auto stepCount = iCtx.getStepCount(*visibilityValue);
    if(stepCount > 1)
    {
      int deleteItemIdx = -1;
      for(int i = 0; i < fValue.size(); i++)
      {
        ImGui::PushID(i);
        if(ImGui::Button("-"))
          deleteItemIdx = i;
        ImGui::SameLine();
        ImGui::SliderInt(fName.c_str(), &fValue[i], 0, stepCount - 1);
        ImGui::PopID();
      }

      if(deleteItemIdx >= 0)
        fValue.erase(fValue.begin() + deleteItemIdx);

      ImGui::PushID(static_cast<int>(fValue.size()));

      if(ImGui::Button("+"))
      {
        fProvided = true;
        fValue.emplace_back(0);
      }

      ImGui::SameLine();
      ImGui::LabelText(fName.c_str(), "Click + to add");

      ImGui::PopID();
    }
  }
}

//------------------------------------------------------------------------
// StaticStringList::editView
//------------------------------------------------------------------------
void StaticStringList::editView(Widget &iWidget, EditContext const &iCtx)
{
  resetView(iWidget, iCtx);
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
    w->editView(*this, iCtx);
    if(w->fError)
    {
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1,0,0,1), "(?)");
      if (ImGui::IsItemHovered())
      {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(w->fError->c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
      }
    }
    w->hdgui2D(*this, atts);
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
  return addAttribute(Attribute::build<ValueSwitch>("value_switch", ""));
}

//------------------------------------------------------------------------
// Widget::values
//------------------------------------------------------------------------
Widget *Widget::values()
{
  return addAttribute(Attribute::build<Values>("values", {}));
}

//------------------------------------------------------------------------
// Widget::visibility_switch
//------------------------------------------------------------------------
Widget *Widget::visibility_switch()
{
  return addAttribute(Attribute::build<VisibilitySwitch>("visibility_switch", ""));
}

//------------------------------------------------------------------------
// Widget::visibility_values
//------------------------------------------------------------------------
Widget *Widget::visibility_values()
{
  return addAttribute(Attribute::build<VisibilityValues>("visibility_values", {}));
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
  static const std::vector<std::string> tooltipPositions =
    { "bottom_left", "bottom", "bottom_right", "right", "top_right", "top", "top_left", "left", "no_tooltip"};
  return addAttribute(Attribute::build<StaticStringList>("tooltip_position", "", tooltipPositions));
}

//------------------------------------------------------------------------
// Widget::tooltip_template
//------------------------------------------------------------------------
Widget *Widget::tooltip_template()
{
  return addAttribute(Attribute::build<UIText>("tooltip_template", ""));
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
    ->visibility_switch()
    ->visibility_values()
    ->tooltip_position()
    ->tooltip_template()
    ->show_remote_box()
    ->show_automation_rect()
    ;

  return w;
}

}


namespace re::edit::widget {

//------------------------------------------------------------------------
// Attribute::hdgui2D
//------------------------------------------------------------------------
void Attribute::hdgui2D(Widget const &iWidget, attribute_list_t &oAttributes) const
{
  if(fProvided)
    oAttributes.emplace_back(attribute_t{fName, getValueAsLua()});
}

}

