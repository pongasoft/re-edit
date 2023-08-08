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
#include "Utils.h"
#include <raylib.h>

namespace re::edit::ReGui {

//------------------------------------------------------------------------
// Canvas::begin
//------------------------------------------------------------------------
void Canvas::begin(screen_pos_t const &iCanvasPos,
                   screen_size_t const &iCanvasSize,
                   canvas_size_t const &iContentSize,
                   ImVec2 const &iRenderScale,
                   Zoom iZoom,
                   ImVec4 const &iBackgroundColor)
{
  fCanvasPos = iCanvasPos;
  fCanvasSize = {std::max(1.0f, iCanvasSize.x), std::max(1.0f, iCanvasSize.y)};
  fContentSize = iContentSize;
  fRenderScale = iRenderScale;
  updateZoom(iZoom, fFocus);
  DrawRectangle(0, 0,
                static_cast<int>(fCanvasSize.x * fRenderScale.x),
                static_cast<int>(fCanvasSize.y * fRenderScale.y),
                ReGui::GetRLColor(iBackgroundColor));
}

//------------------------------------------------------------------------
// Canvas::begin
//------------------------------------------------------------------------
void Canvas::begin(ImVec2 const &iContentSize, ImVec2 const &iRenderScale, Zoom iZoom, ImVec4 const &iBackgroundColor)
{
  begin(ImGui::GetCursorScreenPos(), ImGui::GetContentRegionAvail(), iContentSize, iRenderScale, iZoom, iBackgroundColor);
}

//------------------------------------------------------------------------
// Canvas::end
//------------------------------------------------------------------------
Canvas::Zoom Canvas::end()
{
  fIsActive = false;
  fIsHovered = false;
  return fZoom;
}

//------------------------------------------------------------------------
// Canvas::updateZoom
//------------------------------------------------------------------------
void Canvas::updateZoom(Zoom iZoom, std::optional<canvas_pos_t> const &iFocus)
{
  if(iZoom.fitContent())
  {
    fZoom = iZoom.update(computeZoomToFit(), true);
    centerContent();
  }
  else
  {
    if(iZoom.value() != fZoom.value())
    {
      // if focus is provided we use it, otherwise we use the center of the canvas as a reference point
      auto focus = iFocus ? *iFocus : computeDefaultFocus();
      fOffset = fOffset - focus * (iZoom.value() - fZoom.value());
    }
    fZoom = iZoom;
  }
}

//------------------------------------------------------------------------
// Canvas::centerContent
//------------------------------------------------------------------------
void Canvas::centerContent()
{
  fOffset = (fCanvasSize - (fContentSize * fZoom.value())) / 2.0f;
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
void Canvas::makeResponsive(ImGuiMouseButton flags)
{
  ImGui::PushID(this);
  auto cp = ImGui::GetCursorScreenPos();
  ImGui::SetCursorScreenPos(fCanvasPos);
  ImGui::InvisibleButton("canvas", fCanvasSize, flags);
  fIsActive = ImGui::IsItemActive();
  fIsHovered = ImGui::IsItemHovered();
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
                        ImU32 iTextureColor,
                        texture::FX const &iTextureFX) const
{
  iTexture->draw(toRenderScreenPos(iPos),
                 (iTextureFX.hasSizeOverride() ? *iTextureFX.fSizeOverride : iTexture->frameSize()) * fZoom.value() * fRenderScale,
                 iFrameNumber,
                 iBorderColor,
                 iTextureColor,
                 iTextureFX);
}

//------------------------------------------------------------------------
// Canvas::addRectFilled
//------------------------------------------------------------------------
void Canvas::addRectFilled(Canvas::canvas_pos_t const &iPos, canvas_size_t const &iSize, ImU32 iCol) const
{
  ImDrawList* dl = ImGui::GetWindowDrawList();
  auto min = toScreenPos(iPos);
  dl->AddRectFilled(min, min + iSize * fZoom.value(), iCol);
}

//------------------------------------------------------------------------
// Canvas::addRect
//------------------------------------------------------------------------
void Canvas::addRect(Canvas::canvas_pos_t const &iPos, canvas_size_t const &iSize, ImU32 iCol) const
{
  ImDrawList* dl = ImGui::GetWindowDrawList();
  auto min = toScreenPos(iPos);
  dl->AddRect(min, min + iSize * fZoom.value(), iCol);
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
  fZoom = fZoom.update(fZoom.value(), false);
}

//------------------------------------------------------------------------
// Canvas::zoomBy
//------------------------------------------------------------------------
void Canvas::zoomBy(float iPercent, std::optional<canvas_pos_t> iFocus)
{
  auto zoom = fZoom.update(fZoom.value() * iPercent, false);
  updateZoom(zoom, iFocus ? iFocus : fFocus);
}

//------------------------------------------------------------------------
// Canvas::zoomToFit
//------------------------------------------------------------------------
void Canvas::zoomToFit()
{
  fZoom = fZoom.update(computeZoomToFit(), true);
  centerContent();
}

//------------------------------------------------------------------------
// Canvas::computeDefaultFocus
//------------------------------------------------------------------------
Canvas::canvas_pos_t Canvas::computeDefaultFocus() const
{
  ReGui::Rect content {{0,0}, fContentSize};
  ReGui::Rect canvas {fromScreenPos(fCanvasPos), fromScreenPos(fCanvasPos + fCanvasSize)};
  content.ClipWithFull(canvas);
  return content.GetCenter();
}

//------------------------------------------------------------------------
// Canvas::Zoom::Zoom
//------------------------------------------------------------------------
Canvas::Zoom::Zoom(float iValue, bool iFitContent, float iMinValue, float iMaxValue)
: fValue{Utils::clamp(iValue, iMinValue, iMaxValue)},
  fFitContent{iFitContent},
  fMinValue{iMinValue},
  fMaxValue{iMaxValue}
{
}

}