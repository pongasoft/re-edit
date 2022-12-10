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

#include "Base.h"

namespace re::edit::lua {

//------------------------------------------------------------------------
// Base::getOptionalImVec2
//------------------------------------------------------------------------
std::optional<ImVec2> Base::getOptionalImVec2(int idx)
{
  std::optional<ImVec2> res{};

  if(lua_type(L, idx) == LUA_TTABLE)
  {
    iterateLuaArray([this, &res](int index) {
      switch(index)
      {
        case 1:
          if(!res)
            res = ImVec2{};
          res->x = lua_tonumber(L, -1);
          break;

        case 2:
          res->y = lua_tonumber(L, -1);
          break;

        default:
          // ignored
          break;
      }
    }, true, false);
  }

  return res;
}

//------------------------------------------------------------------------
// Base::getImVec2
//------------------------------------------------------------------------
ImVec2 Base::getImVec2(int idx)
{
  auto res = getOptionalImVec2(idx);
  return res ? *res : ImVec2{};
}

//------------------------------------------------------------------------
// Base::getOptionalImVec2TableField
//------------------------------------------------------------------------
std::optional<ImVec2> Base::getOptionalImVec2TableField(char const *iKey, int idx)
{
  std::optional<ImVec2> res{};
  luaL_checktype(L, idx, LUA_TTABLE);
  if(lua_getfield(L, idx, iKey) != LUA_TNIL)
    res = getOptionalImVec2();
  lua_pop(L, 1);
  return res;
}

//------------------------------------------------------------------------
// Base::getReEditVersion
//------------------------------------------------------------------------
std::optional<std::string> Base::getReEditVersion()
{
  std::optional<std::string> version{};
  if(lua_getglobal(L, "re_edit") == LUA_TTABLE)
  {
    withOptionalValue(L.getTableValueAsOptionalString("version"), [&version](auto v) { version = v; });
  }
  lua_pop(L, 1);
  return version;
}



}