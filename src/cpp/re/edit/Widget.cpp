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

long Widget::fWidgetIota = 1;

//------------------------------------------------------------------------
// Widget::Widget
//------------------------------------------------------------------------
Widget::Widget(WidgetType iType) : fType{iType}
{
  computeDefaultWidgetName();
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
  fGraphics->setPosition(iOther.getPosition() + ImVec2(iOther.fGraphics->getSize().x + 5,0));
  fSelected = true;
}

//------------------------------------------------------------------------
// Widget::draw
//------------------------------------------------------------------------
void Widget::draw(AppContext &iCtx)
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
    fGraphics->drawBorder(iCtx, borderColor);
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
            fGraphics->drawBorder(iCtx, borderColor);
            break;
          case AppContext::ECustomDisplayRendering::kMain:
            fGraphics->draw(iCtx, borderColor, xRay);
            break;
          case AppContext::ECustomDisplayRendering::kBackgroundSD:
          case AppContext::ECustomDisplayRendering::kBackgroundHD:
          {
            auto bgAttribute = findAttributeByNameAndType<Background>("background");
            if(!bgAttribute->draw(iCtx, fGraphics, borderColor, xRay))
              fGraphics->drawBorder(iCtx, borderColor);
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
            fGraphics->drawBorder(iCtx, borderColor);
            break;
          case AppContext::ESampleDropZoneRendering::kFill:
            fGraphics->draw(iCtx, borderColor, xRay);
            break;
        }
        break;

      default:
        fGraphics->draw(iCtx, borderColor, xRay);
        break;
    }
  }

  if(iCtx.fBorderRendering == AppContext::EBorderRendering::kHitBoundaries)
    fGraphics->drawHitBoundaries(iCtx, ReGui::GetColorU32(kHitBoundariesColor));

  if(fError)
    iCtx.drawRectFilled(fGraphics->fPosition, fGraphics->getSize(), iCtx.getUserPreferences().fWidgetErrorColor);
}

