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
    kPropertyPathList

  };

public:
  virtual ~Attribute() = default;
  void device2D(attribute_list_t &oAttributes) const;
  virtual Kind getKind() const = 0;
  virtual std::string getValueAsLua() const = 0;

  virtual void editView(EditContext &iCtx) {}

  template<typename T>
  static std::unique_ptr<T> build(std::string const &iName, typename T::value_t const &iDefaultValue);

public:
  std::string fName{};
  bool fRequired{};
  bool fProvided{};
};

namespace attribute {

template<typename V>
class VAttribute : public Attribute
{
public:
  using value_t = V;

public:
  void resetView(EditContext &iCtx);
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
  void editView(EditContext &iCtx) override;
};

class String : public VAttribute<std::string>
{
public:
  Kind getKind() const override { return Kind::kString; }
  std::string getValueAsLua() const override;
  void editView(EditContext &iCtx) override;
};

class PropertyPath : public String
{
public:
  Kind getKind() const override { return Kind::kPropertyPath; }
  void editView(EditContext &iCtx) override;
};

class PropertyPathList : public VAttribute<std::vector<std::string>>
{
  Kind getKind() const override { return Kind::kPropertyPathList; }
  std::string getValueAsLua() const override;
  void editView(EditContext &iCtx) override;
};

class StaticStringList : public String
{
  Kind getKind() const override { return Kind::kStaticStringList; }
  void editView(EditContext &iCtx) override;

public:
  std::vector<std::string> fSelectionList{};
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
void VAttribute<V>::resetView(EditContext &iCtx)
{
  if(ImGui::Button("X"))
    reset();
}


} // namespace attribute

//------------------------------------------------------------------------
// Attribute::build
//------------------------------------------------------------------------
template<typename T>
std::unique_ptr<T> Attribute::build(std::string const &iName, typename T::value_t const &iDefaultValue)
{
  auto attribute = std::make_unique<T>();
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

protected:
  Widget *addAttribute(std::unique_ptr<widget::Attribute> iAttribute) { fAttributes.emplace_back(std::move(iAttribute)); return this; }
  Widget *value();
  Widget *value_switch();
  Widget *values();
  Widget *show_remote_box();
  Widget *show_automation_rect();
  Widget *tooltip_position();

private:
  Panel fPanel{};
  ImVec2 fPosition{};
  std::shared_ptr<Texture> fTexture{};
  std::vector<std::unique_ptr<widget::Attribute>> fAttributes{};
};


}

#endif //RE_EDIT_WIDGET_H