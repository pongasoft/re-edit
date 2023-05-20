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
#include "String.h"
#include "Clipboard.h"

#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace re::edit {

class Panel;

namespace widget {

enum class Visibility
{
  kByProperty,
  kManualVisible,
  kManualHidden
};

}

class Widget : public Editable
{
public:
  explicit Widget(WidgetType iType, std::optional<std::string> const &iName = std::nullopt);

  Widget &operator=(Widget const &iOther) = delete;
  Widget(Widget &&iOther) = delete;
  Widget &operator=(Widget &&iOther) = delete;

  inline PanelType getPanelType() const { return fPanelType; }

  inline std::string const &getName() const { return fName.value(); }
  inline StringWithHash::hash_t getNameHash() const { return fName.hash(); }
  void setName(const std::string& iName);
  constexpr int getId() const { return fId; }
  constexpr WidgetType getType() const { return fType; }

  constexpr ImVec2 getPosition() const { return fGraphics->getPosition(); }
  constexpr ImVec2 getTopLeft() const { return fGraphics->getTopLeft(); }
  constexpr ImVec2 getTopLeftFromCenter(ImVec2 const &iCenterPosition) const { return iCenterPosition - getSize() / 2.0f; }
  constexpr ImVec2 getBottomRight() const { return fGraphics->getBottomRight(); }
  inline ImVec2 getSize() const { return fGraphics->getSize(); }
  void setPosition(ImVec2 const &iPosition);
  inline void initPosition(ImVec2 const &iPosition) { setPositionAction(iPosition); }

  constexpr bool isSelected() const { return fSelected; }
  constexpr void select() { fSelected = true; }
  constexpr void unselect() { fSelected = false; }
  constexpr void setSelected(bool b) { fSelected = b; }

  constexpr bool isHidden() const { return fHidden; }
  constexpr bool canBeShown() const { return fHidden && fVisibilityAttribute && !fVisibilityAttribute->fSwitch.fValue.empty() && !fVisibilityAttribute->fValues.fValue.empty(); }
  constexpr bool hasVisibility() const { return fVisibilityAttribute && !fVisibilityAttribute->fSwitch.fValue.empty(); }
  constexpr bool hasVisibilityAttribute() const { return fVisibilityAttribute != nullptr; }
  bool hasVisibility(std::string const &iPropertyPath, int iPropertyValue) const;
  void setVisibility(widget::Visibility iVisibility);
  void addVisibility(std::string const &iPropertyPath, int iPropertyValue);
  void setVisibility(std::string const &iPropertyPath, int iPropertyValue);
  void removeVisibility(std::string const &iPropertyPath, int iPropertyValue);
  void toggleVisibility();
  constexpr widget::Visibility getVisibility() const { return fVisibility; }

  constexpr void setHitBoundaries(HitBoundaries const &iHitBoundaries) { fGraphics->setHitBoundaries(iHitBoundaries); fEdited |= fGraphics->isEdited(); }
  constexpr void disableHitBoundaries() { fGraphics->fHitBoundariesEnabled = false; }

  inline void setTextureKey(Texture::key_t const &iTextureKey) { fGraphics->setTextureKey(iTextureKey); fEdited |= fGraphics->isEdited(); }
  inline void setSize(ImVec2 const &iSize) { fGraphics->setSize(iSize); fEdited |= fGraphics->isEdited(); }
  void collectUsedTexturePaths(std::set<fs::path> &oPaths) const;
  void collectUsedTextureBuiltIns(std::set<FilmStrip::key_t> &oKeys) const;

  constexpr int getFrameNumber() const { return fGraphics->fFrameNumber; }
  constexpr int &getFrameNumber() { return fGraphics->fFrameNumber; }
  constexpr void setFrameNumber(int iFrameNumber) { fGraphics->fFrameNumber = iFrameNumber; }

  inline bool contains(ImVec2 const &iPosition) const { return fGraphics->contains(iPosition); }
  inline bool overlaps(ImVec2 const &iTopLeft, ImVec2 const &iBottomRight) const { return fGraphics->overlaps(iTopLeft, iBottomRight); }

  void init(AppContext &iCtx);
  void draw(AppContext &iCtx, ReGui::Canvas &iCanvas);
  void editView(AppContext &iCtx);
  bool checkForErrors(AppContext &iCtx) override;
  void markEdited() override;

  void resetEdited() override;

  std::string hdgui2D() const;
  std::string device2D() const { return fGraphics->device2D(); }

  std::unique_ptr<Widget> copy(std::string iName) const;
  std::unique_ptr<Widget> clone() const;
  std::unique_ptr<Widget> fullClone() const; // includes id/selected
//  bool eq(Widget *iWidget) const;
  bool copyFrom(Widget const &iWidget);
  bool copyFrom(widget::Attribute const *iAttribute);

  // action implementations (no undo)
  std::string setNameAction(std::string iName);
  bool copyFromAction(Widget const &iWidget);
  bool copyFromAction(widget::Attribute const *iAttribute);
  ImVec2 setPositionAction(ImVec2 const &iPosition);
  constexpr void moveAction(ImVec2 const &iDelta) { fGraphics->move(iDelta); fEdited |= fGraphics->isEdited(); }
  inline widget::Visibility setVisibilityAction(widget::Visibility iVisibility) { return setAction(&fVisibility, iVisibility); }

  static std::unique_ptr<Widget> panel_decal(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> analog_knob(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> audio_input_socket(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> audio_output_socket(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> custom_display(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> cv_input_socket(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> cv_output_socket(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> cv_trim_knob(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> device_name(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> momentary_button(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> patch_browse_group(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> patch_name(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> pitch_wheel(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> placeholder(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> popup_button(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> radio_button(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> sample_browse_group(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> sample_drop_zone(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> sequence_fader(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> sequence_meter(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> static_decoration(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> step_button(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> toggle_button(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> up_down_button(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> value_display(std::optional<std::string> const &iName = std::nullopt);
  static std::unique_ptr<Widget> zero_snap_knob(std::optional<std::string> const &iName = std::nullopt);

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

  static void resetWidgetIota() { fWidgetIota = 1; }

  static void sortByName(std::vector<Widget *> &iWidgets);
  static void selectByType(std::vector<Widget *> const &iWidgets, WidgetType iType, bool iIncludeHiddenWidgets);

  friend class Panel;
  friend class widget::Attribute;

protected:
  void computeIsHidden(AppContext &iCtx);
  void renderVisibilityMenu(AppContext &iCtx);
  void renderVisibilityToggle(AppContext &iCtx);
  bool isPanelDecal() const { return fType == WidgetType::kPanelDecal; }

protected:
  template<class T, class... Args >
  typename T::result_t executeAction(Args&&... args);

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
  std::string computeDefaultWidgetName() const;
  Widget(Widget const &iOther);
  void init(Panel *iParent, int id);

private:
  int fId{-1};
  PanelType fPanelType{PanelType::kUnknown};
  WidgetType fType{};
  StringWithHash fName{};
  bool fSelected{};
  widget::Visibility fVisibility{widget::Visibility::kByProperty};
  bool fHidden{};
  std::vector<std::unique_ptr<widget::Attribute>> fAttributes{};

  // denormalized access to attributes
  widget::attribute::Graphics *fGraphics{};
  widget::attribute::Visibility *fVisibilityAttribute{};

private:
  static inline long fWidgetIota{1};
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

namespace clipboard {
  class WidgetData : public Data
  {
  public:
    explicit WidgetData(std::unique_ptr<Widget> iWidget);
    DataType getType() const override { return DataType::kWidget; }
    Widget const *getWidget() const { return fWidget.get(); }

    static std::unique_ptr<WidgetData> copyFrom(Widget const *iWidget);

  private:
    std::unique_ptr<Widget> fWidget{};
  };

class WidgetAttributeData : public Data
{
public:
  WidgetAttributeData(std::unique_ptr<Widget> iWidget, int iAttributeId);
  DataType getType() const override { return DataType::kWidgetAttribute; }
  widget::Attribute const *getAttribute() const;

  static std::unique_ptr<WidgetAttributeData> copyFrom(Widget const *iWidget, int iAttributeId);

private:
  std::unique_ptr<Widget> fWidget;
  int fAttributeId;
};

class WidgetListData : public Data
{
public:
  explicit WidgetListData(std::vector<std::unique_ptr<Widget>> iWidgets);
  DataType getType() const override { return DataType::kWidgetList; }

  auto size() const { return fWidgets.size(); }
  std::vector<std::unique_ptr<Widget>> const &getWidgets() const { return fWidgets; }

  static std::unique_ptr<WidgetListData> copyFrom(std::vector<Widget *> const &iWidgets);

private:
  std::vector<std::unique_ptr<Widget>> fWidgets{};
};

}

}

#endif //RE_EDIT_WIDGET_H