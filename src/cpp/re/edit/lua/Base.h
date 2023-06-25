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

  std::optional<std::string> getReEditVersion();

protected:
  ImVec2 getImVec2(int idx = -1);
  std::optional<ImVec2> getOptionalImVec2(int idx = -1);
  std::optional<ImVec2> getOptionalImVec2TableField(char const *iKey, int idx = -1);
  inline bool isTableAt(int idx) { return lua_type(L, idx) == LUA_TTABLE; }
  inline bool isTableOnTopOfStack() { return isTableAt(-1); }

  /**
   * Pushes onto the stack the value `t[iFieldName]`, where `t` is the value at the given index and executes f only if
   * this value is of type `iFieldType`. This function then properly pops the stack. */
  template<typename F>
  bool withField(int index, char const *iFieldName, int iFieldType, F f);
};

//------------------------------------------------------------------------
// withOptionalValue
//------------------------------------------------------------------------
template<typename T, typename F>
inline void withOptionalValue(std::optional<T> const &iOptionalValue, F &&f)
{
  if(iOptionalValue)
    f(*iOptionalValue);
}

//------------------------------------------------------------------------
// withField
//------------------------------------------------------------------------
template<typename F>
bool Base::withField(int index, char const *iFieldName, int iFieldType, F f)
{
  bool res = false;
  if(lua_getfield(L, index, iFieldName) == iFieldType)
  {
    f();
    res = true;
  }
  lua_pop(L, 1);
  return res;
}

}

#endif //RE_EDIT_BASE_H