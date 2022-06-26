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

namespace re::edit {

class UserPreferences
{
public:
  ImVec4 fWidgetBorderColor{0,1,0,1};

  ImVec4 fWidgetErrorColor{1,0,0,0.5};

  ImVec4 fSelectedWidgetColor{1,1,0,1};
};

}

#endif //RE_EDIT_USER_PREFERENCES_H