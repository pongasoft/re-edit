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

#ifndef RE_EDIT_BASE_H
#define RE_EDIT_BASE_H

#include <re/mock/lua/MockJBox.h>
#include <imgui.h>

namespace re::edit::lua {

class Base : public re::mock::lua::MockJBox
{
public:
  Base() = default;

protected:
  ImVec2 getImVec2(int idx = -1);
  std::optional<ImVec2> getOptionalImVec2(int idx = -1);
  std::optional<ImVec2> getOptionalImVec2TableField(char const *iKey, int idx = -1);
};

}

#endif //RE_EDIT_BASE_H