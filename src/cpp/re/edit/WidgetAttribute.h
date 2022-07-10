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

#ifndef RE_EDIT_WIDGET_ATTRIBUTE_H
#define RE_EDIT_WIDGET_ATTRIBUTE_H

#include "Constants.h"
#include "Texture.h"
#include "EditContext.h"
#include "DrawContext.h"
#include "ReGui.h"

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
  explicit Attribute(std::string iName) : fName{std::move(iName)} {}
  virtual ~Attribute() = default;
  virtual void hdgui2D(attribute_list_t &oAttributes) const {}
//  virtual Kind getKind() const = 0;

  virtual void reset() {}
  virtual void editView(EditContext &iCtx) {}
  virtual std::string toString() const;
  void clearError() { fError = std::nullopt; };

  template<typename T, typename... ConstructorArgs>
  static std::unique_ptr<T> build(std::string const &iName, typename T::value_t const &iDefaultValue, ConstructorArgs&& ...iArgs);

  virtual std::unique_ptr<Attribute> clone() const = 0;

protected:
  template<typename T>
  static std::unique_ptr<Attribute> clone(T const &iAttribute);

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

  std::string toString() const override;

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
  void editView(EditContext &iCtx) override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Bool>(*this); }
};

class String : public SingleAttribute<std::string>
{
public:
//  Kind getKind() const override { return Kind::kString; }
  explicit String(std::string iName) : SingleAttribute<std::string>{std::move(iName)} {}
  std::string getValueAsLua() const override;
  void editView(EditContext &iCtx) override;
};

class PropertyPath : public String
{
public:
  explicit PropertyPath(std::string iName) : String{std::move(iName)} {}
  void editView(EditContext &iCtx) override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<PropertyPath>(*this); }

  void editView(EditContext &iCtx,
                Property::Filter const &iFilter,
                std::function<void()> const &iOnReset,
                std::function<void(const Property *)> const &iOnSelect,
                std::function<void(EditContext &iCtx)> const &iEditPropertyView,
                std::function<void(EditContext &iCtx)> const &iTooltipPropertyView);

  void editPropertyView(EditContext &iCtx) { editPropertyView(iCtx, fValue) ;}
  void tooltipPropertyView(EditContext &iCtx) { tooltipPropertyView(iCtx, fValue); }

  void menuView(EditContext &iCtx,
                std::function<void()> const &iOnReset,
                std::function<void(EditContext &iCtx)> const &iEditPropertyView = {});

  static void menuView(EditContext &iCtx,
                       std::string const &iPropertyPath,
                       std::function<void()> const &iOnReset,
                       std::function<void(EditContext &iCtx)> const &iEditPropertyView = {});

  static void editPropertyView(EditContext &iCtx, std::string const &iPropertyPath);

  static void tooltipPropertyView(EditContext &iCtx, std::string const &iPropertyPath);
};

class UIText : public String
{
public:
//  Kind getKind() const override { return Kind::kUIText; }
  explicit UIText(std::string iName) : String{std::move(iName)} {}
  std::string getValueAsLua() const override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<UIText>(*this); }
};

class PropertyPathList : public SingleAttribute<std::vector<std::string>>
{
public:
//  Kind getKind() const override { return Kind::kPropertyPathList; }
  explicit PropertyPathList(std::string iName) : SingleAttribute<std::vector<std::string>>{std::move(iName)} {}
  std::string getValueAsLua() const override;
//  void editView(EditContext &iCtx) override;
  void editStaticListView(EditContext &iCtx,
                          Property::Filter const &iFilter,
                          std::function<void(int iIndex, const Property *)> const &iOnSelect) const;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<PropertyPathList>(*this); }
};

class DiscretePropertyValueList : public SingleAttribute<std::vector<int>>
{
public:
  explicit DiscretePropertyValueList(std::string iName) : SingleAttribute<std::vector<int>>{std::move(iName)} {}
  std::string getValueAsLua() const override;

  bool contains(int iValue) const;

  void editView(int iMin,
                int iMax,
                std::function<void()>                       const &iOnAdd,
                std::function<void(int iIndex, int iValue)> const &iOnUpdate,
                std::function<void(int iIndex)>             const &iOnDelete) const;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<DiscretePropertyValueList>(*this); }
};

class Value : public Attribute
{
public:
  Value(Property::Filter iValueFilter, Property::Filter iValueSwitchFilter) :
    Attribute("value"),
    fValueFilter(std::move(iValueFilter)),
    fValueSwitchFilter(std::move(iValueSwitchFilter))
    {}
  void hdgui2D(attribute_list_t &oAttributes) const override;
  void editView(EditContext &iCtx) override;

  void editValueView(EditContext &iCtx);
  void tooltipView(EditContext &iCtx);
//  std::string getPropertyInfo(EditContext &iCtx);

  std::string toString() const override;

  void reset() override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Value>(*this); }

protected:
  std::string const &findActualPropertyPath(EditContext &iCtx) const;

public:
  bool fUseSwitch{};
  PropertyPath fValue{"value"};
  Property::Filter fValueFilter{};
  PropertyPath fValueSwitch{"value_switch"};
  Property::Filter fValueSwitchFilter{};
  PropertyPathList fValues{"values"};
};

class Visibility : public Attribute
{
public:
  Visibility() : Attribute("visibility") {}
  void hdgui2D(attribute_list_t &oAttributes) const override;
  void editView(EditContext &iCtx) override;

  void reset() override;

  std::string toString() const override;

  bool isHidden(DrawContext const &iCtx) const;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Visibility>(*this); }

public:
  PropertyPath fSwitch{"visibility_switch"};
  DiscretePropertyValueList fValues{"visibility_values"};
};

class StaticStringList : public String
{
public:
  explicit StaticStringList(std::string iName, std::vector<std::string> const &iSelectionList) : String{std::move(iName)}, fSelectionList(iSelectionList) {}
//  Kind getKind() const override { return Kind::kStaticStringList; }
  void editView(EditContext &iCtx) override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<StaticStringList>(*this); }

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

//------------------------------------------------------------------------
// SingleAttribute<T>::toString
//------------------------------------------------------------------------
template<typename T>
std::string SingleAttribute<T>::toString() const
{
  return re::mock::fmt::printf("%s={%s,%s}", fName, getValueAsLua(), fProvided ? "true" : "false");
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

//------------------------------------------------------------------------
// Attribute::clone
//------------------------------------------------------------------------
template<typename T>
std::unique_ptr<Attribute> Attribute::clone(T const &iAttribute)
{
  return std::make_unique<T>(iAttribute);
}

} // namespace widget

}

#endif //RE_EDIT_WIDGET_ATTRIBUTE_H