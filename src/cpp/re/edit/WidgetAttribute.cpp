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
#include "WidgetAttribute.hpp"
#include "Widget.h"
#include "Errors.h"
#include <re/mock/fmt.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace re::edit {

//------------------------------------------------------------------------
// Editable::checkForErrors
//------------------------------------------------------------------------
bool Editable::checkForErrors(re::edit::AppContext &iCtx)
{
  bool res;

  if(fEdited)
  {
    fUserError.clear();
    findErrors(iCtx, fUserError);
    res = fUserError.hasErrors();
    resetEdited();
  }
  else
    res = fUserError.hasErrors();

  return res;
}

//------------------------------------------------------------------------
// Editable::errorView
//------------------------------------------------------------------------
bool Editable::errorView()
{
  if(hasErrors())
  {
    ReGui::ErrorIcon();
    if(ReGui::ShowTooltip())
    {
      ReGui::ToolTip([this] {
        for(auto const &error: getErrors())
          ImGui::TextUnformatted(error.c_str());
      });
    }
    return true;
  }

  return false;
}

}

namespace re::edit::widget {

//------------------------------------------------------------------------
// Attribute::Attribute
//------------------------------------------------------------------------
Attribute::Attribute(Attribute const &iOther) :
  fParent{nullptr},
  fWidgetType{iOther.fWidgetType},
  fId{iOther.fId},
  fName{iOther.fName},
  fRequired{iOther.fRequired}
{
}

//------------------------------------------------------------------------
// Attribute::init
//------------------------------------------------------------------------
void Attribute::init(Widget *iParent, int id)
{
  fParent = iParent;
  fWidgetType = fParent->getType();
  fId = id;
}

//------------------------------------------------------------------------
// Attribute::toString
//------------------------------------------------------------------------
std::string Attribute::toString() const
{
  return re::mock::fmt::printf(R"(name="%s")", fName);
}

//------------------------------------------------------------------------
// Attribute::copyFrom
//------------------------------------------------------------------------
bool Attribute::copyFrom(Attribute const *iFromAttribute)
{
  return update([this, iFromAttribute]{ this->copyFromAction(iFromAttribute); }, computeUpdateAttributeDescription());
}

//------------------------------------------------------------------------
// Attribute::resetAttribute
//------------------------------------------------------------------------
bool Attribute::resetAttribute(Attribute *iAttributeForDescription)
{
  return update([this] { reset(); },
                fmt::printf("Reset %s.%s", getParent()->getName(), iAttributeForDescription ? iAttributeForDescription->fName : fName));
}

//------------------------------------------------------------------------
// Attribute::computeAttributeChangeDescription
//------------------------------------------------------------------------
std::string Attribute::computeAttributeChangeDescription(char const *iChangeAction, Attribute *iAttribute, std::optional<int> iIndex) const
{
  if(iIndex)
    return fmt::printf("%s %s.%s[%d]", iChangeAction, getParent()->getName(), iAttribute ? iAttribute->fName : fName, *iIndex);
  else
    return fmt::printf("%s %s.%s", iChangeAction, getParent()->getName(), iAttribute ? iAttribute->fName : fName);
}

//------------------------------------------------------------------------
// toUIText
//------------------------------------------------------------------------
std::string toUIText(std::string const &s)
{
  return re::mock::fmt::printf("jbox.ui_text(\"%s\")", s);
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
  return toUIText(fValue);
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
                                         std::function<void(int iIndex)>             const &iOnDelete)
{
  auto const offset = ImGui::GetCursorPosX();
  auto const itemWidth = AppContext::GetCurrent().fItemWidth;

  int deleteItemIdx = -1;
  for(int i = 0; i < fValue.size(); i++)
  {
    ImGui::PushID(i);
    if(ImGui::Button("-"))
      deleteItemIdx = i;
    ImGui::SameLine();

    ImGui::PushItemWidth(itemWidth - (ImGui::GetCursorPosX() - offset));

    int editedValue = fValue[i];
    if(ImGui::SliderInt(re::mock::fmt::printf("%s [%d]", fName, i).c_str(), &editedValue, iMin, iMax))
      iOnUpdate(i, editedValue);

    ImGui::PopItemWidth();

    ImGui::PopID();
  }

  if(deleteItemIdx >= 0)
    iOnDelete(deleteItemIdx);

  ImGui::PushID(static_cast<int>(fValue.size()));

  if(ImGui::Button("+"))
    iOnAdd();

  ImGui::SameLine();
  ImGui::PushItemWidth(itemWidth - (ImGui::GetCursorPosX() - offset));
  ImGui::LabelText(fName, "Click + to add");
  ImGui::PopItemWidth();

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
void Value::editView(AppContext &iCtx)
{
  ImGui::BeginGroup();

  if(fUseSwitch)
  {
    ImVec2 p;
    fValueSwitch.editView(iCtx,
                          [this] {
                            resetAttribute(&fValueSwitch);
                            },
                          [this, &iCtx] {
                            iCtx.copyToClipboard(this);
                          }, // onCopy
                          [this](const Property *p) {
                            if(updateAttribute([this, p] {
                              fValueSwitch.fValue = p->path();
                              fValueSwitch.fProvided = true;
                              fValues.fValue.clear();
                              fValues.fValue.resize(p->stepCount());
                            }, &fValueSwitch))
                            {
                              fValueSwitch.markEdited();
                              fValues.markEdited();
                            }
                          },
                          [this](auto &iCtx) { editValueView(iCtx); },
                          [this](auto &iCtx) { tooltipView(iCtx); },
                          &p);
    if(ImGui::BeginPopup("Menu"))
    {
      ImGui::Separator();
      if(ImGui::MenuItem("Use value"))
      {
        updateAttribute([this]{
          fUseSwitch = false;
        });
      }
      ImGui::EndPopup();
    }
    if(fValueSwitch.fProvided)
    {
      ImGui::SetCursorPosX(p.x);
      ImGui::BeginGroup();
      fValues.editStaticListView(iCtx,
                                 fValue.fFilter,
                                 [this](int iIndex, const Property *p) { // onSelect
                                   if(update([this, iIndex, p] {
                                               fValues.fValue[iIndex] = p->path();
                                               fValues.fProvided = true;
                                             },
                                             computeUpdateAttributeDescription(&fValues, iIndex)))
                                   {
                                     fValues.markEdited();
                                   }
                                 });
      ImGui::EndGroup();
    }
  }
  else
  {
    fValue.editView(iCtx,
                    [this] {
                      resetAttribute(&fValue);
                    },
                    [this, &iCtx] {
                      iCtx.copyToClipboard(this);
                    }, // onCopy
                    [this](const Property *p) {
                      updateAttribute([this, p]{
                        fValue.fValue = p->path();
                        fValue.fProvided = true;
                      }, &fValue);
                    },
                    [this](auto &iCtx) { fValue.editPropertyView(iCtx); },
                    [this](auto &iCtx) { fValue.tooltipPropertyView(iCtx); });
    if(ImGui::BeginPopup("Menu"))
    {
      ImGui::Separator();
      if(ImGui::MenuItem("Use value_switch"))
      {
        updateAttribute([this] {
          fUseSwitch = true;
        });
      }
      ImGui::EndPopup();
    }
  }

  fEdited |= fValue.isEdited() || fValueSwitch.isEdited() || fValues.isEdited();

  ImGui::EndGroup();
}

//------------------------------------------------------------------------
// Value::reset
//------------------------------------------------------------------------
void Value::reset()
{
  fValue.reset();
  fValueSwitch.reset();
  fValues.reset();
  fUseSwitch = false;
  fEdited = true;
}

//------------------------------------------------------------------------
// Value::editValueView
//------------------------------------------------------------------------
void Value::editValueView(AppContext &iCtx)
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
void Value::tooltipView(AppContext &iCtx)
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
std::string const &Value::findActualPropertyPath(AppContext &iCtx) const
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
                               fmt::Bool::to_chars(fUseSwitch),
                               fValue.toString(),
                               fValueSwitch.toString(),
                               fValues.toString());
}


