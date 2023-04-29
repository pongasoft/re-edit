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

//! Enabling binary operators for re::mock::JboxPropertyType
template<>
struct enable_bitmask_operators<re::mock::JboxPropertyType>{
  static const bool enable=true;
};

//! Enabling binary operators for re::mock::DeviceType
template<>
struct enable_bitmask_operators<re::mock::DeviceType>{
  static const bool enable=true;
};

//! Enabling binary operators for re::mock::PropertyOwner
template<>
struct enable_bitmask_operators<re::mock::PropertyOwner>{
  static const bool enable=true;
};

namespace re::edit {

struct Object
{
  using Type = re::mock::JboxObjectType;

  struct Filter
  {
    using type = std::function<bool(Object const &iObject)>;

    Filter() = default;
    Filter(type iAction, std::string iDescription) : fAction{std::move(iAction)}, fDescription{std::move(iDescription)} {}
    explicit operator bool() const { return fAction.operator bool(); }
    bool operator()(Object const &o) const { return fAction(o); }
    type fAction{};
    std::string fDescription{};
  };

  constexpr Type type() const { return fInfo.fType; };
  constexpr TJBox_ObjectRef ref() const { return fInfo.fObjectRef; };
  constexpr std::string const &path() const { return fInfo.fObjectPath; };

  re::mock::JboxObjectInfo fInfo{};
};

struct Property
{
  using Comparator = std::function<bool(Property const *iLeft, Property const *iRight)>;
  using Type = re::mock::JboxPropertyType;
  using Owner = re::mock::PropertyOwner;

  struct Filter
  {
    using type = std::function<bool(Property const &iProperty)>;

    Filter() = default;
    Filter(type iAction, std::string iDescription) : fAction{std::move(iAction)}, fDescription{std::move(iDescription)} {}
    explicit operator bool() const { return fAction.operator bool(); }
    bool operator()(Property const &p) const { return fAction(p); }
    type fAction{};
    std::string fDescription{};
  };

  constexpr TJBox_PropertyRef const &ref() const { return fInfo.fPropertyRef; };
  constexpr Object const &parent() const { return fParent; };
  constexpr TJBox_ObjectRef parentRef() const { return fInfo.fPropertyRef.fObject; };
  constexpr std::string const &path() const { return fInfo.fPropertyPath; };
  constexpr Type type() const { return fInfo.fValueType; };
  constexpr int stepCount() const { return fInfo.fStepCount; };
  constexpr Owner owner() const { return fInfo.fOwner; };
  constexpr TJBox_Tag tag() const { return fInfo.fTag; };
  constexpr re::mock::lua::EPersistence persistence() const { return fInfo.fPersistence; };

  constexpr bool isDiscrete() const { return fInfo.fStepCount > 0; }
  constexpr bool isDiscreteNumber() const { return isDiscrete() && type() == mock::JboxPropertyType::kNumber; }

  re::mock::JboxPropertyInfo fInfo{};
  Object fParent{};
};

static constexpr auto kDocGuiOwnerFilter = [](const Property &p) {
  return isOneOf(p.owner(), Property::Owner::kDocOwner | Property::Owner::kGUIOwner);
};

static constexpr auto kByPathComparator = [](Property const *iLeft, Property const *iRight) {
  return iLeft->path() < iRight->path();
};

static constexpr auto kByTagComparator = [](Property const *iLeft, Property const *iRight) {
  return iLeft->tag() < iRight->tag();
};


}

#endif //RE_EDIT_PROPERTY_H