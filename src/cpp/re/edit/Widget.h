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

#ifndef RE_EDIT_WIDGET_H
#define RE_EDIT_WIDGET_H

#include "Constants.h"
#include "Texture.h"
#include "AppContext.h"
#include "WidgetAttribute.h"
#include "Graphics.h"
#include "Errors.h"

#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace re::edit {

class Panel;

class Widget
{
public:
  explicit Widget(WidgetType iType);

  constexpr std::string const &getName() const { return fName; }
  void setName(std::string iName) { fName = std::move(iName); }
  constexpr int getId() const { return fId; }
  constexpr WidgetType getType() const { return fType; }

  constexpr ImVec2 getPosition() const { return fGraphics->getPosition(); }
  constexpr ImVec2 getTopLeft() const { return fGraphics->getTopLeft(); }
  constexpr ImVec2 getBottomRight() const { return fGraphics->getBottomRight(); }
  constexpr ImVec2 getSize() const { return fGraphics->getSize(); }
  constexpr void setPosition(ImVec2 const &iPosition) { fGraphics->setPosition(iPosition); }

  constexpr bool isSelected() const { return fSelected; }

  constexpr bool isHidden() const { return fHidden; }

  constexpr bool isError() const { return fError; };
  constexpr void setError(bool iError) { fError = iError; };
  bool hasAttributeErrors() const;

  constexpr void setHitBoundaries(HitBoundaries const &iHitBoundaries) { fGraphics->setHitBoundaries(iHitBoundaries); }
  constexpr void disableHitBoundaries() { fGraphics->fHitBoundariesEnabled = false; }

  constexpr void move(ImVec2 const &iDelta) { fGraphics->move(iDelta); }

  inline void setTexture(std::shared_ptr<Texture> iTexture) { fGraphics->setTexture(std::move(iTexture)); }
  inline void setSize(ImVec2 const &iSize) { fGraphics->setSize(iSize); }

  constexpr int getFrameNumber() const { return fGraphics->fFrameNumber; }
  constexpr int &getFrameNumber() { return fGraphics->fFrameNumber; }
  constexpr void setFrameNumber(int iFrameNumber) { fGraphics->fFrameNumber = iFrameNumber; }

  constexpr Texture const *getTexture() const { return fGraphics->getTexture(); }

  inline bool contains(ImVec2 const &iPosition) const { return fGraphics->contains(iPosition); }
  inline bool overlaps(ImVec2 const &iTopLeft, ImVec2 const &iBottomRight) const { return fGraphics->overlaps(iTopLeft, iBottomRight); }

  void init(AppContext &iCtx);
  void draw(AppContext &iCtx);
  void editView(AppContext &iCtx);
  bool checkForErrors(AppContext &iCtx, bool iForceCheck = false);
  bool errorView(AppContext &iCtx);

  std::string hdgui2D(AppContext &iCtx) const;
  std::string device2D() const { return fGraphics->device2D(); }

  std::unique_ptr<Widget> copy() const;
  std::unique_ptr<Widget> clone() const;
//  bool eq(Widget *iWidget) const;

  static std::unique_ptr<Widget> panel_decal();
  static std::unique_ptr<Widget> analog_knob();
  static std::unique_ptr<Widget> audio_input_socket();
  static std::unique_ptr<Widget> audio_output_socket();
  static std::unique_ptr<Widget> custom_display();
  static std::unique_ptr<Widget> cv_input_socket();
  static std::unique_ptr<Widget> cv_output_socket();
  static std::unique_ptr<Widget> cv_trim_knob();
  static std::unique_ptr<Widget> device_name();
  static std::unique_ptr<Widget> momentary_button();
  static std::unique_ptr<Widget> patch_browse_group();
  static std::unique_ptr<Widget> patch_name();
  static std::unique_ptr<Widget> pitch_wheel();
  static std::unique_ptr<Widget> placeholder();
  static std::unique_ptr<Widget> popup_button();
  static std::unique_ptr<Widget> radio_button();
  static std::unique_ptr<Widget> sample_browse_group();
  static std::unique_ptr<Widget> sample_drop_zone();
  static std::unique_ptr<Widget> sequence_fader();
  static std::unique_ptr<Widget> sequence_meter();
  static std::unique_ptr<Widget> static_decoration();
  static std::unique_ptr<Widget> step_button();
  static std::unique_ptr<Widget> toggle_button();
  static std::unique_ptr<Widget> up_down_button();
  static std::unique_ptr<Widget> value_display();
  static std::unique_ptr<Widget> zero_snap_knob();

  template<typename T>
  T *findAttributeByNameAndType(std::string const &iAttributeName) const;

  template<typename T>
  T *findAttributeByIdAndType(int id) const;

  widget::Attribute *findAttributeByName(std::string const &iAttributeName) const;

  widget::Attribute *findAttributeById(int id) const { return fAttributes[id].get(); }

//
//  template<typename T>
//  typename T::value_t *findAttributeValue(std::string const &iAttributeName) const;

//  std::string *findValueValue() const { return findAttributeValue<widget::attribute::PropertyPath>("value"); }
//  std::string *findValueSwitchValue() const { return findAttributeValue<widget::attribute::ValueSwitch>("value_switch"); }
//  std::string *findVisibilitySwitchValue() const { return findAttributeValue<widget::attribute::VisibilitySwitch>("visibility_switch"); };

  friend class Panel;

protected:
  void computeIsHidden(AppContext &iCtx);
  bool isPanelDecal() const { return fType == WidgetType::kPanelDecal; }

protected:
  Widget *addAttribute(std::unique_ptr<widget::Attribute> iAttribute);
  Widget *value(Property::Filter iValueFilter, Property::Filter iValueSwitchFilter); // value + value_switch
  Widget *value(Property::Filter iValueFilter); // value only
  Widget *values(Property::Filter iValuesFilter);
  Widget *orientation();
  Widget *show_remote_box();
  Widget *show_automation_rect();
  Widget *text_color();
  Widget *text_style();
  Widget *tooltip_position();
  Widget *tooltip_template();
  Widget *visibility();
  Widget *blend_mode();
  Widget *horizontal_justification();
  Widget *socket(re::mock::JboxObjectType iSocketType, Object::Filter iSocketFilter);

private:
  void computeDefaultWidgetName();
  Widget(Widget const &iOther, std::string iName);
  Widget(Widget const &iOther);
  void init(int id) { fId = id; }

private:
  int fId{-1};
  WidgetType fType{};
  std::string fName{};
  bool fSelected{};
  bool fHidden{};
  bool fError{};
  std::vector<std::unique_ptr<widget::Attribute>> fAttributes{};

  // denormalized access to attributes
  widget::attribute::Graphics *fGraphics{};
  widget::attribute::Visibility *fVisibility{};

private:
  static long fWidgetIota;
};

//------------------------------------------------------------------------
// Widget::findAttributeByNameAndType
//------------------------------------------------------------------------
template<typename T>
T *Widget::findAttributeByNameAndType(std::string const &iAttributeName) const
{
  return dynamic_cast<T *>(findAttributeByName(iAttributeName));
}

//------------------------------------------------------------------------
// Widget::findAttributeByIdAndType
//------------------------------------------------------------------------
template<typename T>
T *Widget::findAttributeByIdAndType(int id) const
{
  return dynamic_cast<T *>(fAttributes[id].get());
}


}

#endif //RE_EDIT_WIDGET_H