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
#include "AppContext.h"
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
  virtual void hdgui2D(AppContext &iCtx, attribute_list_t &oAttributes) const {}
//  virtual Kind getKind() const = 0;

  virtual void reset() {}
  virtual void editView(AppContext &iCtx) {}
  virtual void init(AppContext &iCtx) {}
  virtual std::string toString() const;
  void clearError() { fError = kNoError; };
  virtual error_t checkForErrors(AppContext &iCtx) const { return kNoError; }
  virtual void resetEdited() { fEdited = false; }

  template<typename T, typename... ConstructorArgs>
  static std::unique_ptr<T> build(char const *iName, bool iRequired, typename T::value_t const &iDefaultValue, ConstructorArgs&& ...iArgs);

  virtual std::unique_ptr<Attribute> clone() const = 0;
  virtual bool copyFrom(Attribute const *iFromAttribute) = 0;
//  virtual bool eq(Attribute const *iAttribute) const = 0;

  friend class re::edit::Widget;

protected:
  template<typename T>
  static std::unique_ptr<Attribute> clone(T const &iAttribute);

//  template<typename T, typename Eq>
//  static bool eq(T const *iLeftAttribute, Attribute const *iRightAttribute, Eq &&eq);

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
  void hdgui2D(AppContext &iCtx, attribute_list_t &oAttributes) const override;
  void resetView(AppContext &iCtx);
  void resetView(const std::function<void()>& iOnReset) const;
  virtual std::string getValueAsLua() const = 0;
  void reset() override;

  bool copyFrom(Attribute const *iFromAttribute) override;

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
  std::string getValueAsLua() const override { return fmt::Bool::to_string(fValue); }
  void editView(AppContext &iCtx) override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Bool>(*this); }

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) { return l->fValue == r->fValue;});
//  }
};

class Integer : public SingleAttribute<int>
{
public:
  explicit Integer(char const *iName) : SingleAttribute<int>{iName} {}
  std::string getValueAsLua() const override { return std::to_string(fValue); }
  void editView(AppContext &iCtx) override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Integer>(*this); }

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) { return l->fValue == r->fValue;});
//  }
};

class String : public SingleAttribute<std::string>
{
public:
//  Kind getKind() const override { return Kind::kString; }
  explicit String(char const *iName) : SingleAttribute<std::string>{iName} {}
  std::string getValueAsLua() const override;
  void editView(AppContext &iCtx) override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<String>(*this); }

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) { return l->fValue == r->fValue;});
//  }
};

class PropertyPath : public String
{
public:
  explicit PropertyPath(char const *iName, Property::Filter iFilter = {}) : String{iName}, fFilter{std::move(iFilter)} {}
  void editView(AppContext &iCtx) override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<PropertyPath>(*this); }

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) { return l->fValue == r->fValue;});
//  }

  void editView(AppContext &iCtx,
                std::function<void()> const &iOnReset,
                std::function<void(const Property *)> const &iOnSelect,
                std::function<void(AppContext &iCtx)> const &iEditPropertyView,
                std::function<void(AppContext &iCtx)> const &iTooltipPropertyView);

  void editPropertyView(AppContext &iCtx) { editPropertyView(iCtx, fValue) ;}
  void tooltipPropertyView(AppContext &iCtx) { tooltipPropertyView(iCtx, fValue); }

  void menuView(AppContext &iCtx,
                std::function<void()> const &iOnReset,
                std::function<void(AppContext &iCtx)> const &iEditPropertyView = {});

  void menuView(AppContext &iCtx,
                std::string const &iPropertyPath,
                std::function<void()> const &iOnReset,
                std::function<void(AppContext &iCtx)> const &iEditPropertyView = {});

  static void editPropertyView(AppContext &iCtx, std::string const &iPropertyPath);

  static void tooltipPropertyView(AppContext &iCtx, std::string const &iPropertyPath);

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

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) { return l->fValue == r->fValue;});
//  }
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
  void editView(AppContext &iCtx) override;
  void editStaticListView(AppContext &iCtx,
                          Property::Filter const &iFilter,
                          std::function<void(int iIndex, const Property *)> const &iOnSelect) const;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<PropertyPathList>(*this); }

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) { return l->fValue == r->fValue;});
//  }

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
                std::function<void(int iIndex)>             const &iOnDelete);

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<DiscretePropertyValueList>(*this); }

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) { return l->fValue == r->fValue;});
//  }
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
  void hdgui2D(AppContext &iCtx, attribute_list_t &oAttributes) const override;
  void editView(AppContext &iCtx) override;

  void editValueView(AppContext &iCtx);
  void tooltipView(AppContext &iCtx);

  error_t checkForErrors(AppContext &iCtx) const override;
  void resetEdited() override;
//  std::string getPropertyInfo(AppContext &iCtx);

  std::string toString() const override;

  void reset() override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Value>(*this); }

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) {
//      return l->fUseSwitch == r->fUseSwitch &&
//             l->fValue.eq(&r->fValue) &&
//             l->fValueSwitch.eq(&r->fValueSwitch) &&
//             l->fValues.eq(&r->fValues);
//    });
//  }

  bool copyFrom(Attribute const *iAttribute) override;

protected:
  std::string const &findActualPropertyPath(AppContext &iCtx) const;

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
  void hdgui2D(AppContext &iCtx, attribute_list_t &oAttributes) const override;
  void editView(AppContext &iCtx) override;

  error_t checkForErrors(AppContext &iCtx) const override;

  void resetEdited() override;

  void reset() override;

  std::string toString() const override;

  bool isHidden(AppContext const &iCtx) const;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Visibility>(*this); }

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) {
//      return l->fSwitch.eq(&r->fSwitch) && l->fValues.eq(&r->fValues);
//    });
//  }

  bool copyFrom(Attribute const *iAttribute) override;

public:
  PropertyPath fSwitch;
  DiscretePropertyValueList fValues{"visibility_values"};
};

class StaticStringList : public String
{
public:
  explicit StaticStringList(char const *iName, std::vector<std::string> const &iSelectionList) : String{iName}, fSelectionList(iSelectionList) {}
//  Kind getKind() const override { return Kind::kStaticStringList; }
  void editView(AppContext &iCtx) override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<StaticStringList>(*this); }

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) { return l->fValue == r->fValue;});
//  }

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
  void editView(AppContext &iCtx) override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<ObjectPath>(*this); }

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) { return l->fValue == r->fValue;});
//  }

public:
  re::mock::JboxObjectType fObjectType;
  Object::Filter fFilter;
};

class Socket : public ObjectPath
{
public:
  explicit Socket(re::mock::JboxObjectType iSocketType, Object::Filter iFilter = {}) : ObjectPath{"socket", iSocketType, std::move(iFilter)} {}
  void editView(AppContext &iCtx) override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Socket>(*this); }

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) { return l->fValue == r->fValue;});
//  }
};

class Color3 : public SingleAttribute<JboxColor3>
{
public:
  explicit Color3(char const *iName) : SingleAttribute<JboxColor3>{iName} {}
  std::string getValueAsLua() const override;
  void editView(AppContext &iCtx) override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Color3>(*this); }

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) { return l->fValue == r->fValue;});
//  }
};

class Values : public PropertyPathList
{
public:
  explicit Values(char const *iName, Property::Filter iFilter = {}) :
    PropertyPathList{iName, std::move(iFilter)}
  {}

  error_t checkForErrors(AppContext &iCtx) const override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Values>(*this); }

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) { return l->fValue == r->fValue;});
//  }
};

class ValueTemplates : public SingleAttribute<std::vector<std::string>>
{
public:
  ValueTemplates(char const *iName, int iValueAttributeId) :
    SingleAttribute<std::vector<std::string>>{iName}, fValueAttributeId{iValueAttributeId} {}
  std::string getValueAsLua() const override;
  void editView(AppContext &iCtx) override;

  error_t checkForErrors(AppContext &iCtx) const override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<ValueTemplates>(*this); }

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) { return l->fValue == r->fValue;});
//  }

protected:
  int fValueAttributeId;
};

class ReadOnly : public Bool
{
public:
  explicit ReadOnly(char const *iName, int iValueAttributeId) :
    Bool{iName}, fValueAttributeId{iValueAttributeId} {}
  void editView(AppContext &iCtx) override;

  void init(AppContext &iCtx) override { onChanged(iCtx); }

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<ReadOnly>(*this); }

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) { return l->fValue == r->fValue;});
//  }

protected:
  void onChanged(AppContext &iCtx);

protected:
  int fValueAttributeId;
};

class Index : public Integer
{
public:
  Index(char const *iName, int iValueAttributeId) :
    Integer{iName},
    fValueAttributeId{iValueAttributeId} {}

  void editView(AppContext &iCtx) override;

  error_t checkForErrors(AppContext &iCtx) const override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Index>(*this); }

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) { return l->fValue == r->fValue;});
//  }

protected:
  int fValueAttributeId;
};

class UserSampleIndex : public Integer
{
public:
  explicit UserSampleIndex(char const *iName) : Integer{iName} {}

  void editView(AppContext &iCtx) override;

  error_t checkForErrors(AppContext &iCtx) const override;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<UserSampleIndex>(*this); }

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) { return l->fValue == r->fValue;});
//  }
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
void SingleAttribute<T>::resetView(AppContext &iCtx)
{
  if(ReGui::ResetButton())
  {
    iCtx.addUndoAttributeReset(this);
    reset();
  }
  static bool first_time = true;
  if(first_time)
  {
    first_time = false;
    auto size = ImGui::GetItemRectSize();
    RE_EDIT_LOG_INFO("reset %fx%f", size.x, size.y);
  }
}

//------------------------------------------------------------------------
// SingleAttribute<T>::resetView
//------------------------------------------------------------------------
template<typename T>
void SingleAttribute<T>::resetView(const std::function<void()>& iOnReset) const
{
  if(ReGui::ResetButton())
    iOnReset();
}

//------------------------------------------------------------------------
// SingleAttribute<T>::hdgui2D
//------------------------------------------------------------------------
template<typename T>
void SingleAttribute<T>::hdgui2D(AppContext &iCtx, attribute_list_t &oAttributes) const
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
  return re::mock::fmt::printf("%s={%s,%s}", fName, getValueAsLua(), fmt::Bool::to_chars(fProvided));
}

//------------------------------------------------------------------------
// SingleAttribute<T>::copyFrom
//------------------------------------------------------------------------
template<typename T>
bool SingleAttribute<T>::copyFrom(Attribute const *iFromAttribute)
{
  auto fromAttribute = dynamic_cast<SingleAttribute<T> const *>(iFromAttribute);
  if(fromAttribute && strcmp(fName, iFromAttribute->fName) == 0)
  {
    fValue = fromAttribute->fValue;
    fProvided = fromAttribute->fProvided;
    fEdited = true;
    return true;
  }
  else
    return false;
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

////------------------------------------------------------------------------
//// Attribute::eq
////------------------------------------------------------------------------
//template<typename T, typename Eq>
//bool Attribute::eq(T const *iLeftAttribute, Attribute const *iRightAttribute, Eq &&eq)
//{
//  auto r = dynamic_cast<T const *>(iRightAttribute);
//  return r && eq(iLeftAttribute, r);
//}


} // namespace widget

}

#endif //RE_EDIT_WIDGET_ATTRIBUTE_H