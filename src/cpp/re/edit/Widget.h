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
#include "EditContext.h"
#include "DrawContext.h"
#include "WidgetAttribute.h"

#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace re::edit {

class Panel;

class Widget
{
public:
  explicit Widget(std::string iType);

  constexpr std::string const &getName() const { return fName; }
  constexpr int getId() const { return fId; }

  constexpr ImVec2 getPosition() const { return fGraphics.getPosition(); }
  constexpr ImVec2 getTopLeft() const { return fGraphics.getTopLeft(); }
  constexpr ImVec2 getBottomRight() const { return fGraphics.getBottomRight(); }
  constexpr void setPosition(ImVec2 const &iPosition) { fGraphics.setPosition(iPosition); }

  constexpr bool isSelected() const { return fSelected; }
  constexpr void setSelected(bool iSelected) { fSelected = iSelected; }
  constexpr void toggleSelection() { fSelected = !fSelected; }

  constexpr bool isHidden() const { return fHidden; };
  constexpr void setHidden(bool iHidden) { fHidden = iHidden; };

  constexpr bool isError() const { return fError; };
  constexpr void setError(bool iError) { fError = iError; };

  constexpr void move(ImVec2 const &iDelta) { fGraphics.move(iDelta); }

  inline void setTexture(std::shared_ptr<Texture> iTexture) { fGraphics.setTexture(std::move(iTexture)); }
  constexpr int getFrameNumber() const { return fFrameNumber; }
  constexpr int &getFrameNumber() { return fFrameNumber; }
  constexpr void setFrameNumber(int iFrameNumber) { fFrameNumber = iFrameNumber; }

  constexpr Texture const *getTexture() const { return fGraphics.getTexture(); }

  inline bool contains(ImVec2 const &iPosition) const { return fGraphics.contains(iPosition); }

  void draw(DrawContext &iCtx);
  void editView(EditContext &iCtx);

  std::string hdgui2D() const;

  std::unique_ptr<Widget> clone() const;

  static std::unique_ptr<Widget> analog_knob();
  static std::unique_ptr<Widget> static_decoration();
  static std::unique_ptr<Widget> widget(std::string const &iType);

//  template<typename T>
//  T *findAttribute(std::string const &iAttributeName) const;
//
//  template<typename T>
//  typename T::value_t *findAttributeValue(std::string const &iAttributeName) const;

//  std::string *findValueValue() const { return findAttributeValue<widget::attribute::PropertyPath>("value"); }
//  std::string *findValueSwitchValue() const { return findAttributeValue<widget::attribute::ValueSwitch>("value_switch"); }
//  std::string *findVisibilitySwitchValue() const { return findAttributeValue<widget::attribute::VisibilitySwitch>("visibility_switch"); };

  friend class Panel;

protected:
  Widget *addAttribute(std::unique_ptr<widget::Attribute> iAttribute) { fAttributes.emplace_back(std::move(iAttribute)); return this; }
  Widget *value();
  Widget *show_remote_box();
  Widget *show_automation_rect();
  Widget *tooltip_position();
  Widget *tooltip_template();
  Widget *visibility();
  Widget *blend_mode();

private:
  void computeDefaultWidgetName();
  Widget(Widget const &iOther, std::string iName);
  void init(int id) { fId = id; }

private:
  int fId{};
  std::string fType{};
  std::string fName{};
  widget::attribute::Graphics fGraphics{};
  int fFrameNumber{};
  bool fSelected{};
  bool fError{};
  bool fHidden{};
  std::vector<std::unique_ptr<widget::Attribute>> fAttributes{};

private:
  static long fWidgetIota;
};

}

#endif //RE_EDIT_WIDGET_H