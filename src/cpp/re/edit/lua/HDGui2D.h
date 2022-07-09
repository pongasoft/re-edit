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

#ifndef RE_EDIT_HD_GUI_2D_H
#define RE_EDIT_HD_GUI_2D_H

#include <re/mock/lua/MockJBox.h>
#include <re/mock/ObjectManager.hpp>
#include <variant>
#include <map>
#include "../Widget.h"

namespace re::edit::lua {

struct graphics_t {
  std::string fNode;
  // TODO hit_boundaries
};

struct jbox_widget
{
  graphics_t fGraphics;
  std::shared_ptr<Widget> fWidget;
};

struct jbox_panel {
  std::string fGraphicsNode;
  std::optional<std::string> fCableOrigin;
  std::vector<std::shared_ptr<jbox_widget>> fWidgets{};
};

namespace impl {

using jbox_object = std::variant<std::shared_ptr<jbox_widget>, std::shared_ptr<jbox_panel>>;

}

class HDGui2D : public re::mock::lua::MockJBox
{
public:
  HDGui2D();

  int luaPanel();
  int luaAnalogKnob();

  static std::unique_ptr<HDGui2D> fromFile(std::string const &iLuaFilename);

  std::shared_ptr<jbox_panel> front() { return getPanel("front"); }
  std::shared_ptr<jbox_panel> back() { return getPanel("back"); }

  static HDGui2D *loadFromRegistry(lua_State *L);

protected:
  int addObjectOnTopOfStack(impl::jbox_object iObject);
  std::optional<impl::jbox_object> getObjectOnTopOfStack();
  std::shared_ptr<jbox_panel> getPanel(char const *iPanelName);

  bool checkTableArg();

  void populateGraphics(std::shared_ptr<jbox_widget> oWidget);

  /**
   * Pushes onto the stack the value `t[k]`, where `t` is the value at the given index and executes f only if
   * this value is of type `iFieldType`. This function then properly pops the stack. */
  template<typename F>
  void withField(int index, char const *k, int iFieldType, F &&f);

private:
  re::mock::ObjectManager<impl::jbox_object> fObjects{};
};


}

#endif //RE_EDIT_HD_GUI_2D_H