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

namespace re::edit {

using namespace widget;
using namespace widget::attribute;

long Widget::fWidgetIota = 1;

//------------------------------------------------------------------------
// Widget::Widget
//------------------------------------------------------------------------
Widget::Widget(std::string iHDGui2DName) : fType{std::move(iHDGui2DName)}
{
  computeDefaultWidgetName();
}

//------------------------------------------------------------------------
// Widget::draw
//------------------------------------------------------------------------
void Widget::draw(DrawContext &iCtx)
{
  if(fHidden)
    return;

  ImVec4 borderColor{};
  if(fSelected)
    borderColor = iCtx.getUserPreferences().fSelectedWidgetColor;
  else
  {
    if(iCtx.fShowWidgetBorder)
      borderColor = iCtx.getUserPreferences().fWidgetBorderColor;
  }

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
    fGraphics.editView(iCtx.getTextureKeys(),
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

  if(ImGui::TreeNode("hdgui2D"))
  {
    auto size = ImGui::GetWindowSize();
    ImGui::PushTextWrapPos(size.x);
    ImGui::TextUnformatted(hdgui2D().c_str());
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
  attribute_list_t atts{};

  fGraphics.hdgui2D(fName, atts);

  for(auto &w: fAttributes)
    w->hdgui2D(atts);

  std::vector<std::string> l{};
  std::transform(atts.begin(), atts.end(), std::back_inserter(l), [](auto &att) {
    return re::mock::fmt::printf("  %s = %s", att.fName, att.fValue);
  });
  return re::mock::fmt::printf("jbox.%s {\n%s\n}", fType, re::mock::stl::join_to_string(l, ",\n"));
}

//------------------------------------------------------------------------
// Widget::value
//------------------------------------------------------------------------
Widget *Widget::value()
{
  return addAttribute(std::make_unique<Value>());
}

//------------------------------------------------------------------------
// Widget::visibility
//------------------------------------------------------------------------
Widget *Widget::visibility()
{
  return addAttribute(std::make_unique<Visibility>());
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
// Widget::computeDefaultWidgetName
//------------------------------------------------------------------------
void Widget::computeDefaultWidgetName()
{
  fName = re::mock::fmt::printf("%s_%ld", fType, fWidgetIota++);
}

//------------------------------------------------------------------------
// Widget::analog_knob
//------------------------------------------------------------------------
std::unique_ptr<Widget> Widget::analog_knob()
{
  auto w = std::make_unique<Widget>("analog_knob");
  w ->value()
    ->visibility()
    ->tooltip_position()
    ->tooltip_template()
    ->show_remote_box()
    ->show_automation_rect()
    ;
  return w;
}

}

