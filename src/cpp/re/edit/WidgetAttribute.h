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
#include "Views.h"
#include "ReGui.h"
#include "Color.h"

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
  using error_t = char const *;
  inline static error_t kNoError{};

public:
  explicit Attribute(char const *iName) : fName{iName} {}
  virtual ~Attribute() = default;
  virtual void hdgui2D(EditContext &iCtx, attribute_list_t &oAttributes) const {}
//  virtual Kind getKind() const = 0;

  virtual void reset() {}
  virtual void editView(EditContext &iCtx) {}
  virtual void init(EditContext &iCtx) {}
  virtual std::string toString() const;
  void clearError() { fError = kNoError; };
  virtual error_t checkForErrors(EditContext &iCtx) const { return kNoError; }
  virtual void resetEdited() { fEdited = false; }

  template<typename T, typename... ConstructorArgs>
  static std::unique_ptr<T> build(char const *iName, bool iRequired, typename T::value_t const &iDefaultValue, ConstructorArgs&& ...iArgs);

  virtual std::unique_ptr<Attribute> clone() const = 0;

  friend class re::edit::Widget;

protected:
  template<typename T>
  static std::unique_ptr<Attribute> clone(T const &iAttribute);

  void init(int id) { fId = id; }

public:
  int fId{-1};
  char const *fName;
  bool fRequired{};
  bool fEdited{};
  error_t fError{};
};

namespace attribute {

template<typename T>
class SingleAttribute : public Attribute
{
public:
  using value_t = T;

public:
  explicit SingleAttribute(char const *iName) : Attribute{iName} {}
  void hdgui2D(EditContext &iCtx, attribute_list_t &oAttributes) const override;
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

class CompositeAttribute : public Attribute
{
public:
  explicit CompositeAttribute(char const *iName) : Attribute{iName} {}
  void resetEdited() override = 0;
};

class Bool : public SingleAttribute<bool>
{
public:
  explicit Bool(char const *iName) : SingleAttribute<bool>{iName} {}
  std::string getValueAsLua() const override { return fValue ? "true" : "false"; }
  void editView(EditContext &iCtx) override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Bool>(*this); }

};

class Integer : public SingleAttribute<int>
{
public:
  explicit Integer(char const *iName) : SingleAttribute<int>{iName} {}
  std::string getValueAsLua() const override { return std::to_string(fValue); }
  void editView(EditContext &iCtx) override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Integer>(*this); }
};

class String : public SingleAttribute<std::string>
{
public:
//  Kind getKind() const override { return Kind::kString; }
  explicit String(char const *iName) : SingleAttribute<std::string>{iName} {}
  std::string getValueAsLua() const override;
  void editView(EditContext &iCtx) override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<String>(*this); }
};

class PropertyPath : public String
{
public:
  explicit PropertyPath(char const *iName, Property::Filter iFilter = {}) : String{iName}, fFilter{std::move(iFilter)} {}
  void editView(EditContext &iCtx) override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<PropertyPath>(*this); }

  void editView(EditContext &iCtx,
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

public:
  Property::Filter fFilter;
};

class UIText : public String
{
public:
//  Kind getKind() const override { return Kind::kUIText; }
  explicit UIText(char const *iName) : String{iName} {}
  std::string getValueAsLua() const override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<UIText>(*this); }
};

class PropertyPathList : public SingleAttribute<std::vector<std::string>>
{
public:
//  Kind getKind() const override { return Kind::kPropertyPathList; }
  explicit PropertyPathList(char const *iName, Property::Filter iFilter = {}) :
    SingleAttribute<std::vector<std::string>>{iName},
    fFilter{std::move(iFilter)}
    {}
  std::string getValueAsLua() const override;
  void editView(EditContext &iCtx) override;
  void editStaticListView(EditContext &iCtx,
                          Property::Filter const &iFilter,
                          std::function<void(int iIndex, const Property *)> const &iOnSelect) const;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<PropertyPathList>(*this); }

public:
  Property::Filter fFilter;
  std::optional<views::StringListEdit> fStringListEditView{};
};

class DiscretePropertyValueList : public SingleAttribute<std::vector<int>>
{
public:
  explicit DiscretePropertyValueList(char const *iName) : SingleAttribute<std::vector<int>>{iName} {}
  std::string getValueAsLua() const override;

  bool contains(int iValue) const;

  void editView(int iMin,
                int iMax,
                std::function<void()>                       const &iOnAdd,
                std::function<void(int iIndex, int iValue)> const &iOnUpdate,
                std::function<void(int iIndex)>             const &iOnDelete) const;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<DiscretePropertyValueList>(*this); }
};

class Value : public CompositeAttribute
{
public:
  Value(Property::Filter iValueFilter, Property::Filter iValueSwitchFilter) :
    CompositeAttribute("value"),
    fValue{"value", std::move(iValueFilter)},
    fValueSwitch{"value_switch", std::move(iValueSwitchFilter)}
  {
    fRequired = true;
  }
  void hdgui2D(EditContext &iCtx, attribute_list_t &oAttributes) const override;
  void editView(EditContext &iCtx) override;