//------------------------------------------------------------------------
// Widget::hasAttributeErrors
//------------------------------------------------------------------------
bool Widget::hasAttributeErrors() const
{
  for(auto &att: fAttributes)
  {
    if(att->fError)
      return true;
  }
  return false;
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
// Widget::checkForErrors
//------------------------------------------------------------------------
bool Widget::checkForErrors(AppContext &iCtx, bool iForceCheck)
{
  fError = false;

  iCtx.setCurrentWidget(this);
  for(auto &att: fAttributes)
  {
    if(iForceCheck || att->fEdited)
    {
      auto error = att->checkForErrors(iCtx);
      fError |= error != widget::Attribute::kNoError;
      att->fError = error;
      att->resetEdited();
    }
    else
      fError |= att->fError != widget::Attribute::kNoError;
  }
  iCtx.setCurrentWidget(nullptr);
  return fError;
}

//------------------------------------------------------------------------
// Widget::errorView
//------------------------------------------------------------------------
bool Widget::errorView(AppContext &iCtx)
{
  if(fError)
  {
    ImGui::TextColored(ImVec4(1,0,0,1), ReGui::kErrorIcon);
    if(ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
      for(auto &att: fAttributes)
      {
        if(att->fError)
          ImGui::Text("%s | %s", att->fName, att->fError);
      }
      ImGui::PopTextWrapPos();
      ImGui::EndTooltip();
    }
    return true;
  }

  return false;
}

//------------------------------------------------------------------------
// Widget::editView
//------------------------------------------------------------------------
void Widget::editView(AppContext &iCtx)
{
  iCtx.setCurrentWidget(this);

  ImGui::PushID("Widget");

  auto editedName = fName;

  if(ImGui::InputText("name", &editedName))
  {
    iCtx.addOrMergeUndoWidgetChange(this, &fName, fName, editedName,
                                    fmt::printf("Rename %s %s widget", fName, toString(fType)));
    fName = editedName;
  }

  fGraphics->editPositionView(iCtx);

  if(ImGui::TreeNode("Attributes"))
  {
    for(auto &att: fAttributes)
    {
      ImGui::PushID(att->fName);
      att->editView(iCtx);
      if(att->fError)
      {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1,0,0,1), ReGui::kErrorIcon);
        if(ImGui::IsItemHovered())
        {
          ImGui::BeginTooltip();
          ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
          ImGui::TextUnformatted(att->fError);
          ImGui::PopTextWrapPos();
          ImGui::EndTooltip();
        }
      }
      ImGui::PopID();
    }
    ImGui::TreePop();
  }

  if(!isPanelDecal())
  {
    if(ImGui::TreeNode("hdgui2D"))
    {
      auto size = ImGui::GetWindowSize();
      ImGui::PushTextWrapPos(size.x);
      ImGui::TextUnformatted(hdgui2D(iCtx).c_str());
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
std::string Widget::hdgui2D(AppContext &iCtx) const
{
  iCtx.setCurrentWidget(this);

  if(isPanelDecal())
    return "";

  attribute_list_t atts{};

  for(auto &att: fAttributes)
    att->hdgui2D(iCtx, atts);

  std::vector<std::string> l{};
  std::transform(atts.begin(), atts.end(), std::back_inserter(l), [](auto &att) {
    return re::mock::fmt::printf("  %s = %s", att.fName, att.fValue);
  });
  return re::mock::fmt::printf("jbox.%s {\n%s\n}", toString(fType), re::mock::stl::join_to_string(l, ",\n"));

  iCtx.setCurrentWidget(nullptr);
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
  return addAttribute(std::make_unique<PropertyPath>("value", std::move(iValueFilter)));
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
  return addAttribute(std::make_unique<Socket>(iSocketType, std::move(iSocketFilter)));
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
void Widget::computeDefaultWidgetName()
{
  fName = re::mock::fmt::printf("%s_%ld", toString(fType), fWidgetIota++);
}

//------------------------------------------------------------------------
// Widget::analog_knob
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::analog_knob()
{
  static const auto kValueFilter = [](const Property &p) {
    return (p.type() == kJBox_Boolean || p.type() == kJBox_Number) && kDocGuiOwnerFilter(p);
  };
  static const auto kValueSwitchFilter = [](const Property &p) { return p.isDiscrete() && kDocGuiOwnerFilter(p); };
  auto w = std::make_unique<Widget>(WidgetType::kAnalogKnob);
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
std::unique_ptr<Widget> Widget::audio_input_socket()
{
  static const auto kSocketFilter = [](const Object &p) {
    return p.type() == mock::JboxObjectType::kAudioInput;
  };
  auto w = std::make_unique<Widget>(WidgetType::kAudioInputSocket);
  w ->socket(mock::JboxObjectType::kAudioInput, kSocketFilter)
    ->setSize(kAudioSocketSize)
    ;
  w->fGraphics->fFilter = FilmStrip::bySizeFilter(kAudioSocketSize);
  return w;
}

//------------------------------------------------------------------------
// Widget::audio_output_socket
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::audio_output_socket()
{
  static const auto kSocketFilter = [](const Object &p) {
    return p.type() == mock::JboxObjectType::kAudioOutput;
  };
  auto w = std::make_unique<Widget>(WidgetType::kAudioOutputSocket);
  w ->socket(mock::JboxObjectType::kAudioOutput, kSocketFilter)
    ->setSize(kAudioSocketSize)
    ;
  w->fGraphics->fFilter = FilmStrip::bySizeFilter(kAudioSocketSize);
  return w;
}

//------------------------------------------------------------------------
// Widget::custom_display
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::custom_display()
{
  // TODO error message states that can use only custom properties (ex: cannot use audio socket connected)
  // TODO but I fear that it is not entirely correct... what about sample rate? Need to investigate
  static const auto kValuesFilter = [](const Property &p) {
    return p.parent().type() == mock::JboxObjectType::kCustomProperties &&
           (p.type() == kJBox_Boolean || p.type() == kJBox_Number || p.type() == kJBox_String);
  };

  auto w = std::make_unique<Widget>(WidgetType::kCustomDisplay);

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
std::unique_ptr<Widget> Widget::cv_input_socket()
{
  static const auto kSocketFilter = [](const Object &p) {
    return p.type() == mock::JboxObjectType::kCVInput;
  };
  auto w = std::make_unique<Widget>(WidgetType::kCVInputSocket);
  w ->socket(mock::JboxObjectType::kCVInput, kSocketFilter)
    ->setSize(kCVSocketSize)
    ;
  w->fGraphics->fFilter = FilmStrip::bySizeFilter(kCVSocketSize);
  return w;
}

//------------------------------------------------------------------------
// Widget::cv_output_socket
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::cv_output_socket()
{
  static const auto kSocketFilter = [](const Object &p) {
    return p.type() == mock::JboxObjectType::kCVOutput;
  };
  auto w = std::make_unique<Widget>(WidgetType::kCVOutputSocket);
  w ->socket(mock::JboxObjectType::kCVOutput, kSocketFilter)
    ->setSize(kCVSocketSize)
    ;
  w->fGraphics->fFilter = FilmStrip::bySizeFilter(kCVSocketSize);
  return w;
}

//------------------------------------------------------------------------
// Widget::cv_trim_knob
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::cv_trim_knob()
{
  static const auto kSocketFilter = [](const Object &p) {
    return p.type() == mock::JboxObjectType::kCVInput;
  };
  auto w = std::make_unique<Widget>(WidgetType::kCVTrimKnob);
  w ->socket(mock::JboxObjectType::kCVInput, kSocketFilter)
    ->setSize(kCVTrimKnobSize)
    ;
  w->fGraphics->fFilter = FilmStrip::bySizeFilter(kCVTrimKnobSize);
  return w;
}


//------------------------------------------------------------------------
// Widget::device_name
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::device_name()
{
  static const auto kGraphicsFilter = FilmStrip::orFilter(FilmStrip::bySizeFilter(kDeviceNameHorizontal),
                                                          FilmStrip::bySizeFilter(kDeviceNameVertical));
  auto w = std::make_unique<Widget>(WidgetType::kDeviceName);
  w->setSize(kDeviceNameHorizontal);
  w->fGraphics->fFilter = kGraphicsFilter;
  return w;
}

//------------------------------------------------------------------------
// Widget::momentary_button
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::momentary_button()
{
  static const auto kValueFilter = [](const Property &p) {
    return (p.type() == kJBox_Boolean || p.isDiscrete()) && kDocGuiOwnerFilter(p);
  };
  auto w = std::make_unique<Widget>(WidgetType::kMomentaryButton);
  w ->value(kValueFilter)
    ->visibility()
    ->tooltip_position()
    ->tooltip_template()
    ->show_remote_box()
    ->show_automation_rect()
    ;
  // exactly 2 frames
  w->fGraphics->fFilter = [](FilmStrip const &iFilmStrip) { return iFilmStrip.numFrames() == 2; };
  return w;
}

//------------------------------------------------------------------------
// Widget::patch_browse_group
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::patch_browse_group()
{
  auto w = std::make_unique<Widget>(WidgetType::kPatchBrowseGroup);
  w ->tooltip_position()
    ->addAttribute(Attribute::build<Bool>("fx_patch", false, false))
    ;
  w->setSize(kPatchBrowseGroupSize);
  w->fGraphics->fFilter = FilmStrip::bySizeFilter(kPatchBrowseGroupSize);
  return w;
}

//------------------------------------------------------------------------
// Widget::patch_name
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::patch_name()
{
  auto w = std::make_unique<Widget>(WidgetType::kPatchName);
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
std::unique_ptr<Widget> Widget::pitch_wheel()
{
  // TODO: note that there is currently no way to filter on performance_pitchbend as this information is not available
  static const auto kValueFilter = [](const Property &p) {
    return p.type() == kJBox_Number && kDocGuiOwnerFilter(p);
  };
  auto w = std::make_unique<Widget>(WidgetType::kPitchWheel);
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
std::unique_ptr<Widget> Widget::placeholder()
{
  auto w = std::make_unique<Widget>(WidgetType::kPlaceholder);
  w->setSize(kPlaceholderSize);
  w->fGraphics->fFilter = FilmStrip::bySizeFilter(kPlaceholderSize);
  return w;
}


//------------------------------------------------------------------------
// Widget::popup_button
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::popup_button()
{
  static const auto kValueFilter = [](const Property &p) {
    return (p.type() == kJBox_Boolean || p.isDiscrete()) && kDocGuiOwnerFilter(p);
  };
  auto w = std::make_unique<Widget>(WidgetType::kPopupButton);
  w ->value(kValueFilter)
    ->visibility()
    ->text_style()
    ->text_color()
    ->show_remote_box()
    ->show_automation_rect()
    ;
  // 2 or 4 frames
  w->fGraphics->fFilter = [](FilmStrip const &iFilmStrip) { return iFilmStrip.numFrames() == 2 || iFilmStrip.numFrames() == 4; };
  return w;
}

//------------------------------------------------------------------------
// Widget::radio_button
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::radio_button()
{
  static const auto kValueFilter = [](const Property &p) {
    return (p.type() == kJBox_Boolean || p.isDiscrete()) && kDocGuiOwnerFilter(p);
  };
  auto w = std::make_unique<Widget>(WidgetType::kRadioButton);
  w ->value(kValueFilter)
    ->visibility()
    ->addAttribute(Attribute::build<Index>("index", true, 0, 1))
    ->tooltip_position()
    ->tooltip_template()
    ->show_remote_box()
    ->show_automation_rect()
    ;
  // 2 frames
  w->fGraphics->fFilter = [](FilmStrip const &iFilmStrip) { return iFilmStrip.numFrames() == 2; };
  return w;
}

//------------------------------------------------------------------------
// Widget::sample_browse_group
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::sample_browse_group()
{
  auto w = std::make_unique<Widget>(WidgetType::kSampleBrowseGroup);
  w ->visibility()
    ->tooltip_position()
    ;
  w->setSize(kPatchBrowseGroupSize);
  w->fGraphics->fFilter = FilmStrip::bySizeFilter(kSampleBrowseGroupSize);
  return w;
}

//------------------------------------------------------------------------
// Widget::sample_drop_zone
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::sample_drop_zone()
{
  auto w = std::make_unique<Widget>(WidgetType::kSampleDropZone);
  w ->visibility()
    ->addAttribute(Attribute::build<UserSampleIndex>("user_sample_index", true, 0))
    ;
  return w;
}

//------------------------------------------------------------------------
// Widget::sequence_fader
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::sequence_fader()
{
  static const auto kValueFilter = [](const Property &p) {
    return (p.type() == kJBox_Boolean || p.type() == kJBox_Number) && kDocGuiOwnerFilter(p);
  };
  static const auto kValueSwitchFilter = [](const Property &p) { return p.isDiscrete() && kDocGuiOwnerFilter(p); };
  auto w = std::make_unique<Widget>(WidgetType::kSequenceFader);
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
std::unique_ptr<Widget> Widget::sequence_meter()
{
  static const auto kValueFilter = [](const Property &p) {
    return (p.type() == kJBox_Boolean || p.isDiscrete()) && p.owner() == mock::PropertyOwner::kRTOwner;
  };
  auto w = std::make_unique<Widget>(WidgetType::kSequenceMeter);
  w ->value(kValueFilter)
    ->visibility()
    ;
  return w;
}

//------------------------------------------------------------------------
// Widget::static_decoration
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::static_decoration()
{
  auto w = std::make_unique<Widget>(WidgetType::kStaticDecoration);
  w ->blend_mode()
    ->visibility()
    ;
  return w;
}

//------------------------------------------------------------------------
// Widget::step_button
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::step_button()
{
  static const auto kValueFilter = [](const Property &p) {
    return (p.type() == kJBox_Boolean || p.isDiscrete()) && kDocGuiOwnerFilter(p);
  };
  auto w = std::make_unique<Widget>(WidgetType::kStepButton);
  w ->value(kValueFilter)
    ->visibility()
    ->tooltip_position()
    ->tooltip_template()
    ->addAttribute(Attribute::build<Bool>("increasing", false, true))
    ->show_remote_box()
    ->show_automation_rect()
    ;
  // 2 frames
  w->fGraphics->fFilter = [](FilmStrip const &iFilmStrip) { return iFilmStrip.numFrames() == 2; };
  return w;
}

//------------------------------------------------------------------------
// Widget::toggle_button
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::toggle_button()
{
  static const auto kValueFilter = [](const Property &p) {
    return (p.type() == kJBox_Boolean || p.isDiscrete()) && kDocGuiOwnerFilter(p);
  };
  auto w = std::make_unique<Widget>(WidgetType::kToggleButton);
  w ->value(kValueFilter)
    ->visibility()
    ->tooltip_position()
    ->tooltip_template()
    ->show_remote_box()
    ->show_automation_rect()
    ;
  // 2 or 4 frames
  w->fGraphics->fFilter = [](FilmStrip const &iFilmStrip) { return iFilmStrip.numFrames() == 2 || iFilmStrip.numFrames() == 4; };
  return w;
}

//------------------------------------------------------------------------
// Widget::up_down_button
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::up_down_button()
{
  static const auto kValueFilter = [](const Property &p) { return p.isDiscrete() && kDocGuiOwnerFilter(p); };
  auto w = std::make_unique<Widget>(WidgetType::kUpDownButton);
  w ->value(kValueFilter)
    ->visibility()
    ->tooltip_position()
    ->tooltip_template()
    ->addAttribute(Attribute::build<Bool>("inverted", false, false))
    ->show_remote_box()
    ->show_automation_rect()
    ;
  // 3 frames
  w->fGraphics->fFilter = [](FilmStrip const &iFilmStrip) { return iFilmStrip.numFrames() == 3; };
  return w;
}

//------------------------------------------------------------------------
// Widget::value_display
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::value_display()
{
  static const auto kReadWriteValueFilter = [](const Property &p) {
    return (p.type() == kJBox_Boolean || p.type() == kJBox_Number) && kDocGuiOwnerFilter(p);
  };
  static const auto kValueSwitchFilter = [](const Property &p) { return p.isDiscrete() && kDocGuiOwnerFilter(p); };

  auto w = std::make_unique<Widget>(WidgetType::kValueDisplay);
  w ->value(kReadWriteValueFilter, kValueSwitchFilter)
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
std::unique_ptr<Widget> Widget::zero_snap_knob()
{
  static const auto kValueFilter = [](const Property &p) {
    return (p.type() == kJBox_Boolean || p.type() == kJBox_Number) && kDocGuiOwnerFilter(p);
  };
  static const auto kValueSwitchFilter = [](const Property &p) { return p.isDiscrete() && kDocGuiOwnerFilter(p); };
  auto w = std::make_unique<Widget>(WidgetType::kZeroSnapKnob);
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
std::unique_ptr<Widget> Widget::panel_decal()
{
  auto w = std::make_unique<Widget>(WidgetType::kPanelDecal);
  w->fGraphics->fFilter = [](FilmStrip const &iFilmStrip) { return iFilmStrip.numFrames() == 1; };
  return w;
}

//------------------------------------------------------------------------
// Widget::copy
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::copy() const
{
  return std::unique_ptr<Widget>(new Widget(*this, re::mock::fmt::printf("%s Copy", fName)));
}

//------------------------------------------------------------------------
// Widget::clone
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::clone() const
{
  return std::unique_ptr<Widget>(new Widget(*this));
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
// Widget::renderShowMenu
//------------------------------------------------------------------------
void Widget::renderShowMenu(AppContext &iCtx)
{
  if(canBeShown())
  {
    auto const &path = fVisibility->fSwitch.fValue;
    auto const &values = fVisibility->fValues.fValue;
    if(values.size() == 1)
    {
      if(ImGui::MenuItem(fmt::printf("Show [%s=%d]", path, values[0]).c_str()))
        iCtx.setPropertyValueAsInt(path, values[0]);
    }
    else
    {
      if(ImGui::BeginMenu(fmt::printf("Show [%s]", path).c_str()))
      {
        for(auto value: values)
        {
          if(ImGui::MenuItem(fmt::printf("value=%d", value).c_str()))
            iCtx.setPropertyValueAsInt(path, value);
        }
        ImGui::EndMenu();
      }
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

}

