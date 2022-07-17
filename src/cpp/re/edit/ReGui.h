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

}

#endif //RE_EDIT_RE_GUI_H
