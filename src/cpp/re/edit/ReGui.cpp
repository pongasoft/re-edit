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

#include "ReGui.h"
#include "Errors.h"
#include "imgui_internal.h"

namespace re::edit::ReGui {


//------------------------------------------------------------------------
// ReGui::Box
//------------------------------------------------------------------------
void Box(Modifier const &iModifier, std::function<void()> const &iBoxContent, ImDrawListSplitter *iSplitter)
{
  // Implementation note: this is made static because of this https://github.com/ocornut/imgui/issues/5944#issuecomment-1333930454
  // "using a new Splitter every frame is prohibitively costly".
  static ImDrawListSplitter kSplitter{};

  auto hasBackground = !ColorIsTransparent(iModifier.fBackgroundColor);
  auto hasBorder = !ColorIsTransparent(iModifier.fBorderColor);

  ImDrawList *drawList{};

  if(hasBackground || hasBorder)
  {
    drawList = ImGui::GetWindowDrawList();
    if(!iSplitter)
      iSplitter = &kSplitter;

    // split draw list in 2
    iSplitter->Split(drawList, 2);

    // first we draw in channel 1 to render iBoxContent (will be on top)
    iSplitter->SetCurrentChannel(drawList, 1);
  }

  auto min = ImGui::GetCursorScreenPos();
  // account for padding left/top
  ImGui::SetCursorScreenPos(min + ImVec2{iModifier.fPadding.w, iModifier.fPadding.x});

  ImGui::BeginGroup();
  {
    iBoxContent();
    ImGui::EndGroup();
  }

  // account for padding right/bottom
  auto max = ImGui::GetItemRectMax() + ImVec2{iModifier.fPadding.y, iModifier.fPadding.z};

  if(drawList)
  {
    // second we draw the rectangle and border in channel 0 (will be below)
    iSplitter->SetCurrentChannel(drawList, 0);

    // draw the background
    if(hasBackground)
      drawList->AddRectFilled(min, max, iModifier.fBackgroundColor);

    // draw the border
    if(hasBorder)
      drawList->AddRect(min, max, iModifier.fBorderColor);

    // merge the 2 draw lists
    iSplitter->Merge(drawList);
  }

  // reposition the cursor (top left) and render a "dummy" box of the correct size so that it occupies
  // the proper amount of space
  ImGui::SetCursorScreenPos(min);
  ImGui::Dummy(max - min);
}

//------------------------------------------------------------------------
// ReGui::CenteredSeparator
// copied from https://github.com/ocornut/imgui/issues/1643#issuecomment-369376479
//------------------------------------------------------------------------
void CenteredSeparator(float width)
{
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
    return;
  ImGuiContext& g = *GImGui;
  /*
  // Commented out because it is not tested, but it should work, but it won't be centered
  ImGuiWindowFlags flags = 0;
  if ((flags & (ImGuiSeparatorFlags_Horizontal | ImGuiSeparatorFlags_Vertical)) == 0)
      flags |= (window->DC.LayoutType == ImGuiLayoutType_Horizontal) ? ImGuiSeparatorFlags_Vertical : ImGuiSeparatorFlags_Horizontal;
  IM_ASSERT(ImIsPowerOfTwo((int)(flags & (ImGuiSeparatorFlags_Horizontal | ImGuiSeparatorFlags_Vertical))));   // Check that only 1 option is selected
  if (flags & ImGuiSeparatorFlags_Vertical)
  {
      VerticalSeparator();
      return;
  }
  */

  // Horizontal Separator
  float x1, x2;
  if (window->DC.CurrentColumns == nullptr && (width == 0))
  {
    // Span whole window
    ///x1 = window->Pos.x; // This fails with SameLine(); CenteredSeparator();
    // Nah, we have to detect if we have a sameline in a different way
    x1 = window->DC.CursorPos.x;
    x2 = x1 + window->Size.x;
  }
  else
  {
    // Start at the cursor
    x1 = window->DC.CursorPos.x;
    if (width != 0) {
      x2 = x1 + width;
    }
    else
    {
      x2 = window->ClipRect.Max.x;
      // Pad right side of columns (except the last one)
      if (window->DC.CurrentColumns && (window->DC.CurrentColumns->Current < window->DC.CurrentColumns->Count - 1))
        x2 -= g.Style.ItemSpacing.x;
    }
  }
  float y1 = window->DC.CursorPos.y + std::floorf(window->DC.CurrLineSize.y / 2.0f);
  float y2 = y1 + 1.0f;

  window->DC.CursorPos.x += width; //+ g.Style.ItemSpacing.x;

  // TODO YP: unclear what this should be (does not compile)
//  if (!window->DC.GroupStack.empty())
//    x1 += window->DC.IndentX;

  const ImRect bb(ImVec2(x1, y1), ImVec2(x2, y2));
  ImGui::ItemSize(ImVec2(0.0f, 0.0f)); // NB: we don't provide our width so that it doesn't get feed back into AutoFit, we don't provide height to not alter layout.
  if(!ImGui::ItemAdd(bb, 0))
  {
    return;
  }

  window->DrawList->AddLine(bb.Min, ImVec2(bb.Max.x, bb.Min.y), ImGui::GetColorU32(ImGuiCol_Border));

}


//------------------------------------------------------------------------
// ReGui::PreSeparator
// Create a centered separator which can be immediately followed by an item
// copied from https://github.com/ocornut/imgui/issues/1643#issuecomment-369376479
//------------------------------------------------------------------------
void PreSeparator(float width)
{
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  if(window->DC.CurrLineSize.y == 0)
    window->DC.CurrLineSize.y = ImGui::GetTextLineHeight();
  CenteredSeparator(width);
  ImGui::SameLine();
}

//------------------------------------------------------------------------
// ReGui::SameLineSeparator
// Create a centered separator right after the current item.
// Eg.:
// ```cpp
//    ImGui::PreSeparator(10);
//    ImGui::Text("Section VI");
//    ImGui::SameLineSeparator();
// ```
// copied from https://github.com/ocornut/imgui/issues/1643#issuecomment-369376479
//------------------------------------------------------------------------
inline void SameLineSeparator(float width = 0)
{
  ImGui::SameLine();
  CenteredSeparator(width);
}

//------------------------------------------------------------------------
// ReGui::TextSeparator
//------------------------------------------------------------------------
void TextSeparator(char const *text, float pre_width)
{
  ImGuiContext& g = *GImGui;

  PreSeparator(pre_width);
  if(g.LogEnabled)
    ImGui::LogSetNextTextDecoration("---", "---");
  ImGui::Text(text);
  SameLineSeparator();

}

//------------------------------------------------------------------------
// ReGui::MultiLineText
//------------------------------------------------------------------------
void MultiLineText(std::string const &iText)
{
  std::istringstream stream(iText);
  std::string line;
  while(std::getline(stream, line, '\n'))
  {
    ImGui::TextUnformatted(line.c_str());
  }
}

//------------------------------------------------------------------------
// Window::Lifecycle::~Lifecycle
//------------------------------------------------------------------------
Window::Lifecycle::~Lifecycle()
{
  if(fEndRequired)
  {
    if(std::uncaught_exceptions() == 0)
      ImGui::End();
  }
}

//------------------------------------------------------------------------
// Window::begin
//------------------------------------------------------------------------
Window::Lifecycle Window::begin(ImGuiWindowFlags flags)
{
  if(fVisible)
  {
    if(fSizeToFitRequested > 0)
    {
      flags |= ImGuiWindowFlags_AlwaysAutoResize;
      fSizeToFitRequested--;
    }
    return Lifecycle{ImGui::Begin(fName.c_str(), fDisableClosingWidget ? nullptr : &fVisible, fFlags | flags)};
  }
  else
    return {};
}

//------------------------------------------------------------------------
// Window::iName
//------------------------------------------------------------------------
void Window::setName(std::string const &iName)
{
  std::string key = fKey;
  fName = key == iName ? key : fmt::printf("%s###%s", iName, fKey);
}

}