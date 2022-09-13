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

#ifndef RE_EDIT_RE_GUI_H
#define RE_EDIT_RE_GUI_H

#include <imgui.h>
#include <cmath>
#include "Color.h"
#include "Constants.h"
#include <IconsFAReEdit.h>

static constexpr ImVec2 operator*(const ImVec2& lhs, const float rhs)              { return {lhs.x * rhs, lhs.y * rhs}; }
static constexpr ImVec2 operator/(const ImVec2& lhs, const float rhs)              { return {lhs.x / rhs, lhs.y / rhs}; }
static constexpr ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs)            { return {lhs.x + rhs.x, lhs.y + rhs.y}; }
static constexpr ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs)            { return {lhs.x - rhs.x, lhs.y - rhs.y}; }
static constexpr ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs)            { return {lhs.x * rhs.x, lhs.y * rhs.y}; }
static constexpr ImVec2 operator/(const ImVec2& lhs, const ImVec2& rhs)            { return {lhs.x / rhs.x, lhs.y / rhs.y}; }
static constexpr ImVec2& operator*=(ImVec2& lhs, const float rhs)                  { lhs.x *= rhs; lhs.y *= rhs; return lhs; }
static constexpr ImVec2& operator/=(ImVec2& lhs, const float rhs)                  { lhs.x /= rhs; lhs.y /= rhs; return lhs; }
static constexpr ImVec2& operator+=(ImVec2& lhs, const ImVec2& rhs)                { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
static constexpr ImVec2& operator-=(ImVec2& lhs, const ImVec2& rhs)                { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
static constexpr ImVec2& operator*=(ImVec2& lhs, const ImVec2& rhs)                { lhs.x *= rhs.x; lhs.y *= rhs.y; return lhs; }
static constexpr ImVec2& operator/=(ImVec2& lhs, const ImVec2& rhs)                { lhs.x /= rhs.x; lhs.y /= rhs.y; return lhs; }
static constexpr ImVec4 operator+(const ImVec4& lhs, const ImVec4& rhs)            { return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w}; }
static constexpr ImVec4 operator-(const ImVec4& lhs, const ImVec4& rhs)            { return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w}; }
static constexpr ImVec4 operator*(const ImVec4& lhs, const ImVec4& rhs)            { return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w}; }

static constexpr bool operator==(const ImVec2& lhs, const ImVec2& rhs)
{
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

static constexpr bool operator!=(const ImVec2& lhs, const ImVec2& rhs)
{
  return !(lhs == rhs);
}

namespace re::edit::ReGui {

//------------------------------------------------------------------------
// ReGui::InputInt
// Handle float <-> int conversion
//------------------------------------------------------------------------
inline bool InputInt(const char* label, float* v, int step = 1, int step_fast = 100, ImGuiInputTextFlags flags = 0)
{
  auto i = static_cast<int>(std::round(*v));
  if(ImGui::InputInt(label, &i, step, step_fast, flags))
  {
    *v = static_cast<float>(i);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------
// ReGui::SliderInt2
// Handle float <-> int conversion
//------------------------------------------------------------------------
inline bool SliderInt2(const char* label, float *v[2], int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0)
{
  int ar[] = { static_cast<int>(std::round(*v[0])), static_cast<int>(std::round(*v[1]))};

  if(ImGui::SliderInt2(label, ar, v_min, v_max, format, flags))
  {
    *v[0] = static_cast<float>(ar[0]);
    *v[1] = static_cast<float>(ar[1]);
    return true;
  }

  return false;
}

//------------------------------------------------------------------------
// ReGui::ToggleButton
// Behaves like a checkbox but with a shape of a toggle
//------------------------------------------------------------------------
inline bool ToggleButton(char const *iFalseLabel, char const *iTrueLabel, bool* v, const ImVec2& size = ImVec2(0, 0))
{
  if(ImGui::Button(*v ? iTrueLabel : iFalseLabel, size))
  {
    *v = !*v;
    return true;
  }
  return false;
}

//------------------------------------------------------------------------
// ReGui::RadioButton
// With a better api
//------------------------------------------------------------------------
template<typename T>
inline bool RadioButton(char const *iLabel, T *ioCurrentValue, T iSelectedValue)
{
  if(ImGui::RadioButton(iLabel, *ioCurrentValue == iSelectedValue))
  {
    *ioCurrentValue = iSelectedValue;
    return true;
  }
  return false;
}

//------------------------------------------------------------------------
// ReGui::ColorEdit
//------------------------------------------------------------------------
inline bool ColorEdit(const char* label, JboxColor3 *ioColor, ImGuiColorEditFlags flags = 0)
{
  float colors[]{toFloatColor(ioColor->fRed), toFloatColor(ioColor->fGreen), toFloatColor(ioColor->fBlue)};
  if(ImGui::ColorEdit3(label, colors, flags))
  {
    ioColor->fRed = toIntColor(colors[0]);
    ioColor->fGreen = toIntColor(colors[1]);
    ioColor->fBlue = toIntColor(colors[2]);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------
// ReGui::MenuToggle
//------------------------------------------------------------------------
inline bool MenuToggle(char const *iFalseLabel, char const *iTrueLabel, bool *v)
{
  if(ImGui::MenuItem(*v ? iTrueLabel : iFalseLabel))
  {
    *v = !*v;
    return true;
  }
  return false;
}

constexpr auto kFaButtonSize = ImVec2{21.0f, 0.0f};

constexpr auto kHiddenWidgetIcon = fa::kEyeSlash;
constexpr auto kErrorIcon = fa::kTriangleExclamation;
constexpr auto kResetIcon = fa::kCircleX;
constexpr auto kMenuIcon = fa::kBars;

#define ReGui_Prefix(prefix, str) prefix " " str
#define ReGui_Icon_Reset ICON_FA_CircleX
#define ReGui_Icon_Watch ICON_FA_MagnifyingGlass
#define ReGui_Icon_Edit ICON_FA_Pencil
#define ReGui_Icon_Undo ICON_FA_ArrowRotateLeft
#define ReGui_Icon_Redo ICON_FA_ArrowRotateRight
#define ReGui_Icon_Save ICON_FA_FloppyDisk

//------------------------------------------------------------------------
// ReGui::ResetButton
//------------------------------------------------------------------------
inline bool ResetButton(const ImVec2& iSize = kFaButtonSize)
{
  return ImGui::Button(kResetIcon, iSize);
}

//------------------------------------------------------------------------
// ReGui::MenuButton
//------------------------------------------------------------------------
inline bool MenuButton(const ImVec2& iSize = kFaButtonSize)
{
  return ImGui::Button(kMenuIcon, iSize);
}

}

#endif //RE_EDIT_RE_GUI_H
