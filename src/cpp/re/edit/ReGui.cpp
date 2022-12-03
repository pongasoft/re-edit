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