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

#ifndef RE_EDIT_PROPERTY_H
#define RE_EDIT_PROPERTY_H

namespace re::edit {

struct Property
{
  using Filter = std::function<bool(Property const &iProperty)>;

  constexpr TJBox_PropertyRef const &ref() const { return fInfo.fPropertyRef; };
  constexpr TJBox_ObjectRef parent() const { return fInfo.fPropertyRef.fObject; };
  constexpr std::string const &path() const { return fInfo.fPropertyPath; };
  constexpr TJBox_ValueType type() const { return fInfo.fValueType; };
  constexpr int stepCount() const { return fInfo.fStepCount; };
  constexpr re::mock::PropertyOwner owner() const { return fInfo.fOwner; };
  constexpr TJBox_Tag tag() const { return fInfo.fTag; };

  constexpr bool isDiscrete() const { return fInfo.fStepCount > 0; }

  re::mock::JboxPropertyInfo fInfo{};
};


}

#endif //RE_EDIT_PROPERTY_H