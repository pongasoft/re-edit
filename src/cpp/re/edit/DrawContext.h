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

#ifndef RE_EDIT_DRAWCONTEXT_H
#define RE_EDIT_DRAWCONTEXT_H

#include <imgui.h>
#include "EditContext.h"

namespace re::edit {

//static constexpr bool operator<(ImVec2 const &lhs, ImVec2 const &rhs)
//{
//  if(lhs.x < rhs.x)
//    return true;
//  if(rhs.x < lhs.x)
//    return false;
//  return lhs.y < rhs.y;
//}
//static constexpr bool operator>(ImVec2 const &lhs, ImVec2 const &rhs)               { return rhs < lhs; }
//static constexpr bool operator<=(ImVec2 const &lhs, ImVec2 const &rhs)              { return !(rhs < lhs); }
//static constexpr bool operator>=(ImVec2 const &lhs, ImVec2 const &rhs)              { return !(lhs < rhs); }

class DrawContext : public EditContext
{
public:
  DrawContext() = default;

public: // User preferences
  constexpr UserPreferences const &getUserPreferences() const { return *fUserPreferences; }
  constexpr UserPreferences &getUserPreferences() { return *fUserPreferences; }

public: // Texture
  void TextureItem(Texture const *iTexture, ImVec2 const &iPosition = {0,0}, int iFrameNumber = 0, const ImVec4& iBorderCol = ImVec4(0,0,0,0)) const;

  void drawTexture(Texture const *iTexture, ImVec2 const &iPosition = {0,0}, int iFrameNumber = 0, const ImVec4& iBorderCol = ImVec4(0,0,0,0)) const;
  void drawRect(ImVec2 const &iPosition, ImVec2 const &iSize, ImU32 iColor) const;
  void drawRectFilled(ImVec2 const &iPosition, ImVec2 const &iSize, ImU32 iColor, float iRounding = 0.0f, ImDrawFlags iFlags = 0) const;
  inline void drawRectFilled(ImVec2 const &iPosition, ImVec2 const &iSize, const ImVec4& iColor, float iRounding = 0.0f, ImDrawFlags iFlags = 0) const {
    drawRectFilled(iPosition, iSize, ImGui::GetColorU32(iColor), iRounding, iFlags);
  }
  void drawRect(ImVec2 const &iPosition, ImVec2 const &iSize, const ImVec4& iColor) const { drawRect(iPosition, iSize, ImGui::GetColorU32(iColor)); }
  void drawLine(const ImVec2& iP1, const ImVec2& iP2, ImU32 iColor, float iThickness = 1.0f) const;
  inline void drawLine(const ImVec2& iP1, const ImVec2& iP2, const ImVec4& iColor, float iThickness = 1.0f) const { drawLine(iP1, iP2, ImGui::GetColorU32(iColor), iThickness); }

public:
  float fZoom{0.20f};
  bool fShowWidgetBorder{};
};

}

#endif //RE_EDIT_DRAWCONTEXT_H