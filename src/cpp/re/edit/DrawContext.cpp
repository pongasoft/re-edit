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

#include "DrawContext.h"
#include "ReGui.h"
#include "imgui_internal.h"

namespace re::edit {

//------------------------------------------------------------------------
// DrawContext::TextureItem
//------------------------------------------------------------------------
void DrawContext::TextureItem(Texture const *iTexture, ImVec2 const &iPosition, int iFrameNumber, const ImVec4& iBorderCol) const
{
  iTexture->Item(iPosition, fZoom, iFrameNumber, iBorderCol);
}

//------------------------------------------------------------------------
// DrawContext::drawTexture
//------------------------------------------------------------------------
void DrawContext::drawTexture(Texture const *iTexture, ImVec2 const &iPosition, int iFrameNumber, const ImVec4& iBorderCol) const
{
  iTexture->draw(iPosition, fZoom, iFrameNumber, iBorderCol);
}

//------------------------------------------------------------------------
// DrawContext::drawRect
//------------------------------------------------------------------------
void DrawContext::drawRect(ImVec2 const &iPosition, ImVec2 const &iSize, ImU32 iColor) const
{
  auto const cp = ImGui::GetCursorScreenPos();
  ImVec2 pos(cp + iPosition * fZoom);
  auto drawList = ImGui::GetWindowDrawList();
  drawList->AddRect(pos, {pos.x + (iSize.x * fZoom), pos.y + (iSize.y * fZoom)}, iColor);
}

//------------------------------------------------------------------------
// DrawContext::drawRectFilled
//------------------------------------------------------------------------
void DrawContext::drawRectFilled(ImVec2 const &iPosition,
                                 ImVec2 const &iSize,
                                 ImU32 iColor,
                                 float iRounding,
                                 ImDrawFlags iFlags) const
{
  auto const cp = ImGui::GetCursorScreenPos();
  ImVec2 pos(cp + iPosition * fZoom);
  auto drawList = ImGui::GetWindowDrawList();
  drawList->AddRectFilled(pos, {pos.x + (iSize.x * fZoom), pos.y + (iSize.y * fZoom)}, iColor, iRounding, iFlags);
}

//------------------------------------------------------------------------
// DrawContext::drawLine
//------------------------------------------------------------------------
void DrawContext::drawLine(const ImVec2& iP1, const ImVec2& iP2, ImU32 iColor, float iThickness) const
{
  auto const cp = ImGui::GetCursorScreenPos();
  auto drawList = ImGui::GetWindowDrawList();
  drawList->AddLine(cp + iP1 * fZoom, cp + iP2 * fZoom, iColor, iThickness);
}

}