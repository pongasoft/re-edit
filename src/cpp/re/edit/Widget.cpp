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
}

//------------------------------------------------------------------------
// Widget::Widget
//------------------------------------------------------------------------
Widget::Widget(Widget const &iOther) :
  fType(iOther.fType),
  fName(iOther.fName),
  fGraphics(iOther.fGraphics)
{
  for(auto &attribute: iOther.fAttributes)
  {
    auto newAttribute = attribute->clone();
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
  fName(std::move(iName)),
  fGraphics(iOther.fGraphics)
{
  for(auto &attribute: iOther.fAttributes)
  {
    auto newAttribute = attribute->clone();
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
  fGraphics.setPosition(iOther.getPosition() + ImVec2(iOther.fGraphics.getSize().x + 5,0));
  fSelected = true;
}

//------------------------------------------------------------------------
// Widget::draw
//------------------------------------------------------------------------
void Widget::draw(DrawContext &iCtx)
{
  if(isHidden())
    return;

  ImVec4 borderColor{};
  if(fSelected)
    borderColor = iCtx.getUserPreferences().fSelectedWidgetColor;
  else
  {
    if(iCtx.fShowBorder == DrawContext::ShowBorder::kWidget)
      borderColor = iCtx.getUserPreferences().fWidgetBorderColor;
  }

  if(fType == WidgetType::kCustomDisplay)
  {
    switch(iCtx.fShowCustomDisplay)
    {
      case EditContext::ShowCustomDisplay::kNone:
        fGraphics.drawBorder(iCtx, borderColor);
        break;
      case EditContext::ShowCustomDisplay::kMain:
        fGraphics.draw(iCtx, fFrameNumber, borderColor);
        break;
      case EditContext::ShowCustomDisplay::kBackgroundSD:
      case EditContext::ShowCustomDisplay::kBackgroundHD:
      {
        auto bgAttribute = findAttributeByNameAndType<Background>("background");
        bgAttribute->draw(iCtx, &fGraphics);
        fGraphics.drawBorder(iCtx, borderColor);
        break;
      }
      default:
        RE_EDIT_FAIL("not reached");
    }
  }
  else
    fGraphics.draw(iCtx, fFrameNumber, borderColor);

  if(fError)
    iCtx.drawRectFilled(fGraphics.fPosition, fGraphics.getSize(), iCtx.getUserPreferences().fWidgetErrorColor);
}


//------------------------------------------------------------------------
// Widget::editView
//------------------------------------------------------------------------
void Widget::editView(EditContext &iCtx)
{
  ImGui::PushID("Widget");

  ImGui::InputText("name", &fName);
  ReGui::InputInt("x", &fGraphics.fPosition.x, 1, 5);
  ReGui::InputInt("y", &fGraphics.fPosition.y, 1, 5);

  if(fGraphics.hasTexture())
  {
    auto numFrames = fGraphics.getTexture()->numFrames();
    if(numFrames > 2)
      ImGui::SliderInt("Frame", &fFrameNumber, 0, numFrames - 1);

    if(numFrames == 2)
    {
      bool v = fFrameNumber == 1;
      if(ImGui::Checkbox("Frame", &v))
        fFrameNumber = v ? 1 : 0;
    }
  }

  if(ImGui::TreeNode("Attributes"))
  {
    ImGui::PushID(fGraphics.fName.c_str());
    fGraphics.editView(iCtx,
                       fGraphics.fFilter,
                       [this]() {
                         fGraphics.reset();
                       },
                       [this, &iCtx](std::string const &iTextureKey) {
                         fGraphics.setTexture(iCtx.getTexture(iTextureKey));
                         fFrameNumber = 0;
                       },
                       [this](auto &s) {
                         fGraphics.fSize = s;
                         fGraphics.fTexture = nullptr;
                         fFrameNumber = 0;
                       }
    );
    ImGui::Indent();
    fGraphics.editHitBoundariesView(iCtx);
    ImGui::Unindent();
    ImGui::PopID();

    for(auto &w: fAttributes)
    {
      ImGui::PushID(w->fName.c_str());
      w->editView(iCtx);
      if(w->fError)
      {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1,0,0,1), "(?)");
        if(ImGui::IsItemHovered())
        {
          ImGui::BeginTooltip();
          ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
          ImGui::TextUnformatted(w->fError->c_str());
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
}

//------------------------------------------------------------------------
// Widget::hdgui2D
//------------------------------------------------------------------------
std::string Widget::hdgui2D() const
{
  if(isPanelDecal())
    return "";

  attribute_list_t atts{};

  fGraphics.hdgui2D(fName, atts);

  for(auto &w: fAttributes)
    w->hdgui2D(atts);

  std::vector<std::string> l{};
  std::transform(atts.begin(), atts.end(), std::back_inserter(l), [](auto &att) {
    return re::mock::fmt::printf("  %s = %s", att.fName, att.fValue);
  });
  return re::mock::fmt::printf("jbox.%s {\n%s\n}", toString(fType), re::mock::stl::join_to_string(l, ",\n"));
}

//------------------------------------------------------------------------
// Widget::value
//------------------------------------------------------------------------
Widget *Widget::value(Property::Filter iValueFilter, Property::Filter iValueSwitchFilter)
{
  return addAttribute(std::make_unique<Value>(std::move(iValueFilter), std::move(iValueSwitchFilter)));
}

//------------------------------------------------------------------------
// Widget::values
//------------------------------------------------------------------------
Widget *Widget::values(Property::Filter iValuesFilter)
{
  return addAttribute(std::make_unique<PropertyPathList>("values", std::move(iValuesFilter)));
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
  return addAttribute(Attribute::build<Bool>("show_remote_box", true));
}

//------------------------------------------------------------------------
// Widget::show_automation_rect
//------------------------------------------------------------------------
Widget *Widget::show_automation_rect()
{
  return addAttribute(Attribute::build<Bool>("show_automation_rect", true));
}

//------------------------------------------------------------------------
// Widget::orientation
//------------------------------------------------------------------------
Widget *Widget::orientation()
{
  static const std::vector<std::string> kOrientations = { "horizontal", "vertical"};
  return addAttribute(Attribute::build<StaticStringList>("orientation", "vertical", kOrientations));
}

//------------------------------------------------------------------------
// Widget::tooltip_position
//------------------------------------------------------------------------
Widget *Widget::tooltip_position()
{
  static const std::vector<std::string> tooltipPositions =
    { "bottom_left", "bottom", "bottom_right", "right", "top_right", "top", "top_left", "left", "no_tooltip"};
  return addAttribute(Attribute::build<StaticStringList>("tooltip_position", "", tooltipPositions));
}

//------------------------------------------------------------------------
// Widget::tooltip_template
//------------------------------------------------------------------------
Widget *Widget::tooltip_template()
{
  return addAttribute(Attribute::build<UIText>("tooltip_template", ""));
}

//------------------------------------------------------------------------
// Widget::blend_mode
//------------------------------------------------------------------------
Widget *Widget::blend_mode()
{
  static const std::vector<std::string> blend_modes =
    { "normal", "luminance"};
  return addAttribute(Attribute::build<StaticStringList>("blend_mode", "normal", blend_modes));
}

//------------------------------------------------------------------------
// Widget::computeDefaultWidgetName
//------------------------------------------------------------------------
void Widget::computeDefaultWidgetName()
{
  fName = re::mock::fmt::printf("%s_%ld", toString(fType), fWidgetIota++);
}

static constexpr auto kDocGuiOwnerFilter = [](const Property &p) {
  return p.owner() == mock::PropertyOwner::kDocOwner || p.owner() == mock::PropertyOwner::kGUIOwner;
};

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
  w->fGraphics.fFilter = FilmStrip::bySizeFilter(kAudioSocketSize);
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
  w->fGraphics.fFilter = FilmStrip::bySizeFilter(kAudioSocketSize);
  return w;
}

//------------------------------------------------------------------------
// Widget::custom_display
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::custom_display()
{
  // TODO can only bind to custom properties
  static const auto kValuesFilter = [](const Property &p) {
    return (p.type() == kJBox_Boolean || p.type() == kJBox_Number || p.type() == kJBox_String);
  };

  auto w = std::make_unique<Widget>(WidgetType::kCustomDisplay);

  w ->addAttribute(Attribute::build<Background>("background", ""))
    ->addAttribute(Attribute::build<Integer>("display_width_pixels", 0))
    ->addAttribute(Attribute::build<Integer>("display_height_pixels", 0))
    ->addAttribute(Attribute::build<String>("draw_function", ""))
    ->addAttribute(Attribute::build<String>("invalidate_function", ""))
    ->addAttribute(Attribute::build<String>("gesture_function", ""))
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
  w->fGraphics.fFilter = FilmStrip::bySizeFilter(kCVSocketSize);
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
  w->fGraphics.fFilter = FilmStrip::bySizeFilter(kCVSocketSize);
  return w;
}


//------------------------------------------------------------------------
// Widget::device_name
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::device_name()
{
  return std::make_unique<Widget>(WidgetType::kDeviceName);
}

//------------------------------------------------------------------------
// Widget::placeholder
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::placeholder()
{
  auto w = std::make_unique<Widget>(WidgetType::kPlaceholder);
  w->setSize(kPlaceholderSize);
  w->fGraphics.fFilter = FilmStrip::bySizeFilter(kPlaceholderSize);
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
    ->addAttribute(Attribute::build<Integer>("inset1", 0))
    ->addAttribute(Attribute::build<Integer>("inset2", 0))
    ->addAttribute(Attribute::build<Integer>("handle_size", 0))
    ->visibility()
    ->tooltip_position()
    ->tooltip_template()
    ->addAttribute(Attribute::build<Bool>("inverted", false))
    ->show_remote_box()
    ->show_automation_rect()
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
// Widget::panel_decal
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::panel_decal()
{
  auto w = std::make_unique<Widget>(WidgetType::kPanelDecal);
  w->fGraphics.fFilter = [](FilmStrip const &iFilmStrip) { return iFilmStrip.numFrames() == 1; };
  return w;
}

//------------------------------------------------------------------------
// Widget::widget
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::copy() const
{
  return std::unique_ptr<Widget>(new Widget(*this, re::mock::fmt::printf("%s Copy", fName)));
}

//------------------------------------------------------------------------
// Widget::widget
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::clone() const
{
  return std::unique_ptr<Widget>(new Widget(*this));
}

//------------------------------------------------------------------------
// Widget::computeIsHidden
//------------------------------------------------------------------------
void Widget::computeIsHidden(DrawContext &iCtx)
{
  fHidden = fVisibility ? fVisibility->isHidden(iCtx) : false;
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

}

