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

#ifndef RE_EDIT_WIDGET_H
#define RE_EDIT_WIDGET_H

#include "Constants.h"
#include "Texture.h"
#include "EditContext.h"

#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace re::edit {

struct attribute_t
{
  std::string fName;
  std::string fValue;
};

using attribute_list_t = std::vector<attribute_t>;

class Widget;

namespace widget {

class Attribute
{
public:
  enum class Kind
  {
    kBool,
    kString,
    kStaticStringList,
    kPropertyPath,
    kValue,
    kValueSwitch,
    kValues,
    kPropertyPathList,
    kUIText,
//    kDiscretePropertyValueList,
    kVisibilitySwitch,
    kVisibilityValues
  };

public:
  virtual ~Attribute() = default;
  virtual void hdgui2D(Widget const &iWidget, attribute_list_t &oAttributes) const;
  virtual Kind getKind() const = 0;
  virtual std::string getValueAsLua() const = 0;

  virtual void editView(Widget &iWidget, EditContext const &iCtx) {}

  template<typename T, typename... ConstructorArgs>
  static std::unique_ptr<T> build(std::string const &iName, typename T::value_t const &iDefaultValue, ConstructorArgs&& ...iArgs);

public:
  std::string fName{};
  bool fRequired{};
  bool fProvided{};
  std::optional<std::string> fError{};
};

namespace attribute {

template<typename V>
class VAttribute : public Attribute
{
public:
  using value_t = V;

public:
  virtual bool resetView(Widget &iWidget, EditContext const &iCtx);
  void reset();

public:
  value_t fDefaultValue{};
  value_t fValue{};
};

class Bool : public VAttribute<bool>
{
public:
  Kind getKind() const override { return Kind::kBool; }
  std::string getValueAsLua() const override { return fValue ? "true" : "false"; }
  void editView(Widget &iWidget, EditContext const &iCtx) override;
};

class String : public VAttribute<std::string>
{
public:
  Kind getKind() const override { return Kind::kString; }
  std::string getValueAsLua() const override;
  void editView(Widget &iWidget, EditContext const &iCtx) override;
};

class PropertyPath : public String
{
public:
  Kind getKind() const override { return Kind::kPropertyPath; }
  virtual EditContext::PropertyKind getPropertyKind() const { return EditContext::PropertyKind::kAny; }
  void editView(Widget &iWidget, EditContext const &iCtx) override;
};

class UIText : public String
{
public:
  Kind getKind() const override { return Kind::kUIText; }
  std::string getValueAsLua() const override;
};

class PropertyPathList : public VAttribute<std::vector<std::string>>
{
public:
  Kind getKind() const override { return Kind::kPropertyPathList; }
  std::string getValueAsLua() const override;
  void editView(Widget &iWidget, EditContext const &iCtx) override;
};

class ValueSwitch : public PropertyPath
{
public:
  EditContext::PropertyKind getPropertyKind() const override { return EditContext::PropertyKind::kDiscrete; }
  Kind getKind() const override { return Kind::kValueSwitch; }

  void editView(Widget &iWidget, EditContext const &iCtx) override;