//------------------------------------------------------------------------
// Value::toValueString
//------------------------------------------------------------------------
std::string Value::toValueString() const
{
  if(fUseSwitch)
    return fmt::printf("%s = \"%s\" ([%ld] values)", fValueSwitch.fName, fValueSwitch.fValue, fValues.fValue.size());
  else
    return fmt::printf("%s = \"%s\"", fValue.fName, fValue.fValue);
}

//------------------------------------------------------------------------
// Value::findErrors
//------------------------------------------------------------------------
void Value::findErrors(AppContext &iCtx, UserError &oErrors) const
{
  if(fUseSwitch)
  {
    if(!fValueSwitch.fProvided)
      oErrors.add("Either value or value_switch required");
    else
    {
      fValueSwitch.findErrors(iCtx, oErrors);
      fValues.findErrors(iCtx, oErrors);
    }
  }
  else
  {
    if(!fValue.fProvided)
      oErrors.add("Either value or value_switch required");
    else
      fValue.findErrors(iCtx, oErrors);
  }
}

//------------------------------------------------------------------------
// Value::markEdited
//------------------------------------------------------------------------
void Value::markEdited()
{
  fEdited = true;
  fValue.markEdited();
  fValueSwitch.markEdited();
  fValues.markEdited();
}

//------------------------------------------------------------------------
// Value::resetEdited
//------------------------------------------------------------------------
void Value::resetEdited()
{
  fEdited = false;
  fValue.resetEdited();
  fValueSwitch.resetEdited();
  fValues.resetEdited();
}

