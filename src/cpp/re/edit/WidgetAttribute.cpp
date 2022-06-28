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

#include "WidgetAttribute.h"
#include "ReGui.h"
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
  if(hasTexture())
    iCtx.drawTexture(getTexture(), fPosition, iFrameNumber, iBorderCol);
  else
  {
    iCtx.drawRectFilled(fPosition, getSize(), ImVec4{1, 0, 0, 1});
    if(iBorderCol.w > 0.0f)
      iCtx.drawRect(fPosition, getSize(), iBorderCol);
  }
}

//------------------------------------------------------------------------
// Graphics::editView
//------------------------------------------------------------------------
void Graphics::editView(std::vector<std::string> const &iTextureKeys,
                        std::function<void()> const &iOnReset,
                        std::function<void(std::string const &)> const &iOnTextureUpdate,
                        std::function<void(ImVec2 const &)> const &iOnSizeUpdate) const
{

  if(iOnReset)
  {
    if(ImGui::Button("X"))
      iOnReset();
    ImGui::SameLine();
  }

  ImGui::BeginGroup();
  auto const *texture = getTexture();
  auto key = texture ? texture->key() : "";
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


  if(!texture)
  {
    auto size = fSize;
    ImGui::Indent();
    auto updated = ReGui::InputInt("w", &size.x, 1, 5);
    updated |= ReGui::InputInt("h", &size.y, 1, 5);
    if(updated)
      iOnSizeUpdate(size);
    ImGui::Unindent();
  }
  ImGui::EndGroup();

}

//------------------------------------------------------------------------
// Graphics::editView
//------------------------------------------------------------------------
void Graphics::editView(EditContext const &iCtx)
{
  return editView(iCtx.getTextureKeys(),
                  {},
                  [this, &iCtx](auto &k) {
                    fTexture = iCtx.getTexture(k);
                  },
                  [this](auto &s) { fSize = s; }
                  );
}

//------------------------------------------------------------------------
// Graphics::reset
//------------------------------------------------------------------------
void Graphics::reset()
{
  fSize = fTexture ? fTexture->frameSize() : ImVec2{100, 100};
  fTexture = nullptr;
}

//------------------------------------------------------------------------
// Graphics::hdgui2D
//------------------------------------------------------------------------
void Graphics::hdgui2D(std::string const &iNodeName, attribute_list_t &oAttributes) const
{
  oAttributes.emplace_back(attribute_t{fName, re::mock::fmt::printf("{ node = { \"%s\" } }", iNodeName)});
}

}