  bool resetView(Widget &iWidget, EditContext const &iCtx) override;
};

class Values : public PropertyPathList
{
public:
  Kind getKind() const override { return Kind::kValues; }
  void editView(Widget &iWidget, EditContext const &iCtx) override;
  void hdgui2D(Widget const &iWidget, attribute_list_t &oAttributes) const override;
};

class DiscretePropertyValueList : public VAttribute<std::vector<int>>
{
public:
  std::string getValueAsLua() const override;
};

class VisibilitySwitch : public PropertyPath
{
public:
  Kind getKind() const override { return Kind::kVisibilitySwitch; }
  EditContext::PropertyKind getPropertyKind() const override{ return EditContext::PropertyKind::kDiscrete; }
  void editView(Widget &iWidget, EditContext const &iCtx) override;
};

class VisibilityValues : public DiscretePropertyValueList
{
public:
  Kind getKind() const override { return Kind::kVisibilityValues; }
  void editView(Widget &iWidget, EditContext const &iCtx) override;
  void hdgui2D(Widget const &iWidget, attribute_list_t &oAttributes) const override;
};

class StaticStringList : public String
{
public:
  explicit StaticStringList(std::vector<std::string> const &iSelectionList) : fSelectionList(iSelectionList) {}
  Kind getKind() const override { return Kind::kStaticStringList; }
  void editView(Widget &iWidget, EditContext const &iCtx) override;

public:
  std::vector<std::string> const &fSelectionList;
};

//------------------------------------------------------------------------
// VAttribute<V>::reset
//------------------------------------------------------------------------
template<typename V>
void VAttribute<V>::reset()
{
  fValue = fDefaultValue;
  fProvided = false;
}

//------------------------------------------------------------------------
// VAttribute<V>::resetView
//------------------------------------------------------------------------
template<typename V>
bool VAttribute<V>::resetView(Widget &iWidget, EditContext const &iCtx)
{
  if(ImGui::Button("X"))
  {
    reset();
    return true;
  }
  return false;
}


} // namespace attribute

//------------------------------------------------------------------------
// Attribute::build
//------------------------------------------------------------------------
template<typename T, typename... ConstructorArgs>
std::unique_ptr<T> Attribute::build(std::string const &iName, typename T::value_t const &iDefaultValue, ConstructorArgs&& ...iArgs)
{
  auto attribute = std::make_unique<T>(std::forward<ConstructorArgs>(iArgs)...);
  attribute->fName = iName;
  attribute->fDefaultValue = iDefaultValue;
  attribute->fValue = iDefaultValue;
  return attribute;
}

} // namespace widget

class Widget
{
public:
  explicit Widget(Panel iPanel) : fPanel{iPanel} {}
  Widget(Panel iPanel, ImVec2 const &iPosition, std::shared_ptr<Texture> iTexture);

  void editView(EditContext &iCtx);

  static std::unique_ptr<Widget> analog_knob(Panel iPanel);

  template<typename T>
  T *findAttribute(std::string const &iAttributeName) const;

  template<typename T>
  typename T::value_t *findAttributeValue(std::string const &iAttributeName) const;

  std::string *findValueValue() const { return findAttributeValue<widget::attribute::PropertyPath>("value"); }
  std::string *findValueSwitchValue() const { return findAttributeValue<widget::attribute::ValueSwitch>("value_switch"); }
  std::string *findVisibilitySwitchValue() const { return findAttributeValue<widget::attribute::VisibilitySwitch>("visibility_switch"); };

protected:
  Widget *addAttribute(std::unique_ptr<widget::Attribute> iAttribute) { fAttributes.emplace_back(std::move(iAttribute)); return this; }
  Widget *value();
  Widget *value_switch();
  Widget *values();
  Widget *show_remote_box();
  Widget *show_automation_rect();
  Widget *tooltip_position();
  Widget *tooltip_template();
  Widget *visibility_switch();
  Widget *visibility_values();

private:
  Panel fPanel{};
  ImVec2 fPosition{};
  std::shared_ptr<Texture> fTexture{};
  std::vector<std::unique_ptr<widget::Attribute>> fAttributes{};
};

//------------------------------------------------------------------------
// Widget::findAttribute
//------------------------------------------------------------------------
template<typename T>
T *Widget::findAttribute(std::string const &iAttributeName) const
{
  auto iter = std::find_if(fAttributes.begin(), fAttributes.end(), [&iAttributeName](auto &att) { return att->fName == iAttributeName;} );
  if(iter == fAttributes.end())
    return nullptr;
  else
    return dynamic_cast<T *>(iter->get());
}

//------------------------------------------------------------------------
// Widget::findAttributeValue
//------------------------------------------------------------------------
template<typename T>
typename T::value_t * Widget::findAttributeValue(std::string const &iAttributeName) const
{
  auto attribute = findAttribute<T>(iAttributeName);
  if(attribute)
    return &attribute->fValue;
  else
    return nullptr;
}



}

#endif //RE_EDIT_WIDGET_H