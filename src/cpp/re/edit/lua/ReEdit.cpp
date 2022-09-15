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

#include "ReEdit.h"

namespace re::edit::lua {

//------------------------------------------------------------------------
// ReEdit::fromFile
//------------------------------------------------------------------------
std::unique_ptr<ReEdit> ReEdit::fromFile(std::string const &iLuaFilename)
{
  auto res = std::make_unique<ReEdit>();
  res->loadFile(iLuaFilename);
  return res;
}

//------------------------------------------------------------------------
// ReEdit::getConfig
//------------------------------------------------------------------------
Config ReEdit::getConfig()
{
  if(!fConfig)
    fConfig = loadConfig();

  return *fConfig;
}

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
// ReEdit::getConfig
//------------------------------------------------------------------------
Config ReEdit::loadConfig()
{
  Config c{};

  if(lua_getglobal(L, "re_edit") == LUA_TTABLE)
  {
    withOptionalValue(L.getTableValueAsOptionalInteger("native_window_width"), [&c](auto v) { c.fNativeWindowWidth = static_cast<int>(v); });
    withOptionalValue(L.getTableValueAsOptionalInteger("native_window_height"), [&c](auto v) { c.fNativeWindowHeight = static_cast<int>(v); });
    withOptionalValue(L.getTableValueAsOptionalString("imgui.ini"), [&c](auto v) { c.fImGuiIni = std::move(v); });
    withOptionalValue(L.getTableValueAsOptionalBoolean("show_panel"), [&c](auto v) { c.fShowPanel = v; });
    withOptionalValue(L.getTableValueAsOptionalBoolean("show_panel_widgets"), [&c](auto v) { c.fShowPanelWidgets = v; });
    withOptionalValue(L.getTableValueAsOptionalBoolean("show_properties"), [&c](auto v) { c.fShowProperties = v; });
    withOptionalValue(L.getTableValueAsOptionalBoolean("show_widgets"), [&c](auto v) { c.fShowWidgets = v; });
    withOptionalValue(L.getTableValueAsOptionalNumber("font_size"), [&c](auto v) { c.fFontSize = v; });
    withOptionalValue(getOptionalImVec2TableField("grid"), [&c](auto v) { c.fGrid = v; });
  }
  lua_pop(L, 1);

  return c;
}

}