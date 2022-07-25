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
#include "Errors.h"
#include <re/mock/fmt.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace re::edit::widget {

//------------------------------------------------------------------------
// Attribute::toString
//------------------------------------------------------------------------
std::string Attribute::toString() const
{
  return re::mock::fmt::printf(R"(name="%s")", fName);
}

}

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
  if(fValue.empty())
    return "{}";
  std::vector<std::string> l{};
  std::transform(fValue.begin(), fValue.end(), std::back_inserter(l), escapeString);
  return re::mock::fmt::printf("{ %s }", re::mock::stl::join_to_string(l));
}

//------------------------------------------------------------------------
// DiscretePropertyValueList::getValueAsLua
//------------------------------------------------------------------------
std::string DiscretePropertyValueList::getValueAsLua() const
{
  if(fValue.empty())
    return "{}";
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
// DiscretePropertyValueList::contains
//------------------------------------------------------------------------
bool DiscretePropertyValueList::contains(int iValue) const
{
  return std::find(fValue.begin(), fValue.end(), iValue) != fValue.end();
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
void Value::editView(EditContext &iCtx)
{
  ImGui::BeginGroup();

  if(fUseSwitch)
  {
    fValueSwitch.editView(iCtx,
                          [this] { reset(); },
                          [this](const Property *p) {
                            fValueSwitch.fValue = p->path();
                            fValueSwitch.fProvided = true;
                            fValues.fValue.clear();
                            fValues.fValue.resize(p->stepCount());
                          },
                          [this](auto &iCtx) { editValueView(iCtx); },
                          [this](auto &iCtx) { tooltipView(iCtx); });
    if(ImGui::BeginPopup("Menu"))
    {
      ImGui::Separator();
      if(ImGui::MenuItem("Use value"))
        fUseSwitch = false;
      ImGui::EndPopup();
    }
    ImGui::Indent();
    fValues.editStaticListView(iCtx,
                               fValue.fFilter,
                               [this](int iIndex, const Property *p) { // onSelect
                                 fValues.fValue[iIndex] = p->path();
                                 fValues.fProvided = true;
                               });
    ImGui::Unindent();
  }
  else
  {
    fValue.editView(iCtx,
                    [this] { reset(); },
                    [this](const Property *p) {
                      fValue.fValue = p->path();
                      fValue.fProvided = true;
                    },
                    [this](auto &iCtx) { fValue.editPropertyView(iCtx); },
                    [this](auto &iCtx) { fValue.tooltipPropertyView(iCtx); });
    if(ImGui::BeginPopup("Menu"))
    {
      ImGui::Separator();
      if(ImGui::MenuItem("Use value_switch"))
        fUseSwitch = true;
      ImGui::EndPopup();
    }
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
// Value::editValueView
//------------------------------------------------------------------------
void Value::editValueView(EditContext &iCtx)
{
  if(fUseSwitch)
  {
    ImGui::PushID("Switch");
    fValueSwitch.editPropertyView(iCtx);
    ImGui::PopID();
    auto path = findActualPropertyPath(iCtx);
    if(!path.empty())
    {
      ImGui::PushID("Value");
      ImGui::Separator();
      PropertyPath::editPropertyView(iCtx, path);
      ImGui::PopID();
    }
  }
  else
  {
    fValue.editPropertyView(iCtx);
  }
}

//------------------------------------------------------------------------
// Value::tooltipView
//------------------------------------------------------------------------
void Value::tooltipView(EditContext &iCtx)
{
  if(fUseSwitch)
  {
    fValueSwitch.tooltipPropertyView(iCtx);
    auto path = findActualPropertyPath(iCtx);
    if(!path.empty())
    {
      ImGui::Separator();
      PropertyPath::tooltipPropertyView(iCtx, path);
    }
  }
  else
  {
    fValue.tooltipPropertyView(iCtx);
  }
}

//------------------------------------------------------------------------
// Value::findActualPropertyPath
//------------------------------------------------------------------------
std::string const &Value::findActualPropertyPath(EditContext &iCtx) const
{
  static const std::string kNoProperty{};

  if(fUseSwitch)
  {
    if(fValueSwitch.fValue.empty())
      return kNoProperty;
    else
    {
      auto index = iCtx.getPropertyValueAsInt(fValueSwitch.fValue);
      RE_EDIT_INTERNAL_ASSERT(fValues.fValue.size() > index);
      return fValues.fValue.at(index);
    }
  }
  else
  {
    return fValue.fValue;
  }
}

//------------------------------------------------------------------------
// Value::toString
//------------------------------------------------------------------------
std::string Value::toString() const
{
  return re::mock::fmt::printf(R"(%s={fUseSwitch=%s,%s,%s,%s})",
                               fName,
                               fUseSwitch ? "true" : "false",
                               fValue.toString(),
                               fValueSwitch.toString(),
                               fValues.toString());
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
void Visibility::editView(EditContext &iCtx)
{
  static const std::string EMPTY_LIST_ERROR = "You must provide at least 1 value";

  ImGui::PushID(this);

  fSwitch.editView(iCtx,
                   [this] () { reset(); }, // onReset
                   [this] (const Property *p) { // onSelect
                     fSwitch.fValue = p->path();
                     fSwitch.fProvided = true;
                     fValues.fValue = {0};
                     fValues.fProvided = true;
                   },
                   [this](auto &iCtx) { fSwitch.editPropertyView(iCtx); },
                   [this](auto &iCtx) { fSwitch.tooltipPropertyView(iCtx); });

  auto property = iCtx.findProperty(fSwitch.fValue);
  if(property)
  {
    auto stepCount = property->stepCount();
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
  ImGui::PopID();
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
// Visibility::isHidden
//------------------------------------------------------------------------
bool Visibility::isHidden(DrawContext const &iCtx) const
{
  auto switchPropertyPath = fSwitch.fValue;
  if(switchPropertyPath.empty())
    return false;
  else
    return !fValues.contains(iCtx.getPropertyValueAsInt(switchPropertyPath));
}

//------------------------------------------------------------------------
// Visibility::toString
//------------------------------------------------------------------------
std::string Visibility::toString() const
{
  return re::mock::fmt::printf(R"(%s={%s,%s})",
                               fName,
                               fSwitch.toString(),
                               fValues.toString());
}

static constexpr auto kIsDiscreteFilter = [](const Property &p) { return p.isDiscrete(); };

//------------------------------------------------------------------------
// Visibility::Visibility
//------------------------------------------------------------------------
Visibility::Visibility() : Attribute("visibility"), fSwitch{"visibility_switch", kIsDiscreteFilter} {}

//------------------------------------------------------------------------
// String::editView
//------------------------------------------------------------------------
void String::editView(EditContext &iCtx)
{
  resetView();
  ImGui::SameLine();
  if(ImGui::InputText(fName.c_str(), &fValue))
    fProvided = true;
}

//------------------------------------------------------------------------
// Bool::editView
//------------------------------------------------------------------------
void Bool::editView(EditContext &iCtx)
{
  resetView();
  ImGui::SameLine();
  if(ImGui::Checkbox(fName.c_str(), &fValue))
    fProvided = true;
}

//------------------------------------------------------------------------
// Integer::editView
//------------------------------------------------------------------------
void Integer::editView(EditContext &iCtx)
{
  resetView();
  ImGui::SameLine();
  if(ImGui::InputInt(fName.c_str(), &fValue))
    fProvided = true;
}

//------------------------------------------------------------------------
// PropertyPath::editView
//------------------------------------------------------------------------
void PropertyPath::editView(EditContext &iCtx)
{
  editView(iCtx,
           [this] { reset(); },
           [this](const Property *p) {
             fValue = p->path();
             fProvided = true;
           },
           [this](auto &iCtx) { editPropertyView(iCtx); },
           [this](auto &iCtx) { tooltipPropertyView(iCtx); });
}

//------------------------------------------------------------------------
// PropertyPath::editView
//------------------------------------------------------------------------
void PropertyPath::editView(EditContext &iCtx,
                            std::function<void()> const &iOnReset,
                            std::function<void(const Property *)> const &iOnSelect,
                            std::function<void(EditContext &iCtx)> const &iEditPropertyView,
                            std::function<void(EditContext &iCtx)> const &iTooltipPropertyView)
{
  menuView(iCtx, iOnReset, iEditPropertyView);

  ImGui::SameLine();

  if(ImGui::BeginCombo(fName.c_str(), fValue.c_str()))
  {
    auto properties = iCtx.findProperties(fFilter);
    for(auto &p: properties)
    {
      auto const isSelected = p->path() == fValue;
      if(ImGui::Selectable(p->path().c_str(), isSelected))
        iOnSelect(p);
      if(isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  if(!fValue.empty())
  {
    if(ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
      iTooltipPropertyView(iCtx);
      ImGui::PopTextWrapPos();
      ImGui::EndTooltip();
    }
  }
}

//------------------------------------------------------------------------
// PropertyPath::menuView
//------------------------------------------------------------------------
void PropertyPath::menuView(EditContext &iCtx,
                            std::string const &iPropertyPath,
                            std::function<void()> const &iOnReset,
                            std::function<void(EditContext &iCtx)> const &iEditPropertyView)
{
  if(ImGui::Button("."))
    ImGui::OpenPopup("Menu");

  auto editPopupId = ImGui::GetID("Edit_popup");

  if(ImGui::BeginPopup("Menu"))
  {
    if(ImGui::MenuItem("Reset"))
      iOnReset();

    ImGui::BeginDisabled(iPropertyPath.empty());
    if(ImGui::MenuItem("Watch"))
      iCtx.addPropertyToWatchlist(iPropertyPath);
    if(ImGui::MenuItem("Edit"))
      ImGui::OpenPopup(editPopupId);
    ImGui::EndDisabled();

    ImGui::EndPopup();
  }

  if(ImGui::BeginPopup("Edit_popup"))
  {
    if(iEditPropertyView)
      iEditPropertyView(iCtx);
    else
      PropertyPath::editPropertyView(iCtx, iPropertyPath);
    if(ImGui::Button("Ok"))
      ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
  }
}

//------------------------------------------------------------------------
// PropertyPath::editPropertyView
//------------------------------------------------------------------------
void PropertyPath::editPropertyView(EditContext &iCtx, std::string const &iPropertyPath)
{
  ImGui::Text("%s", iPropertyPath.c_str());
  iCtx.propertyEditView(iPropertyPath);
}

//------------------------------------------------------------------------
// PropertyPath::tooltipPropertyView
//------------------------------------------------------------------------
void PropertyPath::tooltipPropertyView(EditContext &iCtx, std::string const &iPropertyPath)
{
  ImGui::TextUnformatted(iCtx.getPropertyInfo(iPropertyPath).c_str());
}

//------------------------------------------------------------------------
// PropertyPath::menuView
//------------------------------------------------------------------------
void PropertyPath::menuView(EditContext &iCtx,
                            std::function<void()> const &iOnReset,
                            std::function<void(EditContext &iCtx)> const &iEditPropertyView)
{
  menuView(iCtx, fValue, iOnReset, iEditPropertyView);
}

//------------------------------------------------------------------------
// ObjectPath::editView
//------------------------------------------------------------------------
void ObjectPath::editView(EditContext &iCtx)
{
  resetView();

  ImGui::SameLine();

  if(ImGui::BeginCombo(fName.c_str(), fValue.c_str()))
  {
    auto objects = iCtx.findObjects(fFilter);
    for(auto &o: objects)
    {
      auto const isSelected = o->path() == fValue;
      if(ImGui::Selectable(o->path().c_str(), isSelected))
      {
        fValue = o->path();
        fProvided = true;
      }
      if(isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
}

//------------------------------------------------------------------------
// ObjectPath::editView
//------------------------------------------------------------------------
void Socket::editView(EditContext &iCtx)
{
  ObjectPath::editView(iCtx);

  if(!fValue.empty())
  {
    if(ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
      ImGui::TextUnformatted(iCtx.getPropertyInfo(re::mock::fmt::printf("%s/%s", fValue, "connected")).c_str());
      switch(fObjectType)
      {
        case re::mock::JboxObjectType::kAudioOutput:
        case re::mock::JboxObjectType::kCVOutput:
          ImGui::TextUnformatted(iCtx.getPropertyInfo(re::mock::fmt::printf("%s/%s", fValue, "dsp_latency")).c_str());
          break;
        default:
          // nothing to do
          break;
      }
      ImGui::PopTextWrapPos();
      ImGui::EndTooltip();
    }
  }
}

//------------------------------------------------------------------------
// PropertyPathList::editStaticListView
//------------------------------------------------------------------------
void PropertyPathList::editStaticListView(EditContext &iCtx,
                                          Property::Filter const &iFilter,
                                          std::function<void(int iIndex, const Property *)> const &iOnSelect) const
{
  for(int i = 0; i < fValue.size(); i++)
  {
    ImGui::PushID(i);

    auto &value = fValue[i];
    if(ImGui::BeginCombo(re::mock::fmt::printf("%s [%d]", fName, i).c_str(), value.c_str()))
    {
      auto properties = iCtx.findProperties(iFilter);
      for(auto &p: properties)
      {
        auto const isSelected = p->path() == value;

        if(ImGui::Selectable(p->path().c_str(), isSelected))
          iOnSelect(i, p);

        if(isSelected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    if(!value.empty())
    {
      if(ImGui::IsItemHovered())
      {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(iCtx.getPropertyInfo(value).c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
      }
    }

    ImGui::PopID();
  }
}

//------------------------------------------------------------------------
// PropertyPathList::editView
//------------------------------------------------------------------------
void PropertyPathList::editView(EditContext &iCtx)
{
  resetView();

  ImGui::SameLine();

  auto const popupTitleName = re::mock::fmt::printf("%s Editor", fName);

  if(ImGui::Button(re::mock::fmt::printf("[%d] properties", fValue.size()).c_str(), ImVec2{ImGui::CalcItemWidth(), 0}))
  {
    ImGui::OpenPopup(popupTitleName.c_str());
    fStringListEditView = views::StringListEdit(iCtx.findPropertyNames(fFilter),
                                                "Properties",
                                                true,
                                                fValue,
                                                fName,
                                                false);
  }
  ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
  ImGui::Text("%s", fName.c_str());

  if(fStringListEditView)
  {
    // Always center this window when appearing
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Once, ImVec2(0.5f, 0.5f));

    if(ImGui::BeginPopupModal(popupTitleName.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
      fStringListEditView->editView();

      if(ImGui::Button("OK", ImVec2(120, 0)))
      {
        fValue = std::move(fStringListEditView->destination());
        fStringListEditView = std::nullopt;
        fProvided = true;
        ImGui::CloseCurrentPopup();
      }
      ImGui::SetItemDefaultFocus();
      ImGui::SameLine();
      if(ImGui::Button("Cancel", ImVec2(120, 0)))
      {
        fStringListEditView = std::nullopt;
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  }

}

//------------------------------------------------------------------------
// StaticStringList::editView
//------------------------------------------------------------------------
void StaticStringList::editView(EditContext &iCtx)
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


}

