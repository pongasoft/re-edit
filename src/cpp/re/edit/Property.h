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

#include <optional>
#include <set>
#include <string>
#include <functional>
#include <re/mock/Motherboard.h>
#include <JukeboxTypes.h>
#include <bitmask_operators.hpp>

#ifndef RE_EDIT_PROPERTY_H
#define RE_EDIT_PROPERTY_H

//! Enabling binary operators for re::mock::JboxObjectType
template<>
struct enable_bitmask_operators<re::mock::JboxObjectType>{
  static const bool enable=true;
};

namespace re::edit {

struct Object
{
  using Filter = std::function<bool(Object const &iObject)>;

  constexpr re::mock::JboxObjectType type() const { return fInfo.fType; };
  constexpr TJBox_ObjectRef ref() const { return fInfo.fObjectRef; };
  constexpr std::string const &path() const { return fInfo.fObjectPath; };

  re::mock::JboxObjectInfo fInfo{};
};

struct Property
{
  using Filter = std::function<bool(Property const &iProperty)>;
  using Comparator = std::function<bool(Property const *iLeft, Property const *iRight)>;

  constexpr TJBox_PropertyRef const &ref() const { return fInfo.fPropertyRef; };
  constexpr Object const &parent() const { return fParent; };
  constexpr TJBox_ObjectRef parentRef() const { return fInfo.fPropertyRef.fObject; };
  constexpr std::string const &path() const { return fInfo.fPropertyPath; };
  constexpr TJBox_ValueType type() const { return fInfo.fValueType; };
  constexpr int stepCount() const { return fInfo.fStepCount; };
  constexpr re::mock::PropertyOwner owner() const { return fInfo.fOwner; };
  constexpr TJBox_Tag tag() const { return fInfo.fTag; };
  constexpr re::mock::lua::EPersistence persistence() const { return fInfo.fPersistence; };

  constexpr bool isDiscrete() const { return fInfo.fStepCount > 0; }

  re::mock::JboxPropertyInfo fInfo{};
  Object fParent{};
};

static constexpr auto kDocGuiOwnerFilter = [](const Property &p) {
  return p.owner() == mock::PropertyOwner::kDocOwner || p.owner() == mock::PropertyOwner::kGUIOwner;
};

static constexpr auto kByPathComparator = [](Property const *iLeft, Property const *iRight) {
  return iLeft->path() < iRight->path();
};

static constexpr auto kByTagComparator = [](Property const *iLeft, Property const *iRight) {
  return iLeft->tag() < iRight->tag();
};


}

#endif //RE_EDIT_PROPERTY_H