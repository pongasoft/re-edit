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
#include "WidgetView.h"

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
  explicit Attribute(std::string iName) : fName{std::move(iName)} {}
  virtual ~Attribute() = default;
  virtual void hdgui2D(attribute_list_t &oAttributes) const {}
//  virtual Kind getKind() const = 0;

  virtual void reset() {}
  virtual void editView(EditContext const &iCtx) {}
  void clearError() { fError = std::nullopt; };

  template<typename T, typename... ConstructorArgs>
  static std::unique_ptr<T> build(std::string const &iName, typename T::value_t const &iDefaultValue, ConstructorArgs&& ...iArgs);

public:
  std::string fName{};
  std::optional<std::string> fError{};
};

namespace attribute {

template<typename T>
class SingleAttribute : public Attribute
{
public:
  using value_t = T;

public:
  explicit SingleAttribute(std::string iName) : Attribute{std::move(iName)} {}
  void hdgui2D(attribute_list_t &oAttributes) const override;
  void resetView();
  void resetView(const std::function<void()>& iOnReset) const;
  virtual std::string getValueAsLua() const = 0;
  void reset() override;

public:
  value_t fDefaultValue{};
  value_t fValue{};
  bool fProvided{};
};

class Bool : public SingleAttribute<bool>
{
public:
//  Kind getKind() const override { return Kind::kBool; }
  explicit Bool(std::string iName) : SingleAttribute<bool>{std::move(iName)} {}
  std::string getValueAsLua() const override { return fValue ? "true" : "false"; }
  void editView(EditContext const &iCtx) override;
};

class String : public SingleAttribute<std::string>
{
public:
//  Kind getKind() const override { return Kind::kString; }
  explicit String(std::string iName) : SingleAttribute<std::string>{std::move(iName)} {}
  std::string getValueAsLua() const override;
  void editView(EditContext const &iCtx) override;
};

class PropertyPath : public String
{
public:
  explicit PropertyPath(std::string iName) : String{std::move(iName)} {}
//  Kind getKind() const override { return Kind::kPropertyPath; }
  virtual EditContext::PropertyKind getPropertyKind() const { return EditContext::PropertyKind::kAny; }
  void editView(EditContext const &iCtx) override;

  void editView(std::vector<std::string> const &iProperties,
                std::function<void()> const &iOnReset,
                std::function<void(std::string const &)> const &iOnSelect) const;
};

class UIText : public String
{
public:
//  Kind getKind() const override { return Kind::kUIText; }
  explicit UIText(std::string iName) : String{std::move(iName)} {}
  std::string getValueAsLua() const override;
};

class PropertyPathList : public SingleAttribute<std::vector<std::string>>
{
public:
//  Kind getKind() const override { return Kind::kPropertyPathList; }
  explicit PropertyPathList(std::string iName) : SingleAttribute<std::vector<std::string>>{std::move(iName)} {}
  std::string getValueAsLua() const override;
//  void editView(EditContext const &iCtx) override;
  void editStaticListView(std::vector<std::string> const &iProperties,
                          std::function<void()> const &iOnReset,
                          std::function<void(int iIndex, std::string const &)> const &iOnSelect) const;
};

class DiscretePropertyValueList : public SingleAttribute<std::vector<int>>
{
public:
  explicit DiscretePropertyValueList(std::string iName) : SingleAttribute<std::vector<int>>{std::move(iName)} {}
  std::string getValueAsLua() const override;

  void editView(int iMin,
                int iMax,
                std::function<void()>                       const &iOnAdd,
                std::function<void(int iIndex, int iValue)> const &iOnUpdate,
                std::function<void(int iIndex)>             const &iOnDelete) const;
};

class Value : public Attribute
{
public:
  Value() : Attribute("value") {}
  void hdgui2D(attribute_list_t &oAttributes) const override;
  void editView(EditContext const &iCtx) override;

  void reset() override;

public:
  bool fUseSwitch{};
  PropertyPath fValue{"value"};
  PropertyPath fValueSwitch{"value_switch"};
  PropertyPathList fValues{"values"};
};

class Visibility : public Attribute
{
public:
  Visibility() : Attribute("visibility") {}
  void hdgui2D(attribute_list_t &oAttributes) const override;
  void editView(EditContext const &iCtx) override;

  void reset() override;

public:
  PropertyPath fSwitch{"visibility_switch"};
  DiscretePropertyValueList fValues{"visibility_values"};
};


//class VisibilitySwitch : public PropertyPath
//{
//public:
////  Kind getKind() const override { return Kind::kVisibilitySwitch; }
//  EditContext::PropertyKind getPropertyKind() const override{ return EditContext::PropertyKind::kDiscrete; }
//  void editView(EditContext const &iCtx) override;
//};
//
//class VisibilityValues : public DiscretePropertyValueList
//{
//public:
////  Kind getKind() const override { return Kind::kVisibilityValues; }
//  void editView(EditContext const &iCtx) override;
//  void hdgui2D(attribute_list_t &oAttributes) const override;
//};

class StaticStringList : public String
{
public:
  explicit StaticStringList(std::string iName, std::vector<std::string> const &iSelectionList) : String{std::move(iName)}, fSelectionList(iSelectionList) {}
//  Kind getKind() const override { return Kind::kStaticStringList; }
  void editView(EditContext const &iCtx) override;

public:
  std::vector<std::string> const &fSelectionList;
};

//------------------------------------------------------------------------
// SingleAttribute<T>::reset
//------------------------------------------------------------------------
template<typename T>
void SingleAttribute<T>::reset()
{
  fValue = fDefaultValue;
  fProvided = false;
}

//------------------------------------------------------------------------
// SingleAttribute<T>::resetView
//------------------------------------------------------------------------
template<typename T>
void SingleAttribute<T>::resetView()
{
  if(ImGui::Button("X"))
    reset();
}

//------------------------------------------------------------------------
// SingleAttribute<T>::resetView
//------------------------------------------------------------------------
template<typename T>
void SingleAttribute<T>::resetView(const std::function<void()>& iOnReset) const
{
  if(ImGui::Button("X"))
    iOnReset();
}

//------------------------------------------------------------------------
// SingleAttribute<T>::hdgui2D
//------------------------------------------------------------------------
template<typename T>
void SingleAttribute<T>::hdgui2D(attribute_list_t &oAttributes) const
{
  if(fProvided)
    oAttributes.emplace_back(attribute_t{fName, getValueAsLua()});
}

} // namespace attribute

//------------------------------------------------------------------------
// Attribute::build
//------------------------------------------------------------------------
template<typename T, typename... ConstructorArgs>
std::unique_ptr<T> Attribute::build(std::string const &iName, typename T::value_t const &iDefaultValue, ConstructorArgs&& ...iArgs)
{
  auto attribute = std::make_unique<T>(iName, std::forward<ConstructorArgs>(iArgs)...);
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

  constexpr WidgetView &getView() { return fView; }
  constexpr WidgetView const &getView() const { return fView; }

  void editView(EditContext &iCtx);

  static std::unique_ptr<Widget> analog_knob(Panel iPanel);

//  template<typename T>
//  T *findAttribute(std::string const &iAttributeName) const;
//
//  template<typename T>
//  typename T::value_t *findAttributeValue(std::string const &iAttributeName) const;

//  std::string *findValueValue() const { return findAttributeValue<widget::attribute::PropertyPath>("value"); }
//  std::string *findValueSwitchValue() const { return findAttributeValue<widget::attribute::ValueSwitch>("value_switch"); }
//  std::string *findVisibilitySwitchValue() const { return findAttributeValue<widget::attribute::VisibilitySwitch>("visibility_switch"); };

protected:
  Widget *addAttribute(std::unique_ptr<widget::Attribute> iAttribute) { fAttributes.emplace_back(std::move(iAttribute)); return this; }
  Widget *value();
  Widget *show_remote_box();
  Widget *show_automation_rect();
  Widget *tooltip_position();
  Widget *tooltip_template();
  Widget *visibility();

private:
  Panel fPanel{};
  WidgetView fView{};
  std::vector<std::unique_ptr<widget::Attribute>> fAttributes{};
};

////------------------------------------------------------------------------
//// Widget::findAttribute
////------------------------------------------------------------------------
//template<typename T>
//T *Widget::findAttribute(std::string const &iAttributeName) const
//{
//  auto iter = std::find_if(fAttributes.begin(), fAttributes.end(), [&iAttributeName](auto &att) { return att->fName == iAttributeName;} );
//  if(iter == fAttributes.end())
//    return nullptr;
//  else
//    return dynamic_cast<T *>(iter->get());
//}
//
////------------------------------------------------------------------------
//// Widget::findAttributeValue
////------------------------------------------------------------------------
//template<typename T>
//typename T::value_t * Widget::findAttributeValue(std::string const &iAttributeName) const
//{
//  auto attribute = findAttribute<T>(iAttributeName);
//  if(attribute)
//    return &attribute->fValue;
//  else
//    return nullptr;
//}



}

#endif //RE_EDIT_WIDGET_H