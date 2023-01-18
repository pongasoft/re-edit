/*
 * Copyright (c) 2023 pongasoft
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

#include "Canvas.h"
#include "Texture.h"

namespace re::edit::ReGui {

//------------------------------------------------------------------------
// Canvas::begin
//------------------------------------------------------------------------
void Canvas::begin(screen_pos_t const &iCanvasPos,
                   screen_size_t const &iCanvasSize,
                   canvas_size_t const &iContentSize,
                   Zoom iZoom)
{
  fCanvasPos = iCanvasPos;
  fCanvasSize = {std::max(1.0f, iCanvasSize.x), std::max(1.0f, iCanvasSize.y)};
  fContentSize = iContentSize;
  setZoom(iZoom);
}

//------------------------------------------------------------------------
// Canvas::begin
//------------------------------------------------------------------------
void Canvas::begin(ImVec2 const &iContentSize, Zoom iZoom)
{
  begin(ImGui::GetCursorScreenPos(), ImGui::GetContentRegionAvail(), iContentSize, iZoom);
}

//------------------------------------------------------------------------
// Canvas::end
//------------------------------------------------------------------------
void Canvas::end()
{
  // empty for now
}

//------------------------------------------------------------------------
// Canvas::setZoom
//------------------------------------------------------------------------
void Canvas::setZoom(Zoom iZoom)
{
  if(iZoom.fFitContent)
  {
    fZoom.fValue = computeZoomToFit();
    fZoom.fFitContent = true;
    centerContent();
  }
  else
  {
    auto newZoom = std::max(0.1f, iZoom.fValue);
    if(newZoom != fZoom.fValue)
    {
      // if focus is provided we use it, otherwise we use the center of the canvas as a reference point
      auto focus = fFocus ? *fFocus : fromScreenPos(fCanvasPos + fCanvasSize / 2.0f);
      fOffset = fOffset - focus * (newZoom - fZoom.fValue);
    }
    fZoom.fValue = newZoom;
    fZoom.fFitContent = false;
  }
}

//------------------------------------------------------------------------
// Canvas::centerContent
//------------------------------------------------------------------------
void Canvas::centerContent()
{
  fOffset = (fCanvasSize - (fContentSize * fZoom.fValue)) / 2.0f;
}

//------------------------------------------------------------------------
// Canvas::computeZoomToFit
//------------------------------------------------------------------------
float Canvas::computeZoomToFit() const
{
  if(fContentSize.x > 0 && fContentSize.y > 0)
  {
    auto factor = fCanvasSize / fContentSize;
    return std::min(factor.x, factor.y);
  }
  else
    return 1.0f;
}

//------------------------------------------------------------------------
// Canvas::makeResponsive
//------------------------------------------------------------------------
void Canvas::makeResponsive(ImGuiMouseButton flags) const
{
  ImGui::PushID(this);
  auto cp = ImGui::GetCursorScreenPos();
  ImGui::SetCursorScreenPos(fCanvasPos);
  ImGui::InvisibleButton("canvas", fCanvasSize, flags);
  ImGui::SetCursorScreenPos(cp);
  ImGui::PopID();
}

//------------------------------------------------------------------------
// Canvas::addTexture
//------------------------------------------------------------------------
void Canvas::addTexture(Texture const *iTexture,
                        Canvas::canvas_pos_t const &iPos,
                        int iFrameNumber,
                        ImU32 iBorderColor,
                        ImU32 iTextureColor) const
{
  iTexture->draw(toScreenPos(iPos), iTexture->frameSize() * fZoom.fValue, iFrameNumber, iBorderColor, iTextureColor);
}

//------------------------------------------------------------------------
// Canvas::addScaledTexture
//------------------------------------------------------------------------
void Canvas::addScaledTexture(Texture const *iTexture,
                              ImVec2 const &iScale,
                              Canvas::canvas_pos_t const &iPos,
                              int iFrameNumber,
                              ImU32 iBorderColor,
                              ImU32 iTextureColor) const
{
  iTexture->draw(toScreenPos(iPos), iTexture->frameSize() * fZoom.fValue * iScale, iFrameNumber, iBorderColor, iTextureColor);
}

//------------------------------------------------------------------------
// Canvas::addRectFilled
//------------------------------------------------------------------------
void Canvas::addRectFilled(Canvas::canvas_pos_t const &iPos, canvas_size_t const &iSize, ImU32 iCol) const
{
  ImDrawList* dl = ImGui::GetWindowDrawList();
  auto min = toScreenPos(iPos);
  dl->AddRectFilled(min, min + iSize * fZoom.fValue, iCol);
}

//------------------------------------------------------------------------
// Canvas::addRect
//------------------------------------------------------------------------
void Canvas::addRect(Canvas::canvas_pos_t const &iPos, canvas_size_t const &iSize, ImU32 iCol) const
{
  ImDrawList* dl = ImGui::GetWindowDrawList();
  auto min = toScreenPos(iPos);
  dl->AddRect(min, min + iSize * fZoom.fValue, iCol);
}

//------------------------------------------------------------------------
// Canvas::addLine
//------------------------------------------------------------------------
void Canvas::addLine(Canvas::canvas_pos_t const &iP1, Canvas::canvas_pos_t const &iP2, ImU32 iColor, float iThickness) const
{
  ImDrawList* dl = ImGui::GetWindowDrawList();
  dl->AddLine(toScreenPos(iP1), toScreenPos(iP2), iColor, iThickness);
}

//------------------------------------------------------------------------
// Canvas::addVerticalLine
//------------------------------------------------------------------------
void Canvas::addVerticalLine(canvas_pos_t const &p, ImU32 iColor, float iThickness) const
{
  auto p1 = toScreenPos(p);
  p1.y = fCanvasPos.y;
  auto p2 = p1;
  p2.y = fCanvasPos.y + fCanvasSize.y;
  ImDrawList* dl = ImGui::GetWindowDrawList();
  dl->AddLine(p1, p2, iColor, iThickness);
}

//------------------------------------------------------------------------
// Canvas::addHorizontalLine
//------------------------------------------------------------------------
void Canvas::addHorizontalLine(canvas_pos_t const &p, ImU32 iColor, float iThickness) const
{
  auto p1 = toScreenPos(p);
  p1.x = fCanvasPos.x;
  auto p2 = p1;
  p2.x = fCanvasPos.x + fCanvasSize.x;
  ImDrawList* dl = ImGui::GetWindowDrawList();
  dl->AddLine(p1, p2, iColor, iThickness);
}

//------------------------------------------------------------------------
// Canvas::moveByDeltaScreenPos
//------------------------------------------------------------------------
void Canvas::moveByDeltaScreenPos(Canvas::screen_pos_t const &iDelta)
{
  fOffset += iDelta;
  fZoom.fFitContent = false;
}



}