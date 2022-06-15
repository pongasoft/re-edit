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
    kPropertyPath
  };

public:
  virtual ~Attribute() = default;
  void device2D(attribute_list_t &oAttributes) const;
  virtual Kind getKind() const = 0;
  virtual std::string getValueAsLua() const = 0;

  virtual void edit(EditContext &iCtx) {}

  template<typename T>
  static std::unique_ptr<Attribute> build(std::string iName, std::optional<typename T::value_t> iDefaultValue = std::nullopt);

public:
  std::string fName{};
  bool fRequired{};
  bool fProvided{};
};

namespace attribute {

class Bool : public Attribute
{
public:
  using value_t = bool;

public:
  Kind getKind() const override { return Kind::kBool; }
  std::string getValueAsLua() const override { return fValue ? "true" : "false"; }
  void edit(EditContext &iCtx) override;

public:
  value_t fValue{};
};

class String : public Attribute
{
public:
  using value_t = std::string;

public:
  Kind getKind() const override { return Kind::kString; }
  std::string getValueAsLua() const override;
  void edit(EditContext &iCtx) override;

public:
  value_t fValue{};
};

class PropertyPath : public String
{
public:
  Kind getKind() const override { return Kind::kPropertyPath; }
  void edit(EditContext &iCtx) override;
};


}


//------------------------------------------------------------------------
// Attribute::build
//------------------------------------------------------------------------
template<typename T>
std::unique_ptr<Attribute> Attribute::build(std::string iName, std::optional<typename T::value_t> iDefaultValue)
{
  auto attribute = std::make_unique<T>();
  attribute->fName = std::move(iName);
  if(iDefaultValue)
    attribute->fValue = *iDefaultValue;
  return attribute;
}

}

class Widget
{
public:
  explicit Widget(Panel iPanel) : fPanel{iPanel} {}
  Widget(Panel iPanel, ImVec2 const &iPosition, std::shared_ptr<Texture> iTexture);

  void edit(EditContext &iCtx);

  static std::unique_ptr<Widget> analog_knob(Panel iPanel);

protected:
  void addAttribute(std::unique_ptr<widget::Attribute> iAttribute) { fAttributes.emplace_back(std::move(iAttribute)); }

private:
  Panel fPanel{};
  ImVec2 fPosition{};
  std::shared_ptr<Texture> fTexture{};
  std::vector<std::unique_ptr<widget::Attribute>> fAttributes{};
};

}

#endif //RE_EDIT_WIDGET_H