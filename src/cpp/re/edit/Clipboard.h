/*
 * Copyright (c) 2023 pongasoft
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

#ifndef RE_EDIT_CLIPBOARD_H
#define RE_EDIT_CLIPBOARD_H

#include <memory>
#include <string>
#include <bitmask_operators.hpp>
#include "Errors.h"

namespace re::edit::clipboard {

enum class DataType : int
{
  kNone             = 0,
  kWidget           = 1 << 0,
  kWidgetAttribute  = 1 << 1,
  kWidgetList       = 1 << 2
};

class Data
{
public:
  explicit Data(std::string iDescription) : fDescription{std::move(iDescription)} {}
  virtual ~Data() = default;
  virtual clipboard::DataType getType() const = 0;
  inline std::string const &getDescription() const { return fDescription; }

private:
  std::string fDescription;
};

class NoData : public Data
{
public:
  NoData() : Data("No clipboard") {}
  DataType getType() const override { return DataType::kNone; }
};

}

template<>
struct enable_bitmask_operators<re::edit::clipboard::DataType> {
  static const bool enable = true;
};

namespace re::edit {

class Clipboard
{
public:
  inline clipboard::DataType getType() const { return fData->getType(); }
  constexpr bool isEmpty() const { return getType() == clipboard::DataType::kNone; }
  inline bool matchesType(clipboard::DataType iType) const { return (fData->getType() & iType) != clipboard::DataType::kNone; }
  clipboard::Data const *getData() const { return fData.get(); }
  void setData(std::unique_ptr<clipboard::Data> iData) { RE_EDIT_INTERNAL_ASSERT(iData != nullptr); fData = std::move(iData); }
  void reset() { fData = std::make_unique<clipboard::NoData>(); }

private:
  std::unique_ptr<clipboard::Data> fData{new clipboard::NoData{}};
};


}

#endif //RE_EDIT_CLIPBOARD_H