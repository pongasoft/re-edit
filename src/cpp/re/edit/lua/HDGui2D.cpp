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

static int lua_panel(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaPanel(); }
static int lua_ignored(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaIgnored(); }
static int lua_analog_knob(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaAnalogKnob(); }
static int lua_audio_input_socket(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaAudioInputSocket(); }
static int lua_audio_output_socket(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaAudioOutputSocket(); }
static int lua_custom_display(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaCustomDisplay(); }
static int lua_cv_input_socket(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaCVInputSocket(); }
static int lua_cv_output_socket(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaCVOutputSocket(); }
static int lua_cv_trim_knob(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaCVTrimKnob(); }
static int lua_device_name(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaDeviceName(); }
static int lua_image(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaImage(); }
static int lua_momentary_button(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaMomentaryButton(); }
static int lua_patch_browse_group(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaPatchBrowseGroup(); }
static int lua_patch_name(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaPatchName(); }
static int lua_pitch_wheel(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaPitchWheel(); }
static int lua_placeholder(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaPlaceholder(); }
static int lua_popup_button(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaPopupButton(); }
static int lua_radio_button(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaRadioButton(); }
static int lua_sample_browse_group(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaSampleBrowseGroup(); }
static int lua_sample_drop_zone(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaSampleDropZone(); }
static int lua_sequence_fader(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaSequenceFader(); }
static int lua_sequence_meter(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaSequenceMeter(); }
static int lua_static_decoration(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaStaticDecoration(); }
static int lua_step_button(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaStepButton(); }
static int lua_toggle_button(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaToggleButton(); }
static int lua_up_down_button(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaUpDownButton(); }
static int lua_value_display(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaValueDisplay(); }
static int lua_zero_snap_knob(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaZeroSnapKnob(); }
static int lua_ui_text(lua_State *L) { return HDGui2D::loadFromRegistry(L)->luaUIText(); }

}

namespace re::edit::lua {

using namespace widget::attribute;

template<typename F>
void HDGui2D::withField(int index, char const *iFieldName, int iFieldType, F f)
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
    {"audio_input_socket", lua_audio_input_socket},
    {"audio_output_socket", lua_audio_output_socket},
    {"custom_display", lua_custom_display},
    {"cv_input_socket", lua_cv_input_socket},
    {"cv_output_socket", lua_cv_output_socket},
    {"cv_trim_knob", lua_cv_trim_knob},
    {"device_name", lua_device_name},
    {"image", lua_image},
    {"momentary_button", lua_momentary_button},
    {"patch_browse_group", lua_patch_browse_group},
    {"patch_name", lua_patch_name},
    {"pitch_wheel", lua_pitch_wheel},
    {"placeholder", lua_placeholder},
    {"popup_button", lua_popup_button},
    {"radio_button", lua_radio_button},
    {"sample_browse_group", lua_sample_browse_group},
    {"sample_drop_zone", lua_sample_drop_zone},
    {"sequence_fader", lua_sequence_fader},
    {"sequence_meter", lua_sequence_meter},
    {"static_decoration", lua_static_decoration},
    {"step_button", lua_step_button},
    {"toggle_button", lua_toggle_button},
    {"ui_text", lua_ui_text},
    {"up_down_button", lua_up_down_button},
    {"value_display", lua_value_display},
    {"zero_snap_knob", lua_zero_snap_knob},
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
void HDGui2D::checkTableArg()
{
  luaL_checktype(L, 1, LUA_TTABLE);
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
// HDGui2D::luaIgnored
//------------------------------------------------------------------------
int HDGui2D::luaIgnored()
{
  return addObjectOnTopOfStack(impl::jbox_ignored{});
}

//------------------------------------------------------------------------
// HDGui2D::luaUIText
//------------------------------------------------------------------------
int HDGui2D::luaUIText()
{
  luaL_checktype(L, 1, LUA_TSTRING);
  return addObjectOnTopOfStack(impl::jbox_ui_text{lua_tostring(L, 1)});
}

//------------------------------------------------------------------------
// HDGui2D::luaImage
//------------------------------------------------------------------------
int HDGui2D::luaImage()
{
  luaL_checktype(L, 1, LUA_TTABLE);
  return addObjectOnTopOfStack(impl::jbox_image{L.getTableValueAsString("path", 1)});
}

//------------------------------------------------------------------------
// HDGui2D::luaAnalogKnob
//------------------------------------------------------------------------
int HDGui2D::luaAnalogKnob()
{
  auto p = makeWidget(Widget::analog_knob());
  checkTableArg();
  {
    populateGraphics(p);
    populate<Value>(p, "value");
    populate<Visibility>(p, "visibility");
    populate<StaticStringList>(p, "tooltip_position");
    populate<UIText>(p, "tooltip_template");
    populate<Bool>(p, "show_remote_box");
    populate<Bool>(p, "show_automation_rect");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaAudioOutputSocket
//------------------------------------------------------------------------
int HDGui2D::luaAudioOutputSocket()
{
  auto p = makeWidget(Widget::audio_output_socket());
  checkTableArg();
  {
    populateGraphics(p);
    populate<Socket>(p, "socket");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaAudioInputSocket
//------------------------------------------------------------------------
int HDGui2D::luaAudioInputSocket()
{
  auto p = makeWidget(Widget::audio_input_socket());
  checkTableArg();
  {
    populateGraphics(p);
    populate<Socket>(p, "socket");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaCustomDisplay
//------------------------------------------------------------------------
int HDGui2D::luaCustomDisplay()
{
  auto p = makeWidget(Widget::custom_display());
  checkTableArg();
  {
    populateGraphics(p);
    populate<Background>(p, "background");
    populate<Integer>(p, "display_width_pixels");
    populate<Integer>(p, "display_height_pixels");
    populate<PropertyPathList>(p, "values");
    populate<String>(p, "invalidate_function");
    populate<String>(p, "draw_function");
    populate<String>(p, "gesture_function");
    populate<Bool>(p, "show_remote_box");
    populate<Bool>(p, "show_automation_rect");
    populate<Visibility>(p, "visibility");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaCVOutputSocket
//------------------------------------------------------------------------
int HDGui2D::luaCVOutputSocket()
{
  auto p = makeWidget(Widget::cv_output_socket());
  checkTableArg();
  {
    populateGraphics(p);
    populate<Socket>(p, "socket");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaCVInputSocket
//------------------------------------------------------------------------
int HDGui2D::luaCVInputSocket()
{
  auto p = makeWidget(Widget::cv_input_socket());
  checkTableArg();
  {
    populateGraphics(p);
    populate<Socket>(p, "socket");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaCVTrimKnob
//------------------------------------------------------------------------
int HDGui2D::luaCVTrimKnob()
{
  auto p = makeWidget(Widget::cv_trim_knob());
  checkTableArg();
  {
    populateGraphics(p);
    populate<Socket>(p, "socket");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaDeviceName
//------------------------------------------------------------------------
int HDGui2D::luaDeviceName()
{
  auto p = makeWidget(Widget::device_name());
  checkTableArg();
  {
    populateGraphics(p);
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaMomentaryButton
//------------------------------------------------------------------------
int HDGui2D::luaMomentaryButton()
{
  auto p = makeWidget(Widget::momentary_button());
  checkTableArg();
  {
    populateGraphics(p);
    populate<PropertyPath>(p, "value");
    populate<Visibility>(p, "visibility");
    populate<StaticStringList>(p, "tooltip_position");
    populate<UIText>(p, "tooltip_template");
    populate<Bool>(p, "show_remote_box");
    populate<Bool>(p, "show_automation_rect");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaPatchBrowseGroup
//------------------------------------------------------------------------
int HDGui2D::luaPatchBrowseGroup()
{
  auto p = makeWidget(Widget::patch_browse_group());
  checkTableArg();
  {
    populateGraphics(p);
    populate<Bool>(p, "fx_patch");
    populate<StaticStringList>(p, "tooltip_position");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaPatchName
//------------------------------------------------------------------------
int HDGui2D::luaPatchName()
{
  auto p = makeWidget(Widget::patch_name());
  checkTableArg();
  {
    populateGraphics(p);
    populate<StaticStringList>(p, "text_style");
    populate<Color3>(p, "fg_color");
    populate<Color3>(p, "loader_alt_color");
    populate<Bool>(p, "center");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaPitchWheel
//------------------------------------------------------------------------
int HDGui2D::luaPitchWheel()
{
  auto p = makeWidget(Widget::pitch_wheel());
  checkTableArg();
  {
    populateGraphics(p);
    populate<PropertyPath>(p, "value");
    populate<Visibility>(p, "visibility");
    populate<StaticStringList>(p, "tooltip_position");
    populate<UIText>(p, "tooltip_template");
    populate<Bool>(p, "show_remote_box");
    populate<Bool>(p, "show_automation_rect");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaPlaceholder
//------------------------------------------------------------------------
int HDGui2D::luaPlaceholder()
{
  auto p = makeWidget(Widget::placeholder());
  checkTableArg();
  {
    populateGraphics(p);
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaPopupButton
//------------------------------------------------------------------------
int HDGui2D::luaPopupButton()
{
  auto p = makeWidget(Widget::popup_button());
  checkTableArg();
  {
    populateGraphics(p);
    populate<PropertyPath>(p, "value");
    populate<Visibility>(p, "visibility");
    populate<Color3>(p, "text_color");
    populate<StaticStringList>(p, "text_style");
    populate<Bool>(p, "show_remote_box");
    populate<Bool>(p, "show_automation_rect");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaSampleBrowseGroup
//------------------------------------------------------------------------
int HDGui2D::luaSampleBrowseGroup()
{
  auto p = makeWidget(Widget::sample_browse_group());
  checkTableArg();
  {
    populateGraphics(p);
    populate<Visibility>(p, "visibility");
    populate<StaticStringList>(p, "tooltip_position");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaSampleDropZone
//------------------------------------------------------------------------
int HDGui2D::luaSampleDropZone()
{
  auto p = makeWidget(Widget::sample_drop_zone());
  checkTableArg();
  {
    populateGraphics(p);
    populate<Visibility>(p, "visibility");
    populate<UserSampleIndex>(p, "user_sample_index");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaSequenceFader
//------------------------------------------------------------------------
int HDGui2D::luaSequenceFader()
{
  auto p = makeWidget(Widget::sequence_fader());
  checkTableArg();
  {
    populateGraphics(p);
    populate<Value>(p, "value");
    populate<StaticStringList>(p, "orientation");
    populate<Integer>(p, "inset1");
    populate<Integer>(p, "inset2");
    populate<Integer>(p, "handle_size");
    populate<Visibility>(p, "visibility");
    populate<StaticStringList>(p, "tooltip_position");
    populate<UIText>(p, "tooltip_template");
    populate<Bool>(p, "inverted");
    populate<Bool>(p, "show_remote_box");
    populate<Bool>(p, "show_automation_rect");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaSequenceMeter
//------------------------------------------------------------------------
int HDGui2D::luaSequenceMeter()
{
  auto p = makeWidget(Widget::sequence_meter());
  checkTableArg();
  {
    populateGraphics(p);
    populate<PropertyPath>(p, "value");
    populate<Visibility>(p, "visibility");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaStaticDecoration
//------------------------------------------------------------------------
int HDGui2D::luaStaticDecoration()
{
  auto p = makeWidget(Widget::static_decoration());
  checkTableArg();
  {
    populateGraphics(p);
    populate<Visibility>(p, "visibility");
    populate<StaticStringList>(p, "blend_mode");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaStepButton
//------------------------------------------------------------------------
int HDGui2D::luaStepButton()
{
  auto p = makeWidget(Widget::step_button());
  checkTableArg();
  {
    populateGraphics(p);
    populate<PropertyPath>(p, "value");
    populate<Visibility>(p, "visibility");
    populate<StaticStringList>(p, "tooltip_position");
    populate<UIText>(p, "tooltip_template");
    populate<Bool>(p, "increasing");
    populate<Bool>(p, "show_remote_box");
    populate<Bool>(p, "show_automation_rect");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaRadioButton
//------------------------------------------------------------------------
int HDGui2D::luaRadioButton()
{
  auto p = makeWidget(Widget::radio_button());
  checkTableArg();
  {
    populateGraphics(p);
    populate<PropertyPath>(p, "value");
    populate<Index>(p, "index");
    populate<Visibility>(p, "visibility");
    populate<StaticStringList>(p, "tooltip_position");
    populate<UIText>(p, "tooltip_template");
    populate<Bool>(p, "show_remote_box");
    populate<Bool>(p, "show_automation_rect");
  }
  return addObjectOnTopOfStack(std::move(p));
}


//------------------------------------------------------------------------
// HDGui2D::luaToggleButton
//------------------------------------------------------------------------
int HDGui2D::luaToggleButton()
{
  auto p = makeWidget(Widget::toggle_button());
  checkTableArg();
  {
    populateGraphics(p);
    populate<PropertyPath>(p, "value");
    populate<Visibility>(p, "visibility");
    populate<StaticStringList>(p, "tooltip_position");
    populate<UIText>(p, "tooltip_template");
    populate<Bool>(p, "show_remote_box");
    populate<Bool>(p, "show_automation_rect");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaUpDownButton
//------------------------------------------------------------------------
int HDGui2D::luaUpDownButton()
{
  auto p = makeWidget(Widget::up_down_button());
  checkTableArg();
  {
    populateGraphics(p);
    populate<PropertyPath>(p, "value");
    populate<Visibility>(p, "visibility");
    populate<Bool>(p, "inverted");
    populate<StaticStringList>(p, "tooltip_position");
    populate<UIText>(p, "tooltip_template");
    populate<Bool>(p, "show_remote_box");
    populate<Bool>(p, "show_automation_rect");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaValueDisplay
//------------------------------------------------------------------------
int HDGui2D::luaValueDisplay()
{
  auto p = makeWidget(Widget::value_display());
  checkTableArg();
  {
    populateGraphics(p);
    populate<Value>(p, "value");
    populate<ValueTemplates>(p, "value_templates");
    populate<Visibility>(p, "visibility");
    populate<StaticStringList>(p, "tooltip_position");
    populate<StaticStringList>(p, "horizontal_justification");
    populate<Color3>(p, "text_color");
    populate<StaticStringList>(p, "text_style");
    populate<UIText>(p, "tooltip_template");
    populate<Bool>(p, "read_only");
    populate<Bool>(p, "show_remote_box");
    populate<Bool>(p, "show_automation_rect");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::luaZeroSnapKnob
//------------------------------------------------------------------------
int HDGui2D::luaZeroSnapKnob()
{
  auto p = makeWidget(Widget::zero_snap_knob());
  checkTableArg();
  {
    populateGraphics(p);
    populate<Value>(p, "value");
    populate<Visibility>(p, "visibility");
    populate<StaticStringList>(p, "tooltip_position");
    populate<UIText>(p, "tooltip_template");
    populate<Bool>(p, "show_remote_box");
    populate<Bool>(p, "show_automation_rect");
  }
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// HDGui2D::populateGraphics
//------------------------------------------------------------------------
void HDGui2D::populateGraphics(std::shared_ptr<jbox_widget> &oWidget)
{
  withField(1, "graphics", LUA_TTABLE, [this, &oWidget]() {
    auto node = L.getTableValueAsString("node");
    oWidget->fGraphics.fNode = L.getTableValueAsString("node");
    withField(-1, "hit_boundaries", LUA_TTABLE, [this, &oWidget]() {
      HitBoundaries hb{};
      hb.fTopInset = static_cast<float>(L.getTableValueAsInteger("top"));
      hb.fBottomInset = static_cast<float>(L.getTableValueAsInteger("bottom"));
      hb.fLeftInset = static_cast<float>(L.getTableValueAsInteger("left"));
      hb.fRightInset = static_cast<float>(L.getTableValueAsInteger("right"));
      oWidget->fGraphics.fHitBoundaries = hb;
    });
  });
}

namespace impl {
//------------------------------------------------------------------------
// impl::setValue
//------------------------------------------------------------------------
template<typename T, typename V>
void setValue(T &oAttribute, std::optional<V> const &iValue)
{
  if(iValue)
  {
    oAttribute.fValue = *iValue;
    oAttribute.fProvided = true;
  }
}

}

//------------------------------------------------------------------------
// HDGui2D::populate | Value
//------------------------------------------------------------------------
void HDGui2D::populate(Value *oValue)
{
  if(oValue)
  {
    populate(&oValue->fValue);
    populate(&oValue->fValueSwitch);
    populate(&oValue->fValues);
    oValue->fUseSwitch = oValue->fValueSwitch.fProvided;
  }
}

//------------------------------------------------------------------------
// HDGui2D::populate | Bool
//------------------------------------------------------------------------
void HDGui2D::populate(Bool *oValue)
{
  if(oValue)
    impl::setValue(*oValue, L.getTableValueAsOptionalBoolean(oValue->fName));
}

//------------------------------------------------------------------------
// HDGui2D::populate | String
//------------------------------------------------------------------------
void HDGui2D::populate(String *oValue)
{
  if(oValue)
    impl::setValue(*oValue, L.getTableValueAsOptionalString(oValue->fName));
}

//------------------------------------------------------------------------
// HDGui2D::populate | Integer
//------------------------------------------------------------------------
void HDGui2D::populate(Integer *oValue)
{
  if(oValue)
    impl::setValue(*oValue, L.getTableValueAsOptionalInteger<int>(oValue->fName));
}

//------------------------------------------------------------------------
// HDGui2D::populate | Color3
//------------------------------------------------------------------------
void HDGui2D::populate(Color3 *oValue)
{
  if(oValue)
  {
    oValue->reset();
    withField(-1, oValue->fName, LUA_TTABLE, [this, oValue]() {
      oValue->fValue.fRed = static_cast<int>(L.getArrayValueAsInteger(1));
      oValue->fValue.fGreen = static_cast<int>(L.getArrayValueAsInteger(2));
      oValue->fValue.fBlue = static_cast<int>(L.getArrayValueAsInteger(3));
      oValue->fProvided = true;
    });
  }
}

//------------------------------------------------------------------------
// toOptional
//------------------------------------------------------------------------
template<typename T>
std::optional<T> toOptional(std::optional<impl::jbox_object> iObject)
{
  if(!iObject)
    return std::nullopt;

  if(std::holds_alternative<T>(iObject.value()))
    return std::get<T>(iObject.value());
  else
    return std::nullopt;
}


//------------------------------------------------------------------------
// LuaState::getTableValueAsOptionalUIText
//------------------------------------------------------------------------
std::optional<impl::jbox_image> HDGui2D::getTableValueAsOptionalImage(char const *iKey, int idx)
{
  std::optional<impl::jbox_image> res{};
  luaL_checktype(L, idx, LUA_TTABLE);
  if(lua_getfield(L, idx, iKey) != LUA_TNIL)
    res = toOptional<impl::jbox_image>(getObjectOnTopOfStack());
  else
    lua_pop(L, 1);
  return res;
}


//------------------------------------------------------------------------
// LuaState::getTableValueAsOptionalUIText
//------------------------------------------------------------------------
std::optional<impl::jbox_ui_text> HDGui2D::getTableValueAsOptionalUIText(char const *iKey, int idx)
{
  std::optional<impl::jbox_ui_text> res{};
  luaL_checktype(L, idx, LUA_TTABLE);
  if(lua_getfield(L, idx, iKey) != LUA_TNIL)
    res = toOptional<impl::jbox_ui_text>(getObjectOnTopOfStack());
  else
    lua_pop(L, 1);
  return res;
}


//------------------------------------------------------------------------
// HDGui2D::populate | UIText
//------------------------------------------------------------------------
void HDGui2D::populate(UIText *oValue)
{
  if(oValue)
  {
    auto uiText = getTableValueAsOptionalUIText(oValue->fName);
    if(uiText)
      impl::setValue(*oValue, std::optional<std::string>(uiText->fText));
  }
}

//------------------------------------------------------------------------
// HDGui2D::populate | Background
//------------------------------------------------------------------------
void HDGui2D::populate(Background *oValue)
{
  if(oValue)
  {
    auto image = getTableValueAsOptionalImage(oValue->fName);
    if(image)
      impl::setValue(*oValue, std::optional<std::string>(image->fPath));
  }
}

//------------------------------------------------------------------------
// HDGui2D::populate | StaticStringList
//------------------------------------------------------------------------
void HDGui2D::populate(StaticStringList *oValue)
{
  populate(static_cast<String *>(oValue));
}

//------------------------------------------------------------------------
// HDGui2D::populate | PropertyPath
//------------------------------------------------------------------------
void HDGui2D::populate(PropertyPath *oPath)
{
  if(oPath)
    impl::setValue(*oPath, L.getTableValueAsOptionalString(oPath->fName));
}

//------------------------------------------------------------------------
// HDGui2D::populate | PropertyPathList
//------------------------------------------------------------------------
void HDGui2D::populate(PropertyPathList *oList)
{
  if(oList)
  {
    oList->fValue.clear();
    withField(-1, oList->fName, LUA_TTABLE, [this, oList]() {
      iterateLuaArray([this, values = &oList->fValue](auto i) {
        if(lua_type(L, -1) == LUA_TSTRING)
          values->emplace_back(lua_tostring(L, -1));
      }, true, false);
      oList->fProvided = true;
    });
  }
}

//------------------------------------------------------------------------
// HDGui2D::populate | Visibility
//------------------------------------------------------------------------
void HDGui2D::populate(Visibility *oVisibility)
{
  if(oVisibility)
  {
    populate(&oVisibility->fSwitch);
    populate(&oVisibility->fValues);
  }
}

//------------------------------------------------------------------------
// HDGui2D::populate | DiscretePropertyValueList
//------------------------------------------------------------------------
void HDGui2D::populate(DiscretePropertyValueList *oList)
{
  if(oList)
  {
    oList->fValue.clear();
    withField(-1, oList->fName, LUA_TTABLE, [this, oList]() {
      iterateLuaArray([this, values = &oList->fValue](auto i) {
        if(lua_type(L, -1) == LUA_TNUMBER)
          values->emplace_back(static_cast<int>(lua_tonumber(L, -1)));
      }, true, false);
      oList->fProvided = true;
    });
  }
}

//------------------------------------------------------------------------
// HDGui2D::populate | ValueTemplates
//------------------------------------------------------------------------
void HDGui2D::populate(ValueTemplates *oList)
{
  if(oList)
  {
    oList->fValue.clear();
    withField(-1, oList->fName, LUA_TTABLE, [this, oList]() {
      iterateLuaArray([this, values = &oList->fValue](auto i) {
        auto ui_text = toOptional<impl::jbox_ui_text>(getObjectOnTopOfStack());
        if(ui_text)
          values->emplace_back(ui_text->fText);
      }, false, false);
      oList->fProvided = true;
    });
  }
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
  // Implementation note: we "delay" the processing of the map until when it is needed because there are cases
  // when the map argument is modified after calling jbox.panel() and it is a valid use case in Render2D
  // As a result this function simply returns the map that was provided as an argument (after making sure it is a map)
  checkTableArg();
  return 1;
}

//------------------------------------------------------------------------
// HDGui2D::fromFile
//------------------------------------------------------------------------
std::unique_ptr<HDGui2D> HDGui2D::fromFile(fs::path const &iLuaFilename)
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
  if(lua_getglobal(L, iPanelName) == LUA_TTABLE)
  {
    auto p = std::make_shared<jbox_panel>();
    withField(1, "graphics", LUA_TTABLE, [this, p]() { p->fGraphicsNode = L.getTableValueAsString("node"); });
    withField(1, "cable_origin", LUA_TTABLE, [this, p]() { p->fCableOrigin = L.getTableValueAsString("node"); });
    withField(1, "options", LUA_TTABLE, [this, p]() {
      iterateLuaArray([this, p](int i) {
        p->fOptions.emplace_back(lua_tostring(L, -1));
      }, true, false);
    });
    withField(1, "widgets", LUA_TTABLE, [this, p]() {
      iterateLuaTable([this, p](lua_table_key_t const &key) {
        auto widget = toWidget(getObjectOnTopOfStack());
        if(widget)
          p->fWidgets.emplace_back(std::move(widget));
      }, false);
    });
    return p;
  }
  lua_pop(L, 1);

  return nullptr;
}

//------------------------------------------------------------------------
// HDGui2D::populate
//------------------------------------------------------------------------
template<typename T>
bool HDGui2D::populate(std::shared_ptr<jbox_widget> &oWidget, std::string const &iAttributeName)
{
  auto const attribute = oWidget->fWidget->findAttributeByNameAndType<T>(iAttributeName);
  if(attribute)
  {
    populate(attribute);
    return true;
  }
  else
    return false;
}



}
#pragma clang diagnostic pop