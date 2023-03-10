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

#ifndef RE_EDIT_NS_USER_DEFAULTS_MANAGER_H
#define RE_EDIT_NS_USER_DEFAULTS_MANAGER_H

#include "../../PreferencesManager.h"

namespace re::edit {
class NSUserDefaultsManager : public NativePreferencesManager
{
public:
  std::optional<std::string> load() const override;
  void save(std::string const &iPreferences) const override;
};
}

#endif //RE_EDIT_NS_USER_DEFAULTS_MANAGER_H