  void editValueView(EditContext &iCtx);
  void tooltipView(EditContext &iCtx);

  error_t checkForErrors(EditContext &iCtx) const override;
  void resetEdited() override;
//  std::string getPropertyInfo(EditContext &iCtx);

  std::string toString() const override;

  void reset() override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Value>(*this); }

protected:
  std::string const &findActualPropertyPath(EditContext &iCtx) const;

public:
  bool fUseSwitch{};
  PropertyPath fValue;
  PropertyPath fValueSwitch;
  PropertyPathList fValues{"values"};
};

class Visibility : public CompositeAttribute
{
public:
  Visibility();
  void hdgui2D(EditContext &iCtx, attribute_list_t &oAttributes) const override;
  void editView(EditContext &iCtx) override;

  error_t checkForErrors(EditContext &iCtx) const override;

  void resetEdited() override;

  void reset() override;

  std::string toString() const override;

  bool isHidden(DrawContext const &iCtx) const;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Visibility>(*this); }

public:
  PropertyPath fSwitch;
  DiscretePropertyValueList fValues{"visibility_values"};
};

class StaticStringList : public String
{
public:
  explicit StaticStringList(char const *iName, std::vector<std::string> const &iSelectionList) : String{iName}, fSelectionList(iSelectionList) {}
//  Kind getKind() const override { return Kind::kStaticStringList; }
  void editView(EditContext &iCtx) override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<StaticStringList>(*this); }

public:
  std::vector<std::string> const &fSelectionList;
};

class ObjectPath : public String
{
public:
  explicit ObjectPath(char const *iName, re::mock::JboxObjectType iObjectType, Object::Filter iFilter = {}) :
    String{iName},
    fObjectType{iObjectType},
    fFilter{std::move(iFilter)} {}
  void editView(EditContext &iCtx) override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<ObjectPath>(*this); }

public:
  re::mock::JboxObjectType fObjectType;
  Object::Filter fFilter;
};

class Socket : public ObjectPath
{
public:
  explicit Socket(re::mock::JboxObjectType iSocketType, Object::Filter iFilter = {}) : ObjectPath{"socket", iSocketType, std::move(iFilter)} {}
  void editView(EditContext &iCtx) override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Socket>(*this); }
};

class Color3 : public SingleAttribute<JboxColor3>
{
public:
  explicit Color3(char const *iName) : SingleAttribute<JboxColor3>{iName} {}
  std::string getValueAsLua() const override;
  void editView(EditContext &iCtx) override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Color3>(*this); }
};

class Values : public PropertyPathList
{
public:
  explicit Values(char const *iName, Property::Filter iFilter = {}) :
    PropertyPathList{iName, std::move(iFilter)}
  {}

  error_t checkForErrors(EditContext &iCtx) const override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Values>(*this); }
};

class ValueTemplates : public SingleAttribute<std::vector<std::string>>
{
public:
  ValueTemplates(char const *iName, int iValueAttributeId) :
    SingleAttribute<std::vector<std::string>>{iName}, fValueAttributeId{iValueAttributeId} {}
  std::string getValueAsLua() const override;
  void editView(EditContext &iCtx) override;

  error_t checkForErrors(EditContext &iCtx) const override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<ValueTemplates>(*this); }

protected:
  int fValueAttributeId;
};

class ReadOnly : public Bool
{
public:
  explicit ReadOnly(char const *iName, int iValueAttributeId) :
    Bool{iName}, fValueAttributeId{iValueAttributeId} {}
  void editView(EditContext &iCtx) override;

  void init(EditContext &iCtx) override { onChanged(iCtx); }

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<ReadOnly>(*this); }

protected:
  void onChanged(EditContext &iCtx);

protected:
  int fValueAttributeId;
};

class Index : public Integer
{
public:
  Index(char const *iName, int iValueAttributeId) :
    Integer{iName},
    fValueAttributeId{iValueAttributeId} {}

  void editView(EditContext &iCtx) override;

  error_t checkForErrors(EditContext &iCtx) const override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Index>(*this); }

protected:
  int fValueAttributeId;
};

//------------------------------------------------------------------------
// SingleAttribute<T>::reset
//------------------------------------------------------------------------
template<typename T>
void SingleAttribute<T>::reset()
{
  fValue = fDefaultValue;
  fProvided = false;
  fEdited = true;
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
void SingleAttribute<T>::hdgui2D(EditContext &iCtx, attribute_list_t &oAttributes) const
{
  if(fRequired || fProvided)
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
std::unique_ptr<T> Attribute::build(char const *iName, bool iRequired, typename T::value_t const &iDefaultValue, ConstructorArgs&& ...iArgs)
{
  auto attribute = std::make_unique<T>(iName, std::forward<ConstructorArgs>(iArgs)...);
  attribute->fName = iName;
  attribute->fRequired = iRequired;
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