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
// String::edit
//------------------------------------------------------------------------
void String::edit(EditContext &iCtx)
{
  if(ImGui::InputText(fName.c_str(), &fValue))
    fProvided = true;
}

//------------------------------------------------------------------------
// Bool::edit
//------------------------------------------------------------------------
void Bool::edit(EditContext &iCtx)
{
  if(ImGui::Checkbox(fName.c_str(), &fValue))
    fProvided = true;
}

//------------------------------------------------------------------------
// PropertyPath::edit
//------------------------------------------------------------------------
void PropertyPath::edit(EditContext &iCtx)
{
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

}

namespace re::edit {

using namespace widget;
using namespace widget::attribute;

//------------------------------------------------------------------------
// Widget::edit
//------------------------------------------------------------------------
void Widget::edit(EditContext &iCtx)
{
  attribute_list_t atts{};

  for(auto &w: fAttributes)
  {
    ImGui::PushID(w->fName.c_str());
    w->edit(iCtx);
    w->device2D(atts);
    ImGui::PopID();
  }

  std::vector<std::string> l{};
  std::transform(atts.begin(), atts.end(), std::back_inserter(l), [](auto &att) {
    return re::mock::fmt::printf("%s = %s", att.fName, att.fValue);
  });
  re::mock::stl::join_to_string(l, "\n");
  ImGui::Text("%s", re::mock::stl::join_to_string(l, "\n").c_str());
  ImGui::Text("%s", "hello?");
}

//------------------------------------------------------------------------
// Widget::analog_knob
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::analog_knob(Panel iPanel)
{
  auto w = std::make_unique<Widget>(iPanel);
  w->addAttribute(Attribute::build<PropertyPath>("value"));
  w->addAttribute(Attribute::build<PropertyPath>("value_switch"));
  w->addAttribute(Attribute::build<Bool>("show_remote_box", true));
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