//------------------------------------------------------------------------
// Value::copyFromAction
//------------------------------------------------------------------------
bool Value::copyFromAction(Attribute const *iAttribute)
{
  auto fromAttribute = dynamic_cast<Value const *>(iAttribute);
  if(fromAttribute)
  {
    fUseSwitch = fromAttribute->fUseSwitch;
    fValue.copyFromAction(&fromAttribute->fValue);
    fValueSwitch.copyFromAction(&fromAttribute->fValueSwitch);
    fValues.copyFromAction(&fromAttribute->fValues);
    fEdited = true;
    return true;
  }

  auto pathAttribute = dynamic_cast<PropertyPath const *>(iAttribute);
  if(pathAttribute)
  {
    reset();
    fValue.fValue = pathAttribute->fValue;
    return true;
  }

  return false;
}

//------------------------------------------------------------------------
// Value::updateFilters
//------------------------------------------------------------------------
void Value::updateFilters(Property::Filter iValueFilter, Property::Filter iValueSwitchFilter)
{
  fValue.updateFilter(std::move(iValueFilter));
  fValues.updateFilter(std::move(iValueSwitchFilter));
  fEdited = true;
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
void Visibility::editView(AppContext &iCtx)
{
  ImGui::PushID(this);

  ImVec2 p;
  fSwitch.editView(iCtx,
                   [this] () {
                     resetAttribute(&fSwitch);
                   }, // onReset
                   [this, &iCtx] {
                     iCtx.copyToClipboard(this);
                   }, // onCopy
                   [this] (const Property *p) { // onSelect
                     if(updateAttribute([this, p] {
                                          fSwitch.fValue = p->path();
                                          fSwitch.fProvided = true;
                                          fValues.fValue = {0};
                                          fValues.fProvided = true;
                                        },
                                        &fSwitch
                     ))
                     {
                       fSwitch.markEdited();
                       fValues.markEdited();
                     }
                   },
                   [this](auto &iCtx) { fSwitch.editPropertyView(iCtx); },
                   [this](auto &iCtx) { fSwitch.tooltipPropertyView(iCtx); },
                   &p);

  auto property = iCtx.findProperty(fSwitch.fValue);
  if(property)
  {
    auto stepCount = property->stepCount();
    if(stepCount > 1)
    {
      ImGui::SetCursorPosX(p.x);
      ImGui::BeginGroup();
      fValues.editView(0, stepCount - 1,
                       [this]() { // onAdd
                         if(updateAttribute([this] {
                                              fValues.fValue.emplace_back(0);
                                              fValues.fProvided = true;
                                            },
                                            &fValues))
                         {
                           fValues.markEdited();
                         }
                       },
                       [this](int iIndex, int iValue) { // onUpdate
                         if(update([this, iIndex, iValue] {
                                     fValues.fValue[iIndex] = iValue;
                                     fValues.fProvided = true;
                                   },
                                   computeUpdateAttributeDescription(&fValues, iIndex),
                                   MergeKey::from(&fValues.fValue[iIndex])))
                         {
                           fValues.markEdited();
                         }
                       },
                       [this](int iIndex) { // onDelete
                         if(updateAttribute([this, iIndex] {
                                              fValues.fValue.erase(fValues.fValue.begin() + iIndex);
                                              fValues.fProvided = true;
                                            },
                                            &fValues))
                         {
                           fValues.markEdited();
                         }
                       }
      );
      ImGui::EndGroup();
    }
  }
  ImGui::PopID();

  fEdited |= fSwitch.isEdited() || fValues.isEdited();
}

//------------------------------------------------------------------------
// Visibility::findErrors
//------------------------------------------------------------------------
void Visibility::findErrors(AppContext &iCtx, UserError &oErrors) const
{
  auto property = iCtx.findProperty(fSwitch.fValue);
  if(property)
  {
    auto const stepCount = property->stepCount();

    if(stepCount == 0)
      oErrors.add("The property must be a discrete property");

    if(fValues.fValue.empty())
      oErrors.add("You must provide at least 1 value");

    int i = 0;
    for(auto v: fValues.fValue)
    {
      if(v < 0 || v >= stepCount)
        oErrors.add("Invalid value [%d] (%d outside of bound)", i, v);
      i++;
    }
  }
  else
  {
    if(fSwitch.fProvided)
      fSwitch.findErrors(iCtx, oErrors);
  }
}

//------------------------------------------------------------------------
// Visibility::markEdited
//------------------------------------------------------------------------
void Visibility::markEdited()
{
  fEdited = true;
  fSwitch.markEdited();
  fValues.markEdited();
}

//------------------------------------------------------------------------
// Visibility::resetEdited
//------------------------------------------------------------------------
void Visibility::resetEdited()
{
  fEdited = false;
  fSwitch.resetEdited();
  fValues.resetEdited();
}


//------------------------------------------------------------------------
// Visibility::reset
//------------------------------------------------------------------------
void Visibility::reset()
{
  fSwitch.reset();
  fValues.reset();
  fEdited = true;
}

//------------------------------------------------------------------------
// Visibility::isHidden
//------------------------------------------------------------------------
bool Visibility::isHidden(AppContext const &iCtx) const
{
  auto switchPropertyPath = fSwitch.fValue;
  if(switchPropertyPath.empty() || fValues.empty())
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


//------------------------------------------------------------------------
// Visibility::toValueString
//------------------------------------------------------------------------
std::string Visibility::toValueString() const
{
  return fmt::printf("%s = \"%s\" ([%ld] values)", fSwitch.fName, fSwitch.fValue, fValues.fValue.size());
}

static const Property::Filter kIsDiscreteFilter{[](const Property &p) { return p.isDiscrete(); }, "Must be a discrete (stepped) number property"};

//------------------------------------------------------------------------
// Visibility::Visibility
//------------------------------------------------------------------------
Visibility::Visibility() : CompositeAttribute("visibility"), fSwitch{"visibility_switch", kIsDiscreteFilter} {}

//------------------------------------------------------------------------
// Visibility::copyFromAction
//------------------------------------------------------------------------
bool Visibility::copyFromAction(Attribute const *iAttribute)
{
  auto fromAttribute = dynamic_cast<Visibility const *>(iAttribute);
  if(fromAttribute)
  {
    fSwitch.copyFromAction(&fromAttribute->fSwitch);
    fValues.copyFromAction(&fromAttribute->fValues);
    fEdited = true;
    return true;
  }
  else
    return false;
}

//------------------------------------------------------------------------
// String::editView
//------------------------------------------------------------------------
void String::editView(AppContext &iCtx)
{
  menuView(iCtx);
  ImGui::SameLine();

  auto editedValue = fValue;

  if(ImGui::InputText(fName, &editedValue))
  {
    mergeUpdate(editedValue);
  }
}

//------------------------------------------------------------------------
// Bool::editView
//------------------------------------------------------------------------
void Bool::editView(AppContext &iCtx)
{
  menuView(iCtx);
  ImGui::SameLine();
  bool editedValue = fValue;
  if(ImGui::Checkbox(fName, &editedValue))
  {
    update(editedValue);
  }
}

//------------------------------------------------------------------------
// Integer::editView
//------------------------------------------------------------------------
void Integer::editView(AppContext &iCtx)
{
  menuView(iCtx);
  ImGui::SameLine();

  auto editedValue = fValue;

  if(ImGui::InputInt(fName, &editedValue))
  {
    mergeUpdate(editedValue);
  }
}

//------------------------------------------------------------------------
// PropertyPath::editView
//------------------------------------------------------------------------
void PropertyPath::editView(AppContext &iCtx)
{
  editView(iCtx,
           [this] {
             resetAttribute();
           },
           [this, &iCtx] {
             iCtx.copyToClipboard(this);
           },
           [this](const Property *p) {
             update(p->path());
           },
           [this](auto &iCtx) { this->editPropertyView(iCtx); },
           [this](auto &iCtx) { this->tooltipPropertyView(iCtx); });
}

//------------------------------------------------------------------------
// PropertyPath::editView
//------------------------------------------------------------------------
void PropertyPath::editView(AppContext &iCtx,
                            std::function<void()> const &iOnReset,
                            std::function<void()> const &iOnCopy,
                            std::function<void(const Property *)> const &iOnSelect,
                            std::function<void(AppContext &iCtx)> const &iEditPropertyView,
                            std::function<void(AppContext &iCtx)> const &iTooltipPropertyView,
                            ImVec2 *oComboPosition)
{
  menuView(iCtx, iOnReset, iOnCopy, iEditPropertyView);

  ImGui::SameLine();

  if(oComboPosition)
    *oComboPosition = ImGui::GetCursorPos();

  if(ImGui::BeginCombo(fName, fValue.c_str()))
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
    if(ReGui::ShowQuickView())
    {
      ReGui::ToolTip([&iCtx, &iTooltipPropertyView] {
        iTooltipPropertyView(iCtx);
      });
    }
  }
}

//------------------------------------------------------------------------
// PropertyPath::menuView
//------------------------------------------------------------------------
void PropertyPath::menuView(AppContext &iCtx,
                            std::string const &iPropertyPath,
                            std::function<void()> const &iOnReset,
                            std::function<void()> const &iOnCopy,
                            std::function<void(AppContext &iCtx)> const &iEditPropertyView)
{
  if(ReGui::MenuButton())
    ImGui::OpenPopup("Menu");

  auto editPopupId = ImGui::GetID("Edit_popup");

  if(ImGui::BeginPopup("Menu"))
  {
    if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Reset, "Reset")))
      iOnReset();

    // Copy
    if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Copy, "Copy")))
      iOnCopy();

    ImGui::BeginDisabled(iPropertyPath.empty());
    if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Watch, "Watch")))
      iCtx.addPropertyToWatchlist(iPropertyPath);
    if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Edit, "Edit")))
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
void PropertyPath::editPropertyView(AppContext &iCtx, std::string const &iPropertyPath)
{
  ImGui::Text("%s", iPropertyPath.c_str());
  iCtx.propertyEditView(iPropertyPath);
}

