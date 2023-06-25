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

#ifndef RE_EDIT_CANVAS_H
#define RE_EDIT_CANVAS_H

#include "ReGui.h"
#include <optional>

namespace re::edit {
  class Texture;
};

namespace re::edit::ReGui {

class Canvas
{
public:
  struct Zoom
  {
    Zoom(float iValue, bool iFitContent, float iMinValue, float iMaxValue);

    constexpr float value() const {return fValue; }
    constexpr bool fitContent() const {return fFitContent; }

    inline Zoom update(float iValue, bool iFitContent) const { return Zoom{iValue, iFitContent, fMinValue, fMaxValue}; }

  private:
    float fValue;
    bool fFitContent;
    float fMinValue;
    float fMaxValue;
  };

public:
  using screen_pos_t  = ImVec2;
  using screen_size_t = ImVec2;
  using canvas_pos_t  = ImVec2;
  using canvas_size_t = ImVec2;

public:
  void begin(ImVec2 const &iContentSize, Zoom iZoom);

  void begin(screen_pos_t const &iCanvasPos,
             screen_size_t const &iCanvasSize,
             canvas_size_t const &iContentSize,
             Zoom iZoom);


  Zoom end();

  void centerContent();

  void addTexture(Texture const *iTexture,
                  canvas_pos_t const &iPos = {0,0},
                  int iFrameNumber = 0,
                  ImU32 iBorderColor = ReGui::kTransparentColorU32,
                  ImU32 iTextureColor = ReGui::kWhiteColorU32) const;

  void addResizedTexture(Texture const *iTexture,
                         ImVec2 const &iTextureSize,
                         canvas_pos_t const &iPos = {0,0},
                         int iFrameNumber = 0,
                         ImU32 iBorderColor = ReGui::kTransparentColorU32,
                         ImU32 iTextureColor = ReGui::kWhiteColorU32) const;

  void addScaledTexture(Texture const *iTexture,
                        ImVec2 const &iScale,
                        canvas_pos_t const &iPos = {0,0},
                        int iFrameNumber = 0,
                        ImU32 iBorderColor = ReGui::kTransparentColorU32,
                        ImU32 iTextureColor = ReGui::kWhiteColorU32) const;

  void addRectFilled(canvas_pos_t const &iPos, canvas_size_t const &iSize, ImU32 iCol = ReGui::kWhiteColorU32) const;

  void addRect(canvas_pos_t const &iPos, canvas_size_t const &iSize, ImU32 iCol = ReGui::kWhiteColorU32) const;

  void addLine(canvas_pos_t const &iP1, canvas_pos_t const &iP2, ImU32 iColor, float iThickness = 1.0f) const;

  void addVerticalLine(canvas_pos_t const &p, ImU32 iColor, float iThickness = 1.0f) const;
  void addHorizontalLine(canvas_pos_t const &p, ImU32 iColor, float iThickness = 1.0f) const;

  void makeResponsive(ImGuiMouseButton flags = 0);

  bool isActive() const { return fIsActive; }
  bool isHovered() const { return fIsHovered; }
  bool canReceiveInput() const { return isActive() || !ImGui::IsAnyItemActive(); }

  inline canvas_pos_t getCanvasMousePos() const { return fromScreenPos(ImGui::GetMousePos()); }

  void setFocus(std::optional<canvas_pos_t> const &iFocus) { fFocus = iFocus; }
  std::optional<canvas_pos_t> getFocus() const { return fFocus; }

  void moveByDeltaScreenPos(screen_pos_t const &iDelta);
  inline void moveByDeltaCanvasPos(canvas_pos_t const &iDelta) { moveByDeltaScreenPos(iDelta * fZoom.value()); }
  void zoomBy(float iPercent, std::optional<canvas_pos_t> iFocus = std::nullopt);
  void zoomToFit();

protected:
  constexpr screen_pos_t toScreenPos(canvas_pos_t const &iPos) const { return fCanvasPos + fOffset + iPos * fZoom.value(); }
  constexpr canvas_pos_t fromScreenPos(screen_pos_t const &iPos) const { return (iPos - fCanvasPos - fOffset) / fZoom.value(); }

  void updateZoom(Zoom iZoom, std::optional<canvas_pos_t> const &iFocus);
  float computeZoomToFit() const;
  canvas_pos_t computeDefaultFocus() const;

private:
  screen_pos_t fCanvasPos{};
  screen_size_t fCanvasSize{};
  canvas_size_t fContentSize{}; // original / NOT zoomed
  std::optional<canvas_pos_t> fFocus{};
  Zoom fZoom{1.0f, true, 1.0f, 1.0f};

  bool fIsActive{};
  bool fIsHovered{};

  screen_pos_t fOffset{};
};

}



#endif //RE_EDIT_CANVAS_H