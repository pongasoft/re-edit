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

#include "Base.h"
#include <re/mock/ObjectManager.hpp>
#include <variant>
#include <map>
#include "../Widget.h"

namespace re::edit::lua {

struct graphics_t {
  std::string fNode;
  std::optional<HitBoundaries> fHitBoundaries{};
};

struct jbox_widget
{
  graphics_t fGraphics;
  std::shared_ptr<Widget> fWidget;
};

struct jbox_panel {
  std::string fGraphicsNode;
  std::optional<std::string> fCableOrigin;
  std::vector<std::string> fOptions{};
  std::vector<std::shared_ptr<jbox_widget>> fWidgets{};
};

namespace impl {

struct jbox_ignored{};

struct jbox_ui_text
{
  std::string fText{};
};

struct jbox_image
{
  std::string fPath;
};

using jbox_object = std::variant<jbox_ignored, std::shared_ptr<jbox_widget>, jbox_ui_text, jbox_image>;

}

class HDGui2D : public Base
{
public:
  HDGui2D();

  int luaPanel();
  int luaAnalogKnob();
  int luaAudioInputSocket();
  int luaAudioOutputSocket();
  int luaCustomDisplay();
  int luaCVInputSocket();
  int luaCVOutputSocket();
  int luaCVTrimKnob();
  int luaDeviceName();
  int luaImage();
  int luaMomentaryButton();
  int luaPatchBrowseGroup();
  int luaPatchName();
  int luaPitchWheel();
  int luaPlaceholder();
  int luaPopupButton();
  int luaSampleBrowseGroup();
  int luaSampleDropZone();
  int luaSequenceFader();
  int luaSequenceMeter();
  int luaStaticDecoration();
  int luaStepButton();
  int luaRadioButton();
  int luaToggleButton();
  int luaUpDownButton();
  int luaValueDisplay();
  int luaZeroSnapKnob();
  int luaUIText();

  int luaIgnored();

  static std::unique_ptr<HDGui2D> fromFile(fs::path const &iLuaFilename);

  std::shared_ptr<jbox_panel> front() { return getPanel("front"); }
  std::shared_ptr<jbox_panel> folded_front() { return getPanel("folded_front"); }
  std::shared_ptr<jbox_panel> back() { return getPanel("back"); }
  std::shared_ptr<jbox_panel> folded_back() { return getPanel("folded_back"); }

  static HDGui2D *loadFromRegistry(lua_State *L);

protected:
  int addObjectOnTopOfStack(impl::jbox_object iObject);
  std::optional<impl::jbox_object> getObjectOnTopOfStack();
  std::optional<impl::jbox_ui_text> getTableValueAsOptionalUIText(char const *iKey, int idx = -1);
  std::optional<impl::jbox_image> getTableValueAsOptionalImage(char const *iKey, int idx = -1);
  std::shared_ptr<jbox_panel> getPanel(char const *iPanelName);

  void checkTableArg();

  void populateGraphics(std::shared_ptr<jbox_widget> &oWidget);

  void populate(widget::attribute::Value *oValue);
  void populate(widget::attribute::Bool *oValue);
  void populate(widget::attribute::String *oValue);
  void populate(widget::attribute::Integer *oValue);
  void populate(widget::attribute::Color3 *oValue);
  void populate(widget::attribute::Background *oValue);
  void populate(widget::attribute::UIText *oValue);
  void populate(widget::attribute::StaticStringList *oValue);
  void populate(widget::attribute::PropertyPathList *oList);
  void populate(widget::attribute::PropertyPath *oPath);
  void populate(widget::attribute::Visibility *oVisibility);
  void populate(widget::attribute::DiscretePropertyValueList *oList);
  void populate(widget::attribute::ValueTemplates *oValue);

  /**
   * Returns `true` if the widget has an attribute of the given type/name combination NOT if the population happens
   * (for example attribute not defined in the hdgui_2D.lua) */
  template<typename T>
  bool populate(std::shared_ptr<jbox_widget> &oWidget, std::string const &iAttributeName);

  /**
   * Pushes onto the stack the value `t[k]`, where `t` is the value at the given index and executes f only if
   * this value is of type `iFieldType`. This function then properly pops the stack. */
  template<typename F>
  void withField(int index, char const *k, int iFieldType, F f);

private:
  re::mock::ObjectManager<impl::jbox_object> fObjects{};
};


}

#endif //RE_EDIT_HD_GUI_2D_H