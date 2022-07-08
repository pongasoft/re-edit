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

#include "../Errors.h"
#include "HDGui2D.h"

//------------------------------------------------------------------------
// Defining the C-API to invoke from lua as jbox.xxx
//------------------------------------------------------------------------
#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantFunctionResult"
extern "C" {

using namespace re::edit::lua;

static int lua_analog_knob(lua_State *L)
{
  return HDGui2D::loadFromRegistry(L)->luaAnalogKnob();
}

static int lua_panel(lua_State *L)
{
  return HDGui2D::loadFromRegistry(L)->luaPanel();
}

}

namespace re::edit::lua {

template<typename F>
void HDGui2D::withField(int index, char const *iFieldName, int iFieldType, F &&f)
{
  if(lua_getfield(L, index, iFieldName) == iFieldType)
    f();
  lua_pop(L, 1);
}

struct JBoxObjectUD
{
  int fId;

  static JBoxObjectUD *New(lua_State *L)
  {
    return reinterpret_cast<JBoxObjectUD *>(lua_newuserdata(L, sizeof(JBoxObjectUD)));
  }
};

//------------------------------------------------------------------------
// HDGui2D::HDGui2D
//------------------------------------------------------------------------
HDGui2D::HDGui2D()
{
  static const struct luaL_Reg jboxLib[] = {
    {"panel", lua_panel},
    {"analog_knob", lua_analog_knob},
    {nullptr,                    nullptr}
  };

  luaL_newlib(L, jboxLib);
  lua_setglobal(L, "jbox"); // will be available in realtime_controller.lua as jbox
}

//------------------------------------------------------------------------
// HDGui2D::HDGui2D
//------------------------------------------------------------------------
HDGui2D *HDGui2D::loadFromRegistry(lua_State *L)
{
  auto res = dynamic_cast<HDGui2D *>(MockJBox::loadFromRegistry(L));
  RE_EDIT_ASSERT(res != nullptr);
  return res;
}

//------------------------------------------------------------------------
// HDGui2D::addObjectOnTopOfStack
//------------------------------------------------------------------------
int HDGui2D::addObjectOnTopOfStack(impl::jbox_object iObject)
{
  auto ud = JBoxObjectUD::New(L);
  ud->fId = fObjects.add(std::move(iObject));
  return 1;
}

//------------------------------------------------------------------------
// HDGui2D::getObjectOnTopOfStack
//------------------------------------------------------------------------
std::optional<impl::jbox_object> HDGui2D::getObjectOnTopOfStack()
{
  if(lua_type(L, -1) == LUA_TNIL)
  {
    lua_pop(L, 1);
    return std::nullopt;
  }

  luaL_checktype(L, -1, LUA_TUSERDATA);
  auto ud = reinterpret_cast<JBoxObjectUD *>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  return fObjects.get(ud->fId);
}

//------------------------------------------------------------------------
// HDGui2D::checkTableArg
//------------------------------------------------------------------------
bool HDGui2D::checkTableArg()
{
  if(lua_gettop(L) <= 0 || lua_type(L, 1) != LUA_TTABLE)
  {
    RE_EDIT_LOG_WARNING("Missing table arg... Did you use () instead of {}?");
    return false;
  }
  else
    return true;
}

//------------------------------------------------------------------------
// makeWidget - convenient api
//------------------------------------------------------------------------
std::shared_ptr<jbox_widget> makeWidget(std::unique_ptr<Widget> iWidget)
{
  auto widget = std::make_shared<jbox_widget>();
  widget->fWidget = std::move(iWidget);
  return widget;
}

//------------------------------------------------------------------------
// HDGui2D::luaAnalogKnob
//------------------------------------------------------------------------
int HDGui2D::luaAnalogKnob()
{
  auto p = makeWidget(Widget::analog_knob());
  if(checkTableArg())
  {
    populateGraphics(p);
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::populateGraphics
//------------------------------------------------------------------------
void HDGui2D::populateGraphics(std::shared_ptr<jbox_widget> oWidget)
{
  withField(1, "graphics", LUA_TTABLE, [this, w = std::move(oWidget)]() {
    auto node = L.getTableValueAsString("node");
    w->fGraphics.fNode = L.getTableValueAsString("node");
  });
}

//------------------------------------------------------------------------
// toWidget
//------------------------------------------------------------------------
std::shared_ptr<jbox_widget> toWidget(std::optional<impl::jbox_object> iObject)
{
  if(!iObject)
    return nullptr;

  if(std::holds_alternative<std::shared_ptr<jbox_widget>>(iObject.value()))
    return std::get<std::shared_ptr<jbox_widget>>(iObject.value());
  else
    return nullptr;
}

//------------------------------------------------------------------------
// HDGui2D::luaPanel
//------------------------------------------------------------------------
int HDGui2D::luaPanel()
{
  auto p = std::make_shared<jbox_panel>();
  if(checkTableArg())
  {
    withField(1, "graphics", LUA_TTABLE, [this, p]() { p->fGraphicsNode = L.getTableValueAsString("node"); });
    withField(1, "cable_origin", LUA_TTABLE, [this, p]() { p->fCableOrigin = L.getTableValueAsString("node"); });
    withField(1, "widgets", LUA_TTABLE, [this, p]() {
      iterateLuaTable([this, p](lua_table_key_t const &key) {
        auto widget = toWidget(getObjectOnTopOfStack());
        if(widget)
          p->fWidgets.emplace_back(std::move(widget));
      }, false);
    });
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::fromFile
//------------------------------------------------------------------------
std::unique_ptr<HDGui2D> HDGui2D::fromFile(std::string const &iLuaFilename)
{
  auto res = std::make_unique<HDGui2D>();
  res->loadFile(iLuaFilename);
  return res;
}

//------------------------------------------------------------------------
// HDGui2D::getPanel
//------------------------------------------------------------------------
std::shared_ptr<jbox_panel> HDGui2D::getPanel(char const *iPanelName)
{
  if(lua_getglobal(L, iPanelName) != LUA_TNIL)
  {
    auto o = getObjectOnTopOfStack();
    if(o && std::holds_alternative<std::shared_ptr<jbox_panel>>(o.value()))
      return std::get<std::shared_ptr<jbox_panel>>(o.value());
  }

  return nullptr;
}


}
#pragma clang diagnostic pop