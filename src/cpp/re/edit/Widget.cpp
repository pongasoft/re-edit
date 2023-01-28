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

#include "Widget.h"
#include "ReGui.h"
#include <re/mock/fmt.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include "Errors.h"
#include "Panel.h"

namespace re::edit {

using namespace widget;
using namespace widget::attribute;

//------------------------------------------------------------------------
// Widget::Widget
//------------------------------------------------------------------------
Widget::Widget(WidgetType iType, std::optional<std::string> const &iName) : fType{iType}
{
  setName(iName ? *iName : computeDefaultWidgetName());
  auto graphics = std::make_unique<Graphics>();
  fGraphics = graphics.get();
  addAttribute(std::move(graphics));
}

//------------------------------------------------------------------------
// Widget::Widget
//------------------------------------------------------------------------
Widget::Widget(Widget const &iOther) :
  fType(iOther.fType),
  fName(iOther.fName)
{
  for(auto &attribute: iOther.fAttributes)
  {
    auto newAttribute = attribute->clone();
    auto graphics = dynamic_cast<widget::attribute::Graphics *>(newAttribute.get());
    if(graphics)
    {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "LocalValueEscapesScope"
      fGraphics = graphics;
#pragma clang diagnostic pop
    }
    auto visibility = dynamic_cast<widget::attribute::Visibility *>(newAttribute.get());
    if(visibility)
    {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "LocalValueEscapesScope"
      fVisibility = visibility;
#pragma clang diagnostic pop
    }
    fAttributes.emplace_back(std::move(newAttribute));
  }
}

//------------------------------------------------------------------------
// Widget::Widget
//------------------------------------------------------------------------
Widget::Widget(Widget const &iOther, std::string iName) :
  fType(iOther.fType),
  fName(std::move(iName))
{
  for(auto &attribute: iOther.fAttributes)
  {
    auto newAttribute = attribute->clone();
    auto graphics = dynamic_cast<widget::attribute::Graphics *>(newAttribute.get());
    if(graphics)
    {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "LocalValueEscapesScope"
      fGraphics = graphics;
#pragma clang diagnostic pop
    }
    auto visibility = dynamic_cast<widget::attribute::Visibility *>(newAttribute.get());
    if(visibility)
    {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "LocalValueEscapesScope"
      fVisibility = visibility;
#pragma clang diagnostic pop
    }
    fAttributes.emplace_back(std::move(newAttribute));
  }
}

//------------------------------------------------------------------------
// Widget::draw
//------------------------------------------------------------------------
void Widget::draw(AppContext &iCtx, ReGui::Canvas &iCanvas)
{
  if(isHidden())
    return;

  ImU32 borderColor{};
  if(fSelected)
    borderColor = iCtx.getUserPreferences().fSelectedWidgetColor;
  else
  {
    if(iCtx.fBorderRendering == AppContext::EBorderRendering::kNormal)
      borderColor = iCtx.getUserPreferences().fWidgetBorderColor;
  }

  // widget is not rendered at all
  if(iCtx.fWidgetRendering == AppContext::EWidgetRendering::kNone)
  {
    // border and hit boundaries
    fGraphics->drawBorder(iCanvas, borderColor);
  }
  else
  {
    auto xRay = iCtx.fWidgetRendering == AppContext::EWidgetRendering::kNormal;

    switch(fType)
    {
      case WidgetType::kCustomDisplay:
        switch(iCtx.fCustomDisplayRendering)
        {
          case AppContext::ECustomDisplayRendering::kNone:
            fGraphics->drawBorder(iCanvas, borderColor);
            break;
          case AppContext::ECustomDisplayRendering::kMain:
            fGraphics->draw(iCtx, iCanvas, borderColor, xRay);
            break;
          case AppContext::ECustomDisplayRendering::kBackgroundSD:
          case AppContext::ECustomDisplayRendering::kBackgroundHD:
          {
            auto bgAttribute = findAttributeByNameAndType<Background>("background");
            if(!bgAttribute->draw(iCtx, iCanvas, fGraphics, borderColor, xRay))
              fGraphics->drawBorder(iCanvas, borderColor);
            break;
          }
          default:
            RE_EDIT_FAIL("not reached");
        }
        break;

      case WidgetType::kSampleDropZone:
        switch(iCtx.fSampleDropZoneRendering)
        {
          case AppContext::ESampleDropZoneRendering::kNone:
            fGraphics->drawBorder(iCanvas, borderColor);
            break;
          case AppContext::ESampleDropZoneRendering::kFill:
            fGraphics->draw(iCtx, iCanvas, borderColor, xRay);
            break;
        }
        break;

      default:
        fGraphics->draw(iCtx, iCanvas, borderColor, xRay);
        break;
    }
  }

  if(iCtx.fBorderRendering == AppContext::EBorderRendering::kHitBoundaries)
    fGraphics->drawHitBoundaries(iCanvas, ReGui::GetColorU32(kHitBoundariesColor));

  if(hasErrors())
    iCanvas.addRectFilled(fGraphics->fPosition, fGraphics->getSize(), iCtx.getUserPreferences().fWidgetErrorColor);
}

//------------------------------------------------------------------------
// Widget::init
//------------------------------------------------------------------------
void Widget::init(AppContext &iCtx)
{
  iCtx.setCurrentWidget(this);
  for(auto &att: fAttributes)
    att->init(iCtx);
  iCtx.setCurrentWidget(nullptr);
}

//------------------------------------------------------------------------
// Widget::markEdited
//------------------------------------------------------------------------
void Widget::markEdited()
{
  fEdited = true;

  for(auto &att: fAttributes)
    att->markEdited();
}

//------------------------------------------------------------------------
// Widget::resetEdited
//------------------------------------------------------------------------
void Widget::resetEdited()
{
  fEdited = false;

  for(auto &att: fAttributes)
    att->resetEdited();
}

//------------------------------------------------------------------------
// Widget::checkForErrors
//------------------------------------------------------------------------
bool Widget::checkForErrors(AppContext &iCtx)
{
  if(fEdited)
  {
    fUserError.clear();
    iCtx.setCurrentWidget(this);
    for(auto &att: fAttributes)
    {
      if(att->checkForErrors(iCtx))
        addAllErrors(att->fName, *att);
    }
    iCtx.setCurrentWidget(nullptr);
    fEdited = false;
  }

  return hasErrors();
}

//------------------------------------------------------------------------
// Widget::editView
//------------------------------------------------------------------------
void Widget::editView(AppContext &iCtx)
{
  iCtx.setCurrentWidget(this);

  ImGui::PushID("Widget");

  auto editedHashedName = fName;
  std::string editedName = editedHashedName.value();

  ImGui::PushID("ResetName");
  if(ReGui::ResetButton())
  {
    iCtx.addOrMergeUndoWidgetChange(this, &fName, fName.value(), editedName,
                                    fmt::printf("Rename %s %s widget", fName.c_str(), toString(fType)));
    setName(computeDefaultWidgetName());
  }
  ImGui::PopID();

  ImGui::SameLine();

  if(ImGui::InputText("name", &editedName))
  {
    iCtx.addOrMergeUndoWidgetChange(this, &fName, fName.value(), editedName,
                                    fmt::printf("Rename %s %s widget", fName.c_str(), toString(fType)));
    setName(editedName);
  }

  fGraphics->editPositionView(iCtx);

  for(auto &att: fAttributes)
  {
    ImGui::PushID(att->fName);
    att->editView(iCtx);
    if(att->isEdited())
      fEdited = true;
    att->errorViewSameLine();
    ImGui::PopID();
  }

  ImGui::Separator();

  if(!isPanelDecal())
  {
    if(ImGui::TreeNode("hdgui2D"))
    {
      auto size = ImGui::GetWindowSize();
      ImGui::PushTextWrapPos(size.x);
      ImGui::TextUnformatted(hdgui2D().c_str());
      ImGui::PopTextWrapPos();
      ImGui::TreePop();
    }
  }

  if(ImGui::TreeNode("device2D"))
  {
    auto size = ImGui::GetWindowSize();
    ImGui::PushTextWrapPos(size.x);
    ImGui::TextUnformatted(device2D().c_str());
    ImGui::PopTextWrapPos();
    ImGui::TreePop();
  }

  ImGui::PopID();

  iCtx.setCurrentWidget(nullptr);
}

//------------------------------------------------------------------------
// Widget::hdgui2D
//------------------------------------------------------------------------
std::string Widget::hdgui2D() const
{
  AppContext::GetCurrent().setCurrentWidget(this);

  if(isPanelDecal())
    return "";

  attribute_list_t atts{};

  for(auto &att: fAttributes)
    att->hdgui2D(atts);

  std::vector<std::string> l{};
  std::transform(atts.begin(), atts.end(), std::back_inserter(l), [](auto &att) {
    return re::mock::fmt::printf("  %s = %s", att.fName, att.fValue);
  });

  AppContext::GetCurrent().setCurrentWidget(nullptr);

  return re::mock::fmt::printf("jbox.%s {\n%s\n}", toString(fType), re::mock::stl::join_to_string(l, ",\n"));
}

//------------------------------------------------------------------------
// Widget::collectUsedTexturePaths
//------------------------------------------------------------------------
void Widget::collectUsedTexturePaths(std::set<fs::path> &oPaths) const
{
  for(auto &att: fAttributes)
    att->collectUsedTexturePaths(oPaths);
}

//------------------------------------------------------------------------
// Widget::hdgui2D
//------------------------------------------------------------------------
void Widget::collectUsedTextureBuiltIns(std::set<FilmStrip::key_t> &oKeys) const
{
  for(auto &att: fAttributes)
    att->collectUsedTextureBuiltIns(oKeys);
}

//------------------------------------------------------------------------
// Widget::value
//------------------------------------------------------------------------
Widget *Widget::value(Property::Filter iValueFilter, Property::Filter iValueSwitchFilter)
{
  return addAttribute(std::make_unique<Value>(std::move(iValueFilter), std::move(iValueSwitchFilter)));
}

//------------------------------------------------------------------------
// Widget::value
//------------------------------------------------------------------------
Widget *Widget::value(Property::Filter iValueFilter)
{
  return addAttribute(Attribute::build<PropertyPath>("value", true, "", std::move(iValueFilter)));
}

//------------------------------------------------------------------------
// Widget::values
//------------------------------------------------------------------------
Widget *Widget::values(Property::Filter iValuesFilter)
{
  return addAttribute(std::make_unique<Values>("values", std::move(iValuesFilter)));
}

//------------------------------------------------------------------------
// Widget::socket
//------------------------------------------------------------------------
Widget *Widget::socket(re::mock::JboxObjectType iSocketType, Object::Filter iSocketFilter)
{
  auto socket = std::make_unique<Socket>(iSocketType, std::move(iSocketFilter));
  socket->fRequired = true;
  return addAttribute(std::move(socket));
}

//------------------------------------------------------------------------
// Widget::visibility
//------------------------------------------------------------------------
Widget *Widget::visibility()
{
  auto visibility = std::make_unique<Visibility>();
#pragma clang diagnostic push
#pragma ide diagnostic ignored "LocalValueEscapesScope"
  fVisibility = visibility.get();
#pragma clang diagnostic pop
  return addAttribute(std::move(visibility));
}

//------------------------------------------------------------------------
// Widget::show_remote_box
//------------------------------------------------------------------------
Widget *Widget::show_remote_box()
{
  return addAttribute(Attribute::build<Bool>("show_remote_box", false, true));
}

//------------------------------------------------------------------------
// Widget::show_automation_rect
//------------------------------------------------------------------------
Widget *Widget::show_automation_rect()
{
  return addAttribute(Attribute::build<Bool>("show_automation_rect", false, true));
}

//------------------------------------------------------------------------
// Widget::orientation
//------------------------------------------------------------------------
Widget *Widget::orientation()
{
  static const std::vector<std::string> kOrientations = { "horizontal", "vertical"};
  return addAttribute(Attribute::build<StaticStringList>("orientation", false, "vertical", kOrientations));
}

//------------------------------------------------------------------------
// Widget::text_color
//------------------------------------------------------------------------
Widget *Widget::text_color()
{
  return addAttribute(Attribute::build<Color3>("text_color", true, JboxColor3{}));
}

//------------------------------------------------------------------------
// Widget::text_style
//------------------------------------------------------------------------
Widget *Widget::text_style()
{
  static const std::vector<std::string> textStyles =
    { "LCD font", "Bold LCD font", "Small LCD font", "Big bold LCD font", "Huge bold LCD font",
      "Label font", "Small label font",
      "Arial small font", "Arial medium small font", "Arial medium font", "Arial medium bold font",
      "Arial medium large font", "Arial medium large bold font", "Arial large font", "Arial large bold font",
      "Arial medium large toolbar font"};
  return addAttribute(Attribute::build<StaticStringList>("text_style", true, "LCD font", textStyles));
}

//------------------------------------------------------------------------
// Widget::tooltip_position
//------------------------------------------------------------------------
Widget *Widget::tooltip_position()
{
  static const std::vector<std::string> tooltipPositions =
    { "bottom_left", "bottom", "bottom_right", "right", "top_right", "top", "top_left", "left", "no_tooltip"};
  return addAttribute(Attribute::build<StaticStringList>("tooltip_position", false, "", tooltipPositions));
}

//------------------------------------------------------------------------
// Widget::tooltip_template
//------------------------------------------------------------------------
Widget *Widget::tooltip_template()
{
  return addAttribute(Attribute::build<UIText>("tooltip_template", false, ""));
}

//------------------------------------------------------------------------
// Widget::blend_mode
//------------------------------------------------------------------------
Widget *Widget::blend_mode()
{
  static const std::vector<std::string> blend_modes =
    { "normal", "luminance"};
  return addAttribute(Attribute::build<StaticStringList>("blend_mode", false, "normal", blend_modes));
}

//------------------------------------------------------------------------
// Widget::horizontal_justification
//------------------------------------------------------------------------
Widget *Widget::horizontal_justification()
{
  static const std::vector<std::string> kHorizontalJustification =
    { "left", "center", "right"};
  return addAttribute(Attribute::build<StaticStringList>("horizontal_justification", false, "center", kHorizontalJustification));
}

//------------------------------------------------------------------------
// Widget::computeDefaultWidgetName
//------------------------------------------------------------------------
std::string Widget::computeDefaultWidgetName() const
{
  return re::mock::fmt::printf("%s_%ld", toString(fType), fWidgetIota++);
}

//------------------------------------------------------------------------
// Widget::analog_knob
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::analog_knob(std::optional<std::string> const &iName)
{
  static const Property::Filter kValueFilter{[](const Property &p) {
    return (isOneOf(p.type(), Property::Type::kBoolean | Property::Type::kNumber) && kDocGuiOwnerFilter(p));
  }, "Must be a number or boolean property (document_owner or gui_owner)"};

  static const Property::Filter kValueSwitchFilter{[](const Property &p) {
    return p.isDiscrete() && kDocGuiOwnerFilter(p);
  }, "Must be a discrete (stepped) number property (document_owner or gui_owner)"};

  auto w = std::make_unique<Widget>(WidgetType::kAnalogKnob, iName);
  w ->value(kValueFilter, kValueSwitchFilter)
    ->visibility()
    ->tooltip_position()
    ->tooltip_template()
    ->show_remote_box()
    ->show_automation_rect()
    ;
  return w;
}

//------------------------------------------------------------------------
// Widget::audio_input_socket
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::audio_input_socket(std::optional<std::string> const &iName)
{
  static const Object::Filter kSocketFilter{[](const Object &p) {
    return p.type() == mock::JboxObjectType::kAudioInput;
  }, "Must be an audio input socket"};
  auto w = std::make_unique<Widget>(WidgetType::kAudioInputSocket, iName);
  w ->socket(Object::Type::kAudioInput, kSocketFilter)
    ->setSize(kAudioSocketSize)
    ;
  w->fGraphics->fFilter = FilmStrip::bySizeFilter(kAudioSocketSize, 5);
  w->setTextureKey(BuiltIns::kAudioSocket.fKey);
  return w;
}

//------------------------------------------------------------------------
// Widget::audio_output_socket
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::audio_output_socket(std::optional<std::string> const &iName)
{
  // TODO: "Player devices are allowed to have audio input sockets, but not audio output sockets."
  static const Object::Filter kSocketFilter = {[](const Object &p) {
    return p.type() == mock::JboxObjectType::kAudioOutput;
  }, "Must be an audio output socket"};
  auto w = std::make_unique<Widget>(WidgetType::kAudioOutputSocket, iName);
  w ->socket(Object::Type::kAudioOutput, kSocketFilter)
    ->setSize(kAudioSocketSize)
    ;
  w->fGraphics->fFilter = FilmStrip::bySizeFilter(kAudioSocketSize, 5);
  w->setTextureKey(BuiltIns::kAudioSocket.fKey);
  return w;
}

//------------------------------------------------------------------------
// Widget::custom_display
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::custom_display(std::optional<std::string> const &iName)
{
  // TODO: making up the rules as I go along (= run the sdk examples) as this is clearly NOT defined precisely
  // TODO: For example error message states that can use only custom properties (ex: cannot use audio socket connected)
  static const Property::Filter kValuesFilter{[](const Property &p) {
    return p.path() == "/environment/player_bypassed" ||
           (isOneOf(p.type(), Property::Type::kBoolean | Property::Type::kNumber | Property::Type::kString | Property::Type::kSample) &&
            (isOneOf(p.owner(), Property::Owner::kDocOwner | Property::Owner::kGUIOwner | Property::Owner::kRTOwner)));
  }, "Must be a number, string, boolean or sample property (document/gui/rt owner allowed)"};

  auto w = std::make_unique<Widget>(WidgetType::kCustomDisplay, iName);

  w ->addAttribute(Attribute::build<Background>("background", false, ""))
    ->addAttribute(Attribute::build<Integer>("display_width_pixels", true, 0))
    ->addAttribute(Attribute::build<Integer>("display_height_pixels", true, 0))
    ->addAttribute(Attribute::build<String>("draw_function", true, ""))
    ->addAttribute(Attribute::build<String>("invalidate_function", false, ""))
    ->addAttribute(Attribute::build<String>("gesture_function", false, ""))
    ->values(kValuesFilter)
    ->visibility()
    ->show_remote_box()
    ->show_automation_rect()
    ;

  w->disableHitBoundaries();
  return w;
}

//------------------------------------------------------------------------
// Widget::cv_input_socket
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::cv_input_socket(std::optional<std::string> const &iName)
{
  static const Object::Filter kSocketFilter{[](const Object &p) {
    return p.type() == mock::JboxObjectType::kCVInput;
  }, "Must be a cv input socket"};
  auto w = std::make_unique<Widget>(WidgetType::kCVInputSocket, iName);
  w ->socket(mock::JboxObjectType::kCVInput, kSocketFilter)
    ->setSize(kCVSocketSize)
    ;
  w->fGraphics->fFilter = FilmStrip::bySizeFilter(kCVSocketSize, 5);
  w->setTextureKey(BuiltIns::kCVSocket.fKey);
  return w;
}

//------------------------------------------------------------------------
// Widget::cv_output_socket
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::cv_output_socket(std::optional<std::string> const &iName)
{
  static const Object::Filter kSocketFilter{[](const Object &p) {
    return p.type() == mock::JboxObjectType::kCVOutput;
  }, "Must be a cv output socket"};
  auto w = std::make_unique<Widget>(WidgetType::kCVOutputSocket, iName);
  w ->socket(mock::JboxObjectType::kCVOutput, kSocketFilter)
    ->setSize(kCVSocketSize)
    ;
  w->fGraphics->fFilter = FilmStrip::bySizeFilter(kCVSocketSize, 5);
  w->setTextureKey(BuiltIns::kCVSocket.fKey);
  return w;
}

//------------------------------------------------------------------------
// Widget::cv_trim_knob
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::cv_trim_knob(std::optional<std::string> const &iName)
{
  static const Object::Filter kSocketFilter{[](const Object &p) {
    return p.type() == mock::JboxObjectType::kCVInput;
  }, "Must be a cv input socket"};
  auto w = std::make_unique<Widget>(WidgetType::kCVTrimKnob, iName);
  w ->socket(mock::JboxObjectType::kCVInput, kSocketFilter)
    ->setSize(kCVTrimKnobSize)
    ;
  w->fGraphics->fFilter = FilmStrip::bySizeFilter(kCVTrimKnobSize, 20);
  w->setTextureKey(BuiltIns::kTrimKnob.fKey);
  return w;
}


//------------------------------------------------------------------------
// Widget::device_name
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::device_name(std::optional<std::string> const &iName)
{
  static const auto kGraphicsFilter = FilmStrip::orFilter(FilmStrip::bySizeFilter(kDeviceNameHorizontal, 5),
                                                          FilmStrip::bySizeFilter(kDeviceNameVertical, 5));
  auto w = std::make_unique<Widget>(WidgetType::kDeviceName, iName);
  w->setSize(kDeviceNameHorizontal);
  w->fGraphics->fFilter = kGraphicsFilter;
  w->setTextureKey(BuiltIns::kTapeHorizontal.fKey);
  return w;
}

//------------------------------------------------------------------------
// Widget::momentary_button
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::momentary_button(std::optional<std::string> const &iName)
{
  static const Property::Filter kValueFilter{[](const Property &p) {
    return (p.type() == Property::Type::kBoolean || p.isDiscrete()) && kDocGuiOwnerFilter(p);
  }, "Must be a discrete (stepped) number or boolean property (document_owner or gui_owner)"};

  static const FilmStrip::Filter kGraphicsFilter{[](FilmStrip const &iFilmStrip) { return iFilmStrip.numFrames() == 2; },
                                                 "Must have exactly 2 frames"};

  auto w = std::make_unique<Widget>(WidgetType::kMomentaryButton, iName);
  w ->value(kValueFilter)
    ->visibility()
    ->tooltip_position()
    ->tooltip_template()
    ->show_remote_box()
    ->show_automation_rect()
    ;
  // exactly 2 frames
  w->fGraphics->fFilter = kGraphicsFilter;
  return w;
}

//------------------------------------------------------------------------
// Widget::patch_browse_group
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::patch_browse_group(std::optional<std::string> const &iName)
{
  auto w = std::make_unique<Widget>(WidgetType::kPatchBrowseGroup, iName);
  w ->tooltip_position()
    ->addAttribute(Attribute::build<Bool>("fx_patch", false, false))
    ;
  w->setSize(kPatchBrowseGroupSize);
  w->fGraphics->fFilter = FilmStrip::bySizeFilter(kPatchBrowseGroupSize, 5);
  w->setTextureKey(BuiltIns::kPatchBrowseGroup.fKey);
  return w;
}

//------------------------------------------------------------------------
// Widget::patch_name
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::patch_name(std::optional<std::string> const &iName)
{
  auto w = std::make_unique<Widget>(WidgetType::kPatchName, iName);
  w ->text_style()
    ->addAttribute(Attribute::build<Color3>("fg_color", true, JboxColor3{}))
    ->addAttribute(Attribute::build<Color3>("loader_alt_color", true, JboxColor3{}))
    ->addAttribute(Attribute::build<Bool>("center", false, false))
    ;
  return w;
}

//------------------------------------------------------------------------
// Widget::pitch_wheel
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::pitch_wheel(std::optional<std::string> const &iName)
{
  // TODO: note that there is currently no way to filter on performance_pitchbend as this information is not available
  static const Property::Filter kValueFilter{[](const Property &p) {
    return p.type() == Property::Type::kNumber && kDocGuiOwnerFilter(p);
  }, "Must be a number property (document_owner or gui_owner)"};
  auto w = std::make_unique<Widget>(WidgetType::kPitchWheel, iName);
  w ->value(kValueFilter)
    ->visibility()
    ->tooltip_position()
    ->tooltip_template()
    ->show_remote_box()
    ->show_automation_rect()
    ;
  return w;
}

//------------------------------------------------------------------------
// Widget::placeholder
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::placeholder(std::optional<std::string> const &iName)
{
  auto w = std::make_unique<Widget>(WidgetType::kPlaceholder, iName);
  w->setSize(kPlaceholderSize);
  w->fGraphics->fFilter = FilmStrip::bySizeFilter(kPlaceholderSize);
  w->setTextureKey(BuiltIns::kPlaceholder.fKey);
  return w;
}

//------------------------------------------------------------------------
// Widget::popup_button
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::popup_button(std::optional<std::string> const &iName)
{
  static const Property::Filter kValueFilter{[](const Property &p) {
    return (p.type() == Property::Type::kBoolean || p.isDiscrete()) && kDocGuiOwnerFilter(p);
  }, "Must be a discrete (stepped) number or boolean property (document_owner or gui_owner)"};

  auto w = std::make_unique<Widget>(WidgetType::kPopupButton, iName);
  w ->value(kValueFilter)
    ->visibility()
    ->text_style()
    ->text_color()
    ->show_remote_box()
    ->show_automation_rect()
    ;

  return w;
}

//------------------------------------------------------------------------
// Widget::radio_button
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::radio_button(std::optional<std::string> const &iName)
{
  static const Property::Filter kValueFilter{[](const Property &p) {
    return (p.type() == Property::Type::kBoolean || p.isDiscrete()) && kDocGuiOwnerFilter(p);
  }, "Must be a discrete (stepped) number or boolean property (document_owner or gui_owner)"};

  static const FilmStrip::Filter kGraphicsFilter{[](FilmStrip const &f) { return f.numFrames() == 2; }, "Must have exactly 2 frames"};

  auto w = std::make_unique<Widget>(WidgetType::kRadioButton, iName);
  w ->value(kValueFilter)
    ->visibility()
    ->addAttribute(Attribute::build<Index>("index", true, 0, 1))
    ->tooltip_position()
    ->tooltip_template()
    ->show_remote_box()
    ->show_automation_rect()
    ;
  // 2 frames
  w->fGraphics->fFilter = kGraphicsFilter;
  return w;
}

//------------------------------------------------------------------------
// Widget::sample_browse_group
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::sample_browse_group(std::optional<std::string> const &iName)
{
  auto w = std::make_unique<Widget>(WidgetType::kSampleBrowseGroup, iName);
  w ->visibility()
    ->tooltip_position()
    ;
  w->setSize(kPatchBrowseGroupSize);
  w->fGraphics->fFilter = FilmStrip::bySizeFilter(kSampleBrowseGroupSize, 5);
  w->setTextureKey(BuiltIns::kSampleBrowseGroup.fKey);
  return w;
}

//------------------------------------------------------------------------
// Widget::sample_drop_zone
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::sample_drop_zone(std::optional<std::string> const &iName)
{
  auto w = std::make_unique<Widget>(WidgetType::kSampleDropZone, iName);
  w ->visibility()
    ->addAttribute(Attribute::build<UserSampleIndex>("user_sample_index", true, 0))
    ;
  return w;
}

//------------------------------------------------------------------------
// Widget::sequence_fader
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::sequence_fader(std::optional<std::string> const &iName)
{
  static const Property::Filter kValueFilter{[](const Property &p) {
    return isOneOf(p.type(), Property::Type::kBoolean | Property::Type::kNumber) && kDocGuiOwnerFilter(p);
  }, "Must be a number or boolean property (document_owner or gui_owner)"};

  static const Property::Filter kValueSwitchFilter{[](const Property &p) {
    return p.isDiscrete() && kDocGuiOwnerFilter(p);
  }, "Must be a discrete (stepped) number property (document_owner or gui_owner)"};

  auto w = std::make_unique<Widget>(WidgetType::kSequenceFader, iName);
  w ->value(kValueFilter, kValueSwitchFilter)
    ->orientation()
    ->addAttribute(Attribute::build<Integer>("inset1", false, 0))
    ->addAttribute(Attribute::build<Integer>("inset2", false, 0))
    ->addAttribute(Attribute::build<Integer>("handle_size", false, 0))
    ->visibility()
    ->tooltip_position()
    ->tooltip_template()
    ->addAttribute(Attribute::build<Bool>("inverted", false, false))
    ->show_remote_box()
    ->show_automation_rect()
    ;
  return w;
}

//------------------------------------------------------------------------
// Widget::sequence_meter
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::sequence_meter(std::optional<std::string> const &iName)
{
  static const Property::Filter kValueFilter{[](const Property &p) {
    return isOneOf(p.type(), Property::Type::kBoolean | Property::Type::kNumber);
  }, "Must be a number or boolean property"};

  auto w = std::make_unique<Widget>(WidgetType::kSequenceMeter, iName);
  w ->value(kValueFilter)
    ->visibility()
    ;
  return w;
}

//------------------------------------------------------------------------
// Widget::static_decoration
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::static_decoration(std::optional<std::string> const &iName)
{
  // GUIDefValidation.GUIDefError: RE2DRender: Error in hdgui_2D.lua: Widget type 'static_decoration': Wrong number of frames (2)
  static const FilmStrip::Filter kGraphicsFilter{[](FilmStrip const &f) { return f.numFrames() == 1; }, "Must have exactly 1 frame"};

  auto w = std::make_unique<Widget>(WidgetType::kStaticDecoration, iName);
  w ->blend_mode()
    ->visibility()
    ;
  // 1 frames
  w->fGraphics->fFilter = kGraphicsFilter;
  return w;
}

//------------------------------------------------------------------------
// Widget::step_button
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::step_button(std::optional<std::string> const &iName)
{
  static const Property::Filter kValueFilter{[](const Property &p) {
    return (p.type() == Property::Type::kBoolean || p.isDiscrete()) && kDocGuiOwnerFilter(p);
  }, "Must be a discrete (stepped) number or boolean property (document_owner or gui_owner)"};

  static const FilmStrip::Filter kGraphicsFilter{[](FilmStrip const &f) {
    return f.numFrames() == 2 || f.numFrames() == 4;
  }, "Must have 2 or 4 frame"};

  auto w = std::make_unique<Widget>(WidgetType::kStepButton, iName);
  w ->value(kValueFilter)
    ->visibility()
    ->tooltip_position()
    ->tooltip_template()
    ->addAttribute(Attribute::build<Bool>("increasing", false, true))
    ->show_remote_box()
    ->show_automation_rect()
    ;
  // 2 frames
  w->fGraphics->fFilter = kGraphicsFilter;
  return w;
}

//------------------------------------------------------------------------
// Widget::toggle_button
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::toggle_button(std::optional<std::string> const &iName)
{
  static const Property::Filter kValueFilter{[](const Property &p) {
    return (p.type() == Property::Type::kBoolean || p.isDiscrete()) && kDocGuiOwnerFilter(p);
  }, "Must be a discrete (stepped) number or boolean property (document_owner or gui_owner)"};

  static const FilmStrip::Filter kGraphicsFilter{[](FilmStrip const &f) {
    return f.numFrames() == 2 || f.numFrames() == 4;
  }, "Must have 2 or 4 frame"};

  auto w = std::make_unique<Widget>(WidgetType::kToggleButton, iName);
  w ->value(kValueFilter)
    ->visibility()
    ->tooltip_position()
    ->tooltip_template()
    ->show_remote_box()
    ->show_automation_rect()
    ;
  // 2 or 4 frames
  w->fGraphics->fFilter = kGraphicsFilter;
  return w;
}

//------------------------------------------------------------------------
// Widget::up_down_button
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::up_down_button(std::optional<std::string> const &iName)
{
  static const Property::Filter kValueFilter{[](const Property &p) {
    return p.isDiscrete() && kDocGuiOwnerFilter(p);
  }, "Must be a discrete (stepped) number property (document_owner or gui_owner)"};

  static const FilmStrip::Filter kGraphicsFilter{[](FilmStrip const &f) { return f.numFrames() == 3; }, "Must have exactly 3 frame"};

  auto w = std::make_unique<Widget>(WidgetType::kUpDownButton, iName);
  w ->value(kValueFilter)
    ->visibility()
    ->tooltip_position()
    ->tooltip_template()
    ->addAttribute(Attribute::build<Bool>("inverted", false, false))
    ->show_remote_box()
    ->show_automation_rect()
    ;
  // 3 frames
  w->fGraphics->fFilter = kGraphicsFilter;
  return w;
}

//------------------------------------------------------------------------
// Widget::value_display
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::value_display(std::optional<std::string> const &iName)
{
  static const Property::Filter kValueSwitchFilter{[](const Property &p) {
    return p.isDiscrete() && kDocGuiOwnerFilter(p);
  }, "Must be a discrete (stepped) number property (document_owner or gui_owner)"};

  auto w = std::make_unique<Widget>(WidgetType::kValueDisplay, iName);
  w ->value(attribute::ReadOnly::kReadWriteValueFilter, kValueSwitchFilter)
    ->addAttribute(Attribute::build<ValueTemplates>("value_templates", false, {}, 1))
    ->visibility()
    ->text_style()
    ->text_color()
    ->horizontal_justification()
    ->tooltip_position()
    ->tooltip_template()
    ->addAttribute(Attribute::build<ReadOnly>("read_only", true, false, 1))
    ->show_remote_box()
    ->show_automation_rect()
    ;

  return w;
}

//------------------------------------------------------------------------
// Widget::zero_snap_knob
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::zero_snap_knob(std::optional<std::string> const &iName)
{
  static const Property::Filter kValueFilter{[](const Property &p) {
    return isOneOf(p.type(), Property::Type::kBoolean | Property::Type::kNumber) && kDocGuiOwnerFilter(p);
  }, "Must be a number or boolean property (document_owner or gui_owner)"};
  static const Property::Filter kValueSwitchFilter{[](const Property &p) {
    return p.isDiscrete() && kDocGuiOwnerFilter(p);
  }, "Must be a discrete (stepped) number property (document_owner or gui_owner)"};
  auto w = std::make_unique<Widget>(WidgetType::kZeroSnapKnob, iName);
  w ->value(kValueFilter, kValueSwitchFilter)
    ->visibility()
    ->tooltip_position()
    ->tooltip_template()
    ->show_remote_box()
    ->show_automation_rect()
    ;
  return w;
}

//------------------------------------------------------------------------
// Widget::panel_decal
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::panel_decal(std::optional<std::string> const &iName)
{
  auto w = std::make_unique<Widget>(WidgetType::kPanelDecal, iName);
  w->fGraphics->fSizeEnabled = false;
  w->fGraphics->fCheckForOOBError = false;
  return w;
}

//------------------------------------------------------------------------
// Widget::copy
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::copy(std::string iName) const
{
  return std::unique_ptr<Widget>(new Widget(*this, std::move(iName)));
}

//------------------------------------------------------------------------
// Widget::clone
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::clone() const
{
  return std::unique_ptr<Widget>(new Widget(*this));
}

//------------------------------------------------------------------------
// Widget::copyFrom
//------------------------------------------------------------------------
bool Widget::copyFrom(Widget const &iWidget)
{
  bool res = false;
  for(auto &att: fAttributes)
  {
    auto otherAtt = iWidget.findAttributeByName(att->fName);
    if(otherAtt)
      res |= att->copyFrom(otherAtt);
  }

  if(res)
    fEdited = true;

  return res;
}

//------------------------------------------------------------------------
// Widget::copyFrom
//------------------------------------------------------------------------
bool Widget::copyFrom(widget::Attribute const *iAttribute)
{
  auto att = findAttributeByName(iAttribute->fName);
  if(att)
  {
    auto res = att->copyFrom(iAttribute);
    if(res)
      fEdited = true;
    return res;
  }
  else
    return false;
}

////------------------------------------------------------------------------
//// Widget::eq
////------------------------------------------------------------------------
//bool Widget::eq(Widget *iWidget) const
//{
//  RE_EDIT_INTERNAL_ASSERT(iWidget != nullptr);
//  if(iWidget->fType != fType)
//    return false;
//
//  auto numAttributes = fAttributes.size();
//  for(auto i = 0; i < numAttributes; i++)
//  {
//    if(!fAttributes[i]->eq(iWidget->fAttributes[i].get()))
//      return false;
//  }
//
//  return true;
//}

//------------------------------------------------------------------------
// Widget::computeIsHidden
//------------------------------------------------------------------------
void Widget::computeIsHidden(AppContext &iCtx)
{
  fHidden = fVisibility != nullptr && fVisibility->isHidden(iCtx);
}

//------------------------------------------------------------------------
// Widget::showIfHidden
//------------------------------------------------------------------------
void Widget::showIfHidden(AppContext &iCtx)
{
  if(canBeShown())
  {
    iCtx.setPropertyValueAsInt(fVisibility->fSwitch.fValue, fVisibility->fValues.fValue[0]);
  }
}

//------------------------------------------------------------------------
// Widget::renderVisibilityMenu
//------------------------------------------------------------------------
void Widget::renderVisibilityMenu(AppContext &iCtx)
{
  if(hasVisibility())
  {
    if(ImGui::BeginMenu("Visibility"))
    {
      auto const &path = fVisibility->fSwitch.fValue;
      ReGui::TextSeparator(path.c_str());
      if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Watch, "Watch")))
        iCtx.addPropertyToWatchlist(path);
      if(fHidden)
      {
        auto const &values = fVisibility->fValues.fValue;
        if(!values.empty())
        {
          if(ImGui::MenuItem(fmt::printf("Show [value=%d]", values[0]).c_str()))
            iCtx.setPropertyValueAsInt(path, values[0]);
          if(values.size() > 1)
          {
            if(ImGui::BeginMenu(fmt::printf("Show with value", path).c_str()))
            {
              ReGui::TextSeparator("value");
              for(auto value: values)
              {
                if(ImGui::MenuItem(fmt::printf("%d", value).c_str()))
                  iCtx.setPropertyValueAsInt(path, value);
              }
              ImGui::EndMenu();
            }
          }
        }
      }
      ImGui::EndMenu();
    }
  }
}

//------------------------------------------------------------------------
// Widget::findAttributeByName
//------------------------------------------------------------------------
widget::Attribute *Widget::findAttributeByName(std::string const &iAttributeName) const
{
  auto iter = std::find_if(fAttributes.begin(), fAttributes.end(), [&iAttributeName](auto &a) { return a->fName == iAttributeName; });
  if(iter != fAttributes.end())
    return iter->get();
  else
    return nullptr;
}

//------------------------------------------------------------------------
// Widget::addAttribute
//------------------------------------------------------------------------
Widget *Widget::addAttribute(std::unique_ptr<widget::Attribute> iAttribute)
{
  auto id = static_cast<int>(fAttributes.size());
  iAttribute->init(id);
  fAttributes.emplace_back(std::move(iAttribute)); return this;
}

//------------------------------------------------------------------------
// Widget::setName
//------------------------------------------------------------------------
void Widget::setName(std::string iName)
{
  fName = StringWithHash(std::move(iName));
  fEdited = true;
}

namespace clipboard {

//------------------------------------------------------------------------
// WidgetData::WidgetData
//------------------------------------------------------------------------
WidgetData::WidgetData(std::unique_ptr<Widget> iWidget) :
  Data(fmt::printf("Widget: %s", iWidget->getName())),
  fWidget{std::move(iWidget)}
{
  // empty
}

//------------------------------------------------------------------------
// WidgetData::copyFrom
//------------------------------------------------------------------------
std::unique_ptr<WidgetData> WidgetData::copyFrom(std::shared_ptr<Widget> const &iWidget)
{
  RE_EDIT_INTERNAL_ASSERT(iWidget != nullptr);
  return std::make_unique<WidgetData>(iWidget->clone());
}

//------------------------------------------------------------------------
// WidgetAttributeData::WidgetAttributeData
//------------------------------------------------------------------------
WidgetAttributeData::WidgetAttributeData(std::unique_ptr<Widget> iWidget, int iAttributeId) :
  Data(fmt::printf("Attribute: %s", iWidget->findAttributeById(iAttributeId)->toValueString())),
  fWidget{std::move(iWidget)},
  fAttributeId{iAttributeId}
{
  // empty
}

//------------------------------------------------------------------------
// WidgetAttributeData::getAttribute
//------------------------------------------------------------------------
widget::Attribute const *WidgetAttributeData::getAttribute() const
{
  return fWidget->findAttributeById(fAttributeId);
}

//------------------------------------------------------------------------
// WidgetAttributeData::copyFrom
//------------------------------------------------------------------------
std::unique_ptr<WidgetAttributeData> WidgetAttributeData::copyFrom(std::shared_ptr<Widget> const &iWidget, int iAttributeId)
{
  RE_EDIT_INTERNAL_ASSERT(iWidget != nullptr && iAttributeId >= 0);
  return std::make_unique<WidgetAttributeData>(iWidget->clone(), iAttributeId);
}

//------------------------------------------------------------------------
// WidgetListData::WidgetListData
//------------------------------------------------------------------------
WidgetListData::WidgetListData(std::vector<std::unique_ptr<Widget>> iWidgets) :
  Data(fmt::printf("Widgets: %ld", iWidgets.size())),
  fWidgets{std::move(iWidgets)}
{
  // empty
}

//------------------------------------------------------------------------
// WidgetListData::copyFrom
//------------------------------------------------------------------------
std::unique_ptr<WidgetListData> WidgetListData::copyFrom(std::vector<std::shared_ptr<Widget>> const &iWidgets)
{
  std::vector<std::unique_ptr<Widget>> list{};
  list.reserve(iWidgets.size());
  for(auto &w: iWidgets)
    list.emplace_back(w->clone());
  return std::make_unique<WidgetListData>(std::move(list));
}

}

}

