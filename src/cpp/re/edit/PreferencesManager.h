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

#ifndef RE_EDIT_USER_PREFERENCES_H
#define RE_EDIT_USER_PREFERENCES_H

#include <imgui.h>
#include <optional>
#include <string>
#include "Config.h"
#include "ReGui.h"

namespace re::edit {

class NativePreferencesManager
{
public:
  virtual ~NativePreferencesManager() = default;
  virtual std::optional<std::string> load() const = 0;
  virtual void save(std::string const &iPreferences) const = 0;
};

class UserPreferences
{
public:
  ImU32 fWidgetBorderColor{ReGui::GetColorU32(ImVec4{0,1,0,1})};

  ImU32 fWidgetErrorColor{ReGui::GetColorU32(ImVec4{1,0,0,0.5})};

  ImU32 fSelectedWidgetColor{ReGui::GetColorU32(ImVec4{1,1,0,1})};

  ImU32 fWidgetNoGraphicsColor{ReGui::GetColorU32(ImVec4{0.5,0.5,0.5,1})};
  ImU32 fWidgetNoGraphicsXRayColor{ReGui::GetColorU32(ImVec4{0.5,0.5,0.5,0.4})};
};

class PreferencesManager
{
public:
  static config::Global load(NativePreferencesManager const *iPreferencesManager);
  static void save(NativePreferencesManager const *iPreferencesManager, config::Global const &iConfig);
  static std::string getAsLua(config::Global const &iConfig);
};

}

#endif //RE_EDIT_USER_PREFERENCES_H