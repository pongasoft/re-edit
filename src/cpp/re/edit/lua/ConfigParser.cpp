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

#include "ConfigParser.h"

namespace re::edit::lua {

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
// GlobalConfigParser::fromString
//------------------------------------------------------------------------
config::Global GlobalConfigParser::fromString(std::string const &iLuaString)
{
  GlobalConfigParser parser{};
  parser.loadString(iLuaString);
  return parser.getConfig();
}

//------------------------------------------------------------------------
// GlobalConfigParser::getConfig
//------------------------------------------------------------------------
config::Global GlobalConfigParser::getConfig()
{
  config::Global c{};

  if(lua_getglobal(L, "global_config") == LUA_TTABLE)
  {
    withOptionalValue(L.getTableValueAsOptionalNumber("font_size"), [&c](auto v) { c.fFontSize = v; });
    if(lua_getfield(L, -1, "device_history") == LUA_TTABLE)
    {
      iterateLuaArray([this, &c](int idx) {
        if(lua_type(L, -1) == LUA_TTABLE)
        {
          config::Device item{};

          withOptionalValue(L.getTableValueAsOptionalString("name"), [&item](auto v) { item.fName = v; });
          withOptionalValue(L.getTableValueAsOptionalString("path"), [&item](auto v) { item.fPath = v; });
          withOptionalValue(L.getTableValueAsOptionalString("type"), [&item](auto v) { item.fType = v; });
          withOptionalValue(getOptionalImVec2TableField("native_window_size"), [&item](auto v) { item.fNativeWindowSize = v; });
          withOptionalValue(getOptionalImVec2TableField("native_window_pos"), [&item](auto v) { item.fNativeWindowPos = v; });
          withOptionalValue(L.getTableValueAsOptionalString("imgui.ini"), [&item](auto v) { item.fImGuiIni = std::move(v); });
          withOptionalValue(L.getTableValueAsOptionalBoolean("show_panel"), [&item](auto v) { item.fShowPanel = v; });
          withOptionalValue(L.getTableValueAsOptionalBoolean("show_panel_widgets"), [&item](auto v) { item.fShowPanelWidgets = v; });
          withOptionalValue(L.getTableValueAsOptionalBoolean("show_properties"), [&item](auto v) { item.fShowProperties = v; });
          withOptionalValue(L.getTableValueAsOptionalBoolean("show_widgets"), [&item](auto v) { item.fShowWidgets = v; });
          withOptionalValue(getOptionalImVec2TableField("grid"), [&item](auto v) { item.fGrid = v; });
          withOptionalValue(L.getTableValueAsOptionalNumber("last_access_time"), [&item](auto v) { item.fLastAccessTime = v; });

          c.addDeviceConfigToHistory(item);
        }
      }, true, false);
    }
    lua_pop(L, 1);
  }

  return c;
}

}