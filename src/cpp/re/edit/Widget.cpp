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
// DiscretePropertyValueList::editView
//------------------------------------------------------------------------
void DiscretePropertyValueList::editView(int iMin,
                                         int iMax,
                                         std::function<void()>                       const &iOnAdd,
                                         std::function<void(int iIndex, int iValue)> const &iOnUpdate,
                                         std::function<void(int iIndex)>             const &iOnDelete) const
{
  int deleteItemIdx = -1;
  for(int i = 0; i < fValue.size(); i++)
  {
    ImGui::PushID(i);
    if(ImGui::Button("-"))
      deleteItemIdx = i;
    ImGui::SameLine();
    int value = fValue[i];
    if(ImGui::SliderInt(re::mock::fmt::printf("%s [%d]", fName, i).c_str(), &value, iMin, iMax))
      iOnUpdate(i, value);
    ImGui::PopID();
  }

  if(deleteItemIdx >= 0)
    iOnDelete(deleteItemIdx);

  ImGui::PushID(static_cast<int>(fValue.size()));

  if(ImGui::Button("+"))
    iOnAdd();

  ImGui::SameLine();
  ImGui::LabelText(fName.c_str(), "Click + to add");

  ImGui::PopID();

}

//------------------------------------------------------------------------
// Value::hdgui2D
//------------------------------------------------------------------------
void Value::hdgui2D(attribute_list_t &oAttributes) const
{
  if(fUseSwitch)
  {
    fValueSwitch.hdgui2D(oAttributes);
    fValues.hdgui2D(oAttributes);
  }
  else
  {
    fValue.hdgui2D(oAttributes);
  }
}

//------------------------------------------------------------------------
// Value::editView
//------------------------------------------------------------------------
void Value::editView(EditContext const &iCtx)
{
  if(ImGui::Button("X"))
    reset();

  ImGui::SameLine();

  ImGui::BeginGroup();

  if(fUseSwitch)
  {
    fValueSwitch.editView(iCtx.getPropertyNames(EditContext::PropertyKind::kDiscrete),
                          {}, // disable onReset
                          [this, &iCtx](std::string const &p) {
                            fValueSwitch.fValue = p;
                            fValueSwitch.fProvided = true;
                            fValues.fValue.clear();
                            fValues.fValue.resize(iCtx.getStepCount(p));
                          });
    ImGui::SameLine();
    ImGui::Checkbox("S", &fUseSwitch);
    ImGui::Indent();
    fValues.editStaticListView(iCtx.getPropertyNames(),
                               {},  // disable onReset
                               [this](int iIndex, std::string const &p) { // onSelect
                                 fValues.fValue[iIndex] = p;
                                 fValues.fProvided = true;
                               });
    ImGui::Unindent();
  }
  else
  {
    fValue.editView(iCtx.getPropertyNames(),
                    {},
                    [this](std::string const &p) {
                      fValue.fValue = p;
                      fValue.fProvided = true;
                    });
    ImGui::SameLine();
    ImGui::Checkbox("S", &fUseSwitch);
  }

  ImGui::EndGroup();
}

//------------------------------------------------------------------------
// Value::editView
//------------------------------------------------------------------------
void Value::reset()
{
  fValue.reset();
  fValueSwitch.reset();
  fValues.reset();
  fUseSwitch = false;
}

//------------------------------------------------------------------------
// Visibility::hdgui2D
//------------------------------------------------------------------------
void Visibility::hdgui2D(attribute_list_t &oAttributes) const
{
  if(!fSwitch.fValue.empty())
  {
    fSwitch.hdgui2D(oAttributes);
    fValues.hdgui2D(oAttributes);
  }
}

//------------------------------------------------------------------------
// Visibility::editView
//------------------------------------------------------------------------
void Visibility::editView(EditContext const &iCtx)
{
  static const std::string EMPTY_LIST_ERROR = "You must provide at least 1 value";
  fSwitch.editView(iCtx.getPropertyNames(EditContext::PropertyKind::kDiscrete),
                   [this] () { // onReset
                     reset();
                   },
                   [this] (std::string const &p) { // onSelect
                     fSwitch.fValue = p;
                     fSwitch.fProvided = true;
                     fValues.fValue = {0};
                     fValues.fProvided = true;
                   });
  auto stepCount = iCtx.getStepCount(fSwitch.fValue);
  if(stepCount > 1)
  {
    ImGui::Indent();
    fValues.editView(0, stepCount - 1,
                     [this]() { // onAdd
                       fValues.fValue.emplace_back(0);
                       clearError();
                       fValues.fProvided = true;
                     },
                     [this](int iIndex, int iValue) { // onUpdate
                       fValues.fValue[iIndex] = iValue;
                     },
                     [this](int iIndex) { // onDelete
                       fValues.fValue.erase(fValues.fValue.begin() + iIndex);
                       if(fValues.fValue.empty())
                         fError = EMPTY_LIST_ERROR;
                       fValues.fProvided = false;
                     }
    );
    ImGui::Unindent();
  }
}