//------------------------------------------------------------------------
// PropertyPath::tooltipPropertyView
//------------------------------------------------------------------------
void PropertyPath::tooltipPropertyView(AppContext &iCtx, std::string const &iPropertyPath)
{
  ImGui::TextUnformatted(iCtx.getPropertyInfo(iPropertyPath).c_str());
}

//------------------------------------------------------------------------
// PropertyPath::menuView
//------------------------------------------------------------------------
void PropertyPath::menuView(AppContext &iCtx,
                            std::function<void()> const &iOnReset,
                            std::function<void()> const &iOnCopy,
                            std::function<void(AppContext &iCtx)> const &iEditPropertyView)
{
  menuView(iCtx, fValue, iOnReset, iOnCopy, iEditPropertyView);
}

//------------------------------------------------------------------------
// PropertyPath::menuView
//------------------------------------------------------------------------
void PropertyPath::findErrors(AppContext &iCtx, UserError &oErrors) const
{
  if(fProvided)
  {
    auto property = iCtx.findProperty(fValue);
    if(!property)
      oErrors.add("Invalid property (missing from motherboard)");
    else
    {
      if(fFilter)
      {
        auto properties = iCtx.findProperties(fFilter);
        if(std::find(properties.begin(), properties.end(), property) == properties.end())
          oErrors.add("Invalid property (%s)", fFilter.fDescription);
      }
    }
  }
  else
  {
    if(fRequired)
      oErrors.add("Required");
  }
}

