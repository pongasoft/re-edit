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
#include <optional>

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

namespace impl {
constexpr auto ImSaturate(float f) { return (f < 0.0f) ? 0.0f : (f > 1.0f) ? 1.0f : f; }
constexpr auto ImF32ToInt8Sat(float f) { return ((int)(ImSaturate(f) * 255.0f + 0.5f)); }
}

//------------------------------------------------------------------------
// ReGui::GetColorU32
// ImGui::GetColorU32 is a runtime call which uses global alpha, which is 1.0f
// This call is a constexpr call (ignores the global alpha)
//------------------------------------------------------------------------
constexpr ImU32 GetColorU32(ImVec4 const &iColor)
{
  ImU32 out = ((ImU32)impl::ImF32ToInt8Sat(iColor.x)) << IM_COL32_R_SHIFT;
  out |= ((ImU32)impl::ImF32ToInt8Sat(iColor.y)) << IM_COL32_G_SHIFT;
  out |= ((ImU32)impl::ImF32ToInt8Sat(iColor.z)) << IM_COL32_B_SHIFT;
  out |= ((ImU32)impl::ImF32ToInt8Sat(iColor.w)) << IM_COL32_A_SHIFT;
  return out;
}

//------------------------------------------------------------------------
// ReGui::GetColorImVec4
// Convert back an ImU32 to a ImVec4
//------------------------------------------------------------------------
constexpr ImVec4 GetColorImVec4(ImU32 iColor)
{
  constexpr float sc = 1.0f / 255.0f;
  return {
    (float)((iColor >> IM_COL32_R_SHIFT) & 0xFF) * sc,
    (float)((iColor >> IM_COL32_G_SHIFT) & 0xFF) * sc,
    (float)((iColor >> IM_COL32_B_SHIFT) & 0xFF) * sc,
    (float)((iColor >> IM_COL32_A_SHIFT) & 0xFF) * sc
  };
}

//------------------------------------------------------------------------
// ReGui::ColorIsTransparent
// return `true` if the color is fully transparent (meaning drawing with it is a noop)
//------------------------------------------------------------------------
constexpr bool ColorIsTransparent(ImU32 iColor)
{
  return (iColor & IM_COL32_A_MASK) == 0;
}

constexpr ImU32 kWhiteColorU32 = GetColorU32(kWhiteColor);
constexpr ImU32 kTransparentColorU32 = 0;

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
// ReGui::TextRadioButton
// Behaves like a radio button but with a shape of a button
//------------------------------------------------------------------------
template<typename T>
inline bool TextRadioButton(char const *iLabel, T *ioValue, T iTrueValue, const ImVec2& size = ImVec2(0, 0))
{
  auto res = false;
  if(*ioValue != iTrueValue)
  {
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().DisabledAlpha);
    if(ImGui::Button(iLabel, size))
    {
      *ioValue = iTrueValue;
      res = true;
    }
    ImGui::PopStyleVar();
  }
  else
  {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0);
    ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetStyle().Colors[ImGuiCol_Text]);
    if(ImGui::Button(iLabel, size))
    {
      *ioValue = iTrueValue;
      res = true;
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
  }
  return res;
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
#define ReGui_Icon_Sort ICON_FA_ArrowUpArrowDown

//------------------------------------------------------------------------
// ReGui::ResetButton
//------------------------------------------------------------------------
inline bool ResetButton(const ImVec2& iSize)
{
  return ImGui::Button(kResetIcon, iSize);
}

//------------------------------------------------------------------------
// ReGui::MenuButton
//------------------------------------------------------------------------
inline bool MenuButton(const ImVec2& iSize)
{
  return ImGui::Button(kMenuIcon, iSize);
}

//------------------------------------------------------------------------
// ReGui::DefaultHeaderColumn
//------------------------------------------------------------------------
inline void DefaultHeaderColumn(int iColumn)
{
  ImGui::TableSetColumnIndex(iColumn);
  const char* column_name = ImGui::TableGetColumnName(iColumn); // Retrieve name passed to TableSetupColumn()
  ImGui::PushID(iColumn);
  ImGui::TableHeader(column_name);
  ImGui::PopID();
}

//------------------------------------------------------------------------
// ReGui::AnySpecialKey
//------------------------------------------------------------------------
inline bool AnySpecialKey()
{
  auto &io = ImGui::GetIO();
  return io.KeyShift || io.KeyCtrl || io.KeyAlt || io.KeySuper;
}

//------------------------------------------------------------------------
// ReGui::IsSingleSelectKey
//------------------------------------------------------------------------
constexpr bool IsSingleSelectKey(ImGuiIO const &io)
{
#if WIN32
  return io.KeyCtrl;
#else
  return io.KeySuper;
#endif
}

//------------------------------------------------------------------------
// ReGui::Window
//------------------------------------------------------------------------
class Window
{
public:
  class Lifecycle
  {
  public:
    friend class Window;
    Lifecycle(Lifecycle const &) = delete;
    Lifecycle &operator=(Lifecycle const &) = delete;

    ~Lifecycle()
    {
      if(fEndRequired)
        ImGui::End();
    }

    constexpr explicit operator bool() const { return fContentEnabled; }

  private:
    constexpr Lifecycle() : fContentEnabled{false}, fEndRequired{false} {}
    explicit constexpr Lifecycle(bool iContentEnabled) : fContentEnabled{iContentEnabled}, fEndRequired{true} {}

  private:
    bool fContentEnabled;
    bool fEndRequired;
  };
public:
  Window(char const *iName, std::optional<bool> iVisible, ImGuiWindowFlags iFlags = 0) :
    fName{iName},
    fVisible{iVisible == std::nullopt || *iVisible},
    fDisableClosingWidget{iVisible == std::nullopt},
    fFlags{iFlags} {}

  void requestSizeToFit() { fSizeToFitRequested = 2; }

  constexpr bool isVisible() const { return fVisible; }
  constexpr void setIsVisible(bool iVisible) { fVisible = iVisible; }

  [[nodiscard]] Lifecycle begin(ImGuiWindowFlags flags = 0)
  {
    if(fVisible)
    {
      if(fSizeToFitRequested > 0)
      {
        flags |= ImGuiWindowFlags_AlwaysAutoResize;
        fSizeToFitRequested--;
      }
      return Lifecycle{ImGui::Begin(fName, fDisableClosingWidget ? nullptr : &fVisible, fFlags | flags)};
    }
    else
      return {};
  }

  void menuItem()
  {
    ImGui::MenuItem(fName, nullptr, &fVisible);
  }

private:
  char const *fName;
  bool fVisible;
  bool fDisableClosingWidget;
  ImGuiWindowFlags fFlags;

  int fSizeToFitRequested{};
};


}

#endif //RE_EDIT_RE_GUI_H