//------------------------------------------------------------------------
// Visibility::reset
//------------------------------------------------------------------------
void Visibility::reset()
{
  fSwitch.reset();
  fValues.reset();
}

//------------------------------------------------------------------------
// String::editView
//------------------------------------------------------------------------
void String::editView(EditContext const &iCtx)
{
  resetView();
  ImGui::SameLine();
  if(ImGui::InputText(fName.c_str(), &fValue))
    fProvided = true;
}

//------------------------------------------------------------------------
// Bool::editView
//------------------------------------------------------------------------
void Bool::editView(EditContext const &iCtx)
{
  resetView();
  ImGui::SameLine();
  if(ImGui::Checkbox(fName.c_str(), &fValue))
    fProvided = true;
}

//------------------------------------------------------------------------
// PropertyPath::editView
//------------------------------------------------------------------------
void PropertyPath::editView(EditContext const &iCtx)
{
  editView(iCtx.getPropertyNames(getPropertyKind()),
           [this]() { reset(); },
           [this](std::string const &p) {
             fValue = p;
             fProvided = true;
           });
}

//------------------------------------------------------------------------
// PropertyPath::editView
//------------------------------------------------------------------------
void PropertyPath::editView(std::vector<std::string> const &iProperties,
                            std::function<void()> const &iOnReset,
                            std::function<void(std::string const &)> const &iOnSelect) const
{
  if(iOnReset)
  {
    resetView(iOnReset);
    ImGui::SameLine();
  }
  if(ImGui::BeginCombo(fName.c_str(), fValue.c_str()))
  {
    for(auto &p: iProperties)
    {
      auto const isSelected = p == fValue;
      if(ImGui::Selectable(p.c_str(), isSelected))
        iOnSelect(p);
      if(isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
}

////------------------------------------------------------------------------
//// PropertyPathList::editView
////------------------------------------------------------------------------
//void PropertyPathList::editView(EditContext const &iCtx)
//{
//  int deleteItemIdx = -1;
//  for(int i = 0; i < fValue.size(); i++)
//  {
//    ImGui::PushID(i);
//    if(ImGui::Button("-"))
//      deleteItemIdx = i;
//    ImGui::SameLine();
//    auto &value = fValue[i];
//    if(ImGui::BeginCombo(fName.c_str(), value.c_str()))
//    {
//      for(auto &p: iCtx.getPropertyNames())
//      {
//        auto const isSelected = p == value;
//        if(ImGui::Selectable(p.c_str(), isSelected))
//        {
//          value = p;
//        }
//        if(isSelected)
//          ImGui::SetItemDefaultFocus();
//      }
//      ImGui::EndCombo();
//    }
//    ImGui::PopID();
//  }
//
//  if(deleteItemIdx >= 0)
//    fValue.erase(fValue.begin() + deleteItemIdx);
//
//  resetView();
//  ImGui::SameLine();
//
//  ImGui::PushID(static_cast<int>(fValue.size()));
//
//  std::string newValue{};
//  if(ImGui::BeginCombo(fName.c_str(), newValue.c_str()))
//  {
//    for(auto &p: iCtx.getPropertyNames())
//    {
//      auto const isSelected = p == newValue;
//      if(ImGui::Selectable(p.c_str(), isSelected))
//      {
//        fValue.emplace_back(p);
//      }
//      if(isSelected)
//        ImGui::SetItemDefaultFocus();
//    }
//    ImGui::EndCombo();
//  }
//
//  ImGui::PopID();
//}

//------------------------------------------------------------------------
// PropertyPathList::editStaticListView
//------------------------------------------------------------------------
void PropertyPathList::editStaticListView(std::vector<std::string> const &iProperties,
                                          std::function<void()> const &iOnReset,
                                          std::function<void(int iIndex, std::string const &)> const &iOnSelect) const
{
  if(iOnReset)
  {
    resetView(iOnReset);
    ImGui::SameLine();
  }

  for(int i = 0; i < fValue.size(); i++)
  {
    ImGui::PushID(i);

    auto &value = fValue[i];
    if(ImGui::BeginCombo(re::mock::fmt::printf("%s [%d]", fName, i).c_str(), value.c_str()))
    {
      for(auto &p: iProperties)
      {
        auto const isSelected = p == value;

        if(ImGui::Selectable(p.c_str(), isSelected))
          iOnSelect(i, p);

        if(isSelected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    ImGui::PopID();
  }
}

//------------------------------------------------------------------------
// StaticStringList::editView
//------------------------------------------------------------------------
void StaticStringList::editView(EditContext const &iCtx)
{
  resetView();
  ImGui::SameLine();
  if(ImGui::BeginCombo(fName.c_str(), fValue.c_str()))
  {
    for(auto &p: fSelectionList)
    {
      auto const isSelected = p == fValue;
      if(ImGui::Selectable(p.c_str(), isSelected))
      {
        fValue = p;
        fProvided = true;
      }
      if(isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
}


//------------------------------------------------------------------------
// Graphics::draw
//------------------------------------------------------------------------
void Graphics::draw(DrawContext &iCtx, int iFrameNumber, ImVec4 const &iBorderCol) const
{
  if(fTexture)
    iCtx.drawTexture(fTexture.get(), fPosition, iFrameNumber, iBorderCol);
  else
    iCtx.drawRectFilled(fPosition, getSize(), ImVec4{});
}

//------------------------------------------------------------------------
// Graphics::editView
//------------------------------------------------------------------------
void Graphics::editView(std::vector<std::string> const &iTextureKeys,
                        std::function<void(std::string const &)> const &iOnTextureUpdate) const
{
  auto key = fTexture ? fTexture->key() : "";
  if(ImGui::BeginCombo(fName.c_str(), key.c_str()))
  {
    for(auto &p: iTextureKeys)
    {
      auto const isSelected = p == key;
      if(ImGui::Selectable(p.c_str(), isSelected))
      iOnTextureUpdate(p);
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
// Widget::draw
//------------------------------------------------------------------------
void Widget::draw(DrawContext &iCtx)
{
  if(fHidden)
    return;

  ImVec4 borderColor{};
  if(fSelected)
    borderColor = iCtx.getUserPreferences().fSelectedWidgetColor;
  else
  {
    if(iCtx.fShowWidgetBorder)
      borderColor = iCtx.getUserPreferences().fWidgetBorderColor;
  }

  fGraphics.draw(iCtx, fFrameNumber, borderColor);

  if(fError)
    iCtx.drawRectFilled(fGraphics.fPosition, fGraphics.getSize(), iCtx.getUserPreferences().fWidgetErrorColor);
}

//------------------------------------------------------------------------
// Widget::editView
//------------------------------------------------------------------------
void Widget::editView(EditContext &iCtx)
{
  fGraphics.editView(iCtx.getTextureKeys(),
                     [this, &iCtx](std::string const &iTextureKey) {
    fGraphics.fTexture = iCtx.getTexture(iTextureKey);
    fFrameNumber = 0;
  });

  auto x = static_cast<int>(std::round(fGraphics.fPosition.x));
  ImGui::InputInt("x", &x, 1, 5);
  auto y = static_cast<int>(std::round(fGraphics.fPosition.y));
  ImGui::InputInt("y", &y, 1, 5);
  if(x != static_cast<int>(std::round(fGraphics.fPosition.x)) || y != static_cast<int>(std::round(fGraphics.fPosition.y)))
    fGraphics.fPosition = {static_cast<float>(x), static_cast<float>(y)};

  if(fGraphics.fTexture)
  {
    auto numFrames = fGraphics.fTexture->numFrames();
    if(numFrames > 2)
      ImGui::SliderInt("Frame", &fFrameNumber, 0, numFrames - 1);

    if(numFrames == 2)
    {
      bool v = fFrameNumber == 1;
      if(ImGui::Checkbox("Frame", &v))
        fFrameNumber = v ? 1 : 0;
    }
  }

  if(ImGui::TreeNode("Attributes"))
  {
    for(auto &w: fAttributes)
    {
      ImGui::PushID(w->fName.c_str());
      w->editView(iCtx);
      if(w->fError)
      {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1,0,0,1), "(?)");
        if(ImGui::IsItemHovered())
        {
          ImGui::BeginTooltip();
          ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
          ImGui::TextUnformatted(w->fError->c_str());
          ImGui::PopTextWrapPos();
          ImGui::EndTooltip();
        }
      }
      ImGui::PopID();
    }
    ImGui::TreePop();
  }

  if(ImGui::TreeNode("hdgui2D"))
  {
    auto size = ImGui::GetWindowSize();
    ImGui::PushTextWrapPos(size.x);
    ImGui::TextUnformatted(hdgui2D().c_str());
    ImGui::PopTextWrapPos();
    ImGui::TreePop();
  }

}

//------------------------------------------------------------------------
// Widget::hdgui2D
//------------------------------------------------------------------------
std::string Widget::hdgui2D() const
{
  attribute_list_t atts{};

  fGraphics.hdgui2D(atts);

  for(auto &w: fAttributes)
    w->hdgui2D(atts);

  std::vector<std::string> l{};
  std::transform(atts.begin(), atts.end(), std::back_inserter(l), [](auto &att) {
    return re::mock::fmt::printf("  %s = %s", att.fName, att.fValue);
  });
  return re::mock::fmt::printf("jbox.%s {\n%s\n}", fType, re::mock::stl::join_to_string(l, "\n"));
}

//------------------------------------------------------------------------
// Widget::value
//------------------------------------------------------------------------
Widget *Widget::value()
{
  return addAttribute(std::make_unique<Value>());
}

//------------------------------------------------------------------------
// Widget::visibility
//------------------------------------------------------------------------
Widget *Widget::visibility()
{
  return addAttribute(std::make_unique<Visibility>());
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
std::unique_ptr<Widget> Widget::analog_knob()
{
  auto w = std::make_unique<Widget>("analog_knob");
  w ->value()
    ->visibility()
    ->tooltip_position()
    ->tooltip_template()
    ->show_remote_box()
    ->show_automation_rect()
    ;
  return w;
}


}