//------------------------------------------------------------------------
// PropertyPath::copyFromAction
//------------------------------------------------------------------------
bool PropertyPath::copyFromAction(Attribute const *iFromAttribute)
{
  if(SingleAttribute::copyFromAction(iFromAttribute))
    return true;

  auto valueAttribute = dynamic_cast<Value const *>(iFromAttribute);
  if(valueAttribute)
  {
    if(!valueAttribute->fUseSwitch)
      return copyFromAction(&valueAttribute->fValue);
  }

  return false;
}

//------------------------------------------------------------------------
// ObjectPath::editView
//------------------------------------------------------------------------
void ObjectPath::editView(AppContext &iCtx)
{
  menuView(iCtx);

  ImGui::SameLine();

  if(ImGui::BeginCombo(fName, fValue.c_str()))
  {
    auto objects = iCtx.findObjects(fFilter);
    for(auto &o: objects)
    {
      auto const isSelected = o->path() == fValue;
      if(ImGui::Selectable(o->path().c_str(), isSelected))
      {
        update(o->path());
      }
      if(isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
}

//------------------------------------------------------------------------
// ObjectPath::findErrors
//------------------------------------------------------------------------
void ObjectPath::findErrors(AppContext &iCtx, UserError &oErrors) const
{
  if(fProvided)
  {
    auto object = iCtx.findObject(fValue);
    if(!object)
      oErrors.add("Invalid (missing from motherboard)");
    else
    {
      if(fFilter)
      {
        auto objects = iCtx.findObjects(fFilter);
        if(std::find(objects.begin(), objects.end(), object) == objects.end())
          oErrors.add("Invalid (wrong type)");
      }
    }
  }
  else
  {
    if(fRequired)
      oErrors.add("Required");
  }
}

//------------------------------------------------------------------------
// ObjectPath::editView
//------------------------------------------------------------------------
void Socket::editView(AppContext &iCtx)
{
  ObjectPath::editView(iCtx);

  if(!fValue.empty())
  {
    if(ReGui::ShowQuickView())
    {
      ReGui::ToolTip([this, &iCtx] {
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
      });
    }
  }
}

//------------------------------------------------------------------------
// PropertyPathList::editStaticListView
//------------------------------------------------------------------------
void PropertyPathList::editStaticListView(AppContext &iCtx,
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
      if(ReGui::ShowQuickView())
      {
        ReGui::ToolTip([&iCtx, &value] {
          ImGui::TextUnformatted(iCtx.getPropertyInfo(value).c_str());
        });
      }
    }

    ImGui::PopID();
  }
}

//------------------------------------------------------------------------
// PropertyPathList::editView
//------------------------------------------------------------------------
void PropertyPathList::editView(AppContext &iCtx)
{
  menuView(iCtx);

  ImGui::SameLine();

  auto const popupTitleName = re::mock::fmt::printf("%s Editor", fName);

  if(ImGui::Button(re::mock::fmt::printf("[%d] properties", fValue.size()).c_str(), ImVec2{ImGui::CalcItemWidth(), 0}))
  {
    auto sortBy = [&iCtx, this](std::vector<std::string> &ioString, std::string const &iSortCriteria) {
      fSortCriteria = iSortCriteria;
      if(iSortCriteria == "Path")
        iCtx.sortProperties(ioString, kByPathComparator);
      else
        iCtx.sortProperties(ioString, kByTagComparator);
    };
    ImGui::OpenPopup(popupTitleName.c_str());
    fStringListEditView = views::StringListEdit(iCtx.findPropertyNames(fFilter),
                                                "Properties",
                                                sortBy,
                                                {"Path", "Tag"},
                                                fSortCriteria,
                                                fValue,
                                                fName);
  }
  ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
  ImGui::Text("%s", fName);

  if(fStringListEditView)
  {
    // Always center this window
    ReGui::CenterNextWindow();

    if(ImGui::BeginPopupModal(popupTitleName.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
      fStringListEditView->editView();

      if(ImGui::Button("OK", ImVec2(120, 0)))
      {
        updateAttribute([this] {
          fValue = std::move(fStringListEditView->destination());
          fStringListEditView = std::nullopt;
          fProvided = true;
        });
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
// PropertyPathList::findErrors
//------------------------------------------------------------------------
void PropertyPathList::findErrors(AppContext &iCtx, UserError &oErrors) const
{
  auto properties = iCtx.findProperties(fFilter);

  auto idx = 0;

  for(auto &p: fValue)
  {
    if(p.empty())
      oErrors.add("Required property [%d]", idx);
    else
    {
      auto property = iCtx.findProperty(p);

      if(!property)
        oErrors.add("Invalid property [%d] (%s | missing from motherboard)", idx, p);

      if(!properties.empty())
      {
        if(std::find(properties.begin(), properties.end(), property) == properties.end())
          oErrors.add("Invalid property [%d] (%s | %s)", idx, p, fFilter.fDescription);
      }
    }

    idx++;
  }
}

//------------------------------------------------------------------------
// StaticStringList::editView
//------------------------------------------------------------------------
void StaticStringList::editView(AppContext &iCtx)
{
  menuView(iCtx);
  ImGui::SameLine();
  if(ImGui::BeginCombo(fName, fValue.c_str()))
  {
    for(auto &p: fSelectionList)
    {
      auto const isSelected = p == fValue;
      if(ImGui::Selectable(p.c_str(), isSelected))
      {
        update(p);
      }
      if(isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
}

//------------------------------------------------------------------------
// Index::editView
//------------------------------------------------------------------------
void Index::editView(AppContext &iCtx)
{
  auto valueAtt = getParent()->findAttributeByIdAndType<PropertyPath>(fValueAttributeId);

  auto property = iCtx.findProperty(valueAtt->fValue);
  if(property)
  {
    menuView(iCtx);
    ImGui::SameLine();
    auto editedValue = fValue;
    if(ImGui::SliderInt(fName, &editedValue, 0, property->stepCount() - 1))
    {
      mergeUpdate(editedValue);
    }
  }
  else
  {
    // no property => use "normal" input
    Integer::editView(iCtx);
  }

  // sanity check as the following line will have no effect unless valueAtt is processed first
  RE_EDIT_INTERNAL_ASSERT(valueAtt->fId < fId);
  fEdited |= valueAtt->isEdited();
}

//------------------------------------------------------------------------
// Index::findErrors
//------------------------------------------------------------------------
void Index::findErrors(AppContext &iCtx, UserError &oErrors) const
{
  auto valueAtt = getParent()->findAttributeByIdAndType<PropertyPath>(fValueAttributeId);
  auto property = iCtx.findProperty(valueAtt->fValue);
  if(property && (fValue < 0 || fValue >= property->stepCount()))
    oErrors.add("%d is not in range [0, %d]", fValue, property->stepCount() - 1);
}

//------------------------------------------------------------------------
// UserSampleIndex::editView
//------------------------------------------------------------------------
void UserSampleIndex::editView(AppContext &iCtx)
{
  auto count = iCtx.getUserSamplesCount();
  if(count < 1)
  {
    Integer::editView(iCtx);
  }
  else
  {
    menuView(iCtx);
    ImGui::SameLine();
    auto editedValue = fValue;
    if(ImGui::SliderInt(fName, &editedValue, 0, count - 1))
    {
      mergeUpdate(editedValue);
    }
  }
}

//------------------------------------------------------------------------
// UserSampleIndex::findErrors
//------------------------------------------------------------------------
void UserSampleIndex::findErrors(AppContext &iCtx, UserError &oErrors) const
{
  auto count = iCtx.getUserSamplesCount();

  if(count < 1)
    oErrors.add("No user sample defined");

  if(fValue >= count)
    oErrors.add("%d is not an integer in the range [0,  %d] "
                "(%d is the number of user samples in motherboard_def.lua)",
                fValue, count - 1, count - 1);
}

//------------------------------------------------------------------------
// Values::findErrors
//------------------------------------------------------------------------
void Values::findErrors(AppContext &iCtx, UserError &oErrors) const
{
  if(fValue.empty())
    oErrors.add("The list must contain at least one entry");
  else
    PropertyPathList::findErrors(iCtx, oErrors);
}

//------------------------------------------------------------------------
// ValueTemplates::getValueAsLua
//------------------------------------------------------------------------
std::string ValueTemplates::getValueAsLua() const
{
  if(fValue.empty())
    return "{}";
  std::vector<std::string> l{};
  std::transform(fValue.begin(), fValue.end(), std::back_inserter(l), toUIText);
  return re::mock::fmt::printf("{ %s }", re::mock::stl::join_to_string(l));
}

//------------------------------------------------------------------------
// ValueTemplates::editView
//------------------------------------------------------------------------
void ValueTemplates::editView(AppContext &iCtx)
{
  menuView(iCtx);

  ImGui::SameLine();

  auto const offset = ImGui::GetCursorPosX();
  auto const itemWidth = AppContext::GetCurrent().fItemWidth;

  ImGui::BeginGroup();
  int deleteItemIdx = -1;
  for(int i = 0; i < fValue.size(); i++)
  {
    ImGui::PushID(i);
    if(ImGui::Button("-"))
      deleteItemIdx = i;
    ImGui::SameLine();

    ImGui::PushItemWidth(itemWidth - (ImGui::GetCursorPosX() - offset));

    auto editedValue = fValue[i];
    if(ImGui::InputText(re::mock::fmt::printf("%s [%d]", fName, i).c_str(), &editedValue))
    {
      update([this, i, &editedValue] {
               fValue[i] = editedValue;
               fProvided = true;
             },
             computeUpdateAttributeDescription(this, i),
             MergeKey::from(&fValue[i]));
    }

    ImGui::PopItemWidth();

    ImGui::PopID();
  }

  if(deleteItemIdx >= 0)
  {
    updateAttribute([this, deleteItemIdx] {
      fValue.erase(fValue.begin() + deleteItemIdx);
    });
  }

  ImGui::PushID(static_cast<int>(fValue.size()));

  if(ImGui::Button("+"))
  {
    updateAttribute([this] {
      fValue.resize(fValue.size() + 1);
    });
  }

  ImGui::SameLine();
  ImGui::PushItemWidth(itemWidth - (ImGui::GetCursorPosX() - offset));
  ImGui::LabelText(fName, "Click + to add");
  ImGui::PopItemWidth();

  ImGui::PopID();
  ImGui::EndGroup();

  auto valueAtt = getParent()->findAttributeByIdAndType<Value>(fValueAttributeId);

  // sanity check as the following line will have no effect unless valueAtt is processed first
  RE_EDIT_INTERNAL_ASSERT(valueAtt->fId < fId);
  fEdited |= valueAtt->isEdited();
}

//------------------------------------------------------------------------
// ValueTemplates::findErrors
//------------------------------------------------------------------------
void ValueTemplates::findErrors(AppContext &iCtx, UserError &oErrors) const
{
  if(fValue.size() > 1)
  {
    auto valueAtt = getParent()->findAttributeByIdAndType<Value>(fValueAttributeId);
    if(valueAtt->fUseSwitch)
    {
      auto property = iCtx.findProperty(valueAtt->fValueSwitch.fValue);
      if(property && property->stepCount() != fValue.size())
        oErrors.add("May contain one entry, or the same number of entries as the number of entries in values (%d)", fValue.size());
    }
    else
      oErrors.add("Only 1 value max allowed");
  }
}

//------------------------------------------------------------------------
// ReadOnly::editView
//------------------------------------------------------------------------
void ReadOnly::editView(AppContext &iCtx)
{
  auto previousValue = fValue;
  Bool::editView(iCtx);
  if(previousValue != fValue)
    onChanged(iCtx);
}

//------------------------------------------------------------------------
// ReadOnly::onChanged
//------------------------------------------------------------------------
void ReadOnly::onChanged(AppContext &iCtx)
{
  static const Property::Filter kReadOnlyValueFilter{[](const Property &p) {
    return isOneOf(p.type(),  Property::Type::kBoolean | Property::Type::kNumber | Property::Type::kString)
           && isOneOf(p.owner(), Property::Owner::kDocOwner | Property::Owner::kGUIOwner | Property::Owner::kRTOwner);
  }, "Must be a number, string, or boolean, rt_owner property (read_only is true)"};

  auto valueAtt = getParent()->findAttributeByIdAndType<Value>(fValueAttributeId);
  if(fValue)
    valueAtt->updateFilters(kReadOnlyValueFilter, kReadOnlyValueFilter);
  else
    valueAtt->updateFilters(kReadWriteValueFilter, kReadWriteValueFilter);
}

}

