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
#include <raylib.h>
#include <cmath>
#include "Color.h"
#include "Constants.h"
#include <IconsFAReEdit.h>
#include <IconsFAReEditCustom.h>
#include <optional>
#include <functional>

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
static constexpr ImVec4 operator*(const ImVec4& lhs, const float rhs)              { return {lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs}; }
static constexpr ImVec4 operator/(const ImVec4& lhs, const ImVec4& rhs)            { return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w}; }

static constexpr bool operator==(const ImVec2& lhs, const ImVec2& rhs)
{
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

static constexpr bool operator!=(const ImVec2& lhs, const ImVec2& rhs)
{
  return !(lhs == rhs);
}

static constexpr bool operator==(const ImVec4& lhs, const ImVec4& rhs)
{
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

static constexpr bool operator!=(const ImVec4& lhs, const ImVec4& rhs)
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
// ReGui::GetRLColor
//------------------------------------------------------------------------
constexpr Color GetRLColor(ImU32 iColor) {
  return Color {
    static_cast<unsigned char>((iColor >> IM_COL32_R_SHIFT) & 0xFF),
    static_cast<unsigned char>((iColor >> IM_COL32_G_SHIFT) & 0xFF),
    static_cast<unsigned char>((iColor >> IM_COL32_B_SHIFT) & 0xFF),
    static_cast<unsigned char>((iColor >> IM_COL32_A_SHIFT) & 0xFF)
  };
}

//------------------------------------------------------------------------
// ReGui::GetRLColor
//------------------------------------------------------------------------
constexpr Color GetRLColor(ImVec4 const &iColor) {
  return GetRLColor(GetColorU32(iColor));
}


//------------------------------------------------------------------------
// ReGui::GetColorU32
// Applies provided alpha (copied from ImGui)
//------------------------------------------------------------------------
constexpr ImU32 GetColorU32(ImVec4 const &iColor, float iAlpha)
{
  auto col = GetColorU32(iColor);
  ImU32 a = (col & IM_COL32_A_MASK) >> IM_COL32_A_SHIFT;
  a = (ImU32)(a * iAlpha); // We don't need to clamp 0..255 because Style.Alpha is in 0..1 range.
  return (col & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT);
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
// ReGui::GetColorImVec4
//------------------------------------------------------------------------
constexpr ImVec4 GetColorImVec4(JboxColor3 const &iColor)
{
  return toFloatColor(iColor.fRed, iColor.fGreen, iColor.fBlue);
}

//------------------------------------------------------------------------
// ReGui::GetColorImU32
//------------------------------------------------------------------------
constexpr ImU32 GetColorImU32(JboxColor3 const &iColor)
{
  ImU32 out = ((ImU32)iColor.fRed) << IM_COL32_R_SHIFT;
  out |= ((ImU32)iColor.fGreen) << IM_COL32_G_SHIFT;
  out |= ((ImU32)iColor.fBlue) << IM_COL32_B_SHIFT;
  out |= ((ImU32)255) << IM_COL32_A_SHIFT;
  return out;
}

//------------------------------------------------------------------------
// ReGui::GetJboxColor3
//------------------------------------------------------------------------
constexpr JboxColor3 GetJboxColor3(ImU32 iColor)
{
  return {
    static_cast<int>((iColor >> IM_COL32_R_SHIFT) & 0xFF),
    static_cast<int>((iColor >> IM_COL32_G_SHIFT) & 0xFF),
    static_cast<int>((iColor >> IM_COL32_B_SHIFT) & 0xFF)
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
constexpr ImU32 kBlackColorU32 = GetColorU32(kBlackColor);
constexpr ImU32 kErrorColorU32 = GetColorU32(kErrorColor);
constexpr ImU32 kTipColorU32 = GetColorU32(kTipColor);
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
    ImGui::Button(iLabel, size);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
  }
  return res;
}

//------------------------------------------------------------------------
// ReGui::TextToggleButton
// Behaves like a toggle button but with a shape of a button
//------------------------------------------------------------------------
inline bool TextToggleButton(char const *iLabel, bool *ioValue, const ImVec2& size = ImVec2(0, 0))
{
  auto res = false;
  if(!*ioValue)
  {
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().DisabledAlpha);
    if(ImGui::Button(iLabel, size))
    {
      *ioValue = true;
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
      *ioValue = false;
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
constexpr auto kTipIcon = fa::kLightbulb;
constexpr auto kResetIcon = fa::kCircleX;
constexpr auto kMenuIcon = fa::kBars;

#define ReGui_Prefix(prefix, str) prefix " " str
#define ReGui_Icon_Reset ICON_FA_CircleX
#define ReGui_Icon_Watch ICON_FA_MagnifyingGlass
#define ReGui_Icon_Edit ICON_FA_Pencil
#define ReGui_Icon_Undo ICON_FA_ArrowRotateLeft
#define ReGui_Icon_Redo ICON_FA_ArrowRotateRight
#define ReGui_Icon_Open ICON_FA_FolderOpen
#define ReGui_Icon_Save ICON_FA_FloppyDisk
#define ReGui_Icon_RescanImages ICON_FAC_FolderImageRotate
#define ReGui_Icon_ImportImages ICON_FAC_FolderImageCirclePlus
#define ReGui_Icon_ReloadMotherboard ICON_FAC_MicrochipRotate
#define ReGui_Icon_Sort ICON_FA_ArrowUpArrowDown
#define ReGui_Icon_Copy ICON_FA_Clipboard
#define ReGui_Icon_Hidden_Widget ICON_FA_EyeSlash
#define ReGui_Icon_Visibility_ByProperty_Widget ICON_FA_Eye
#define ReGui_Icon_Visibility_Manual_Widget ICON_FAC_EyeUser
#define ReGui_Icon_Duplicate ICON_FA_Clone
#define ReGui_Icon_Tip ICON_FA_Lightbulb
#define ReGui_Icon_Frames ICON_FA_Film
#define ReGui_Icon_Frames_Edit ICON_FAC_FilmGear
#define ReGui_Icon_ResetAllEffects ICON_FAC_SparklesCircleXmark
#define ReGui_Icon_CommitAllEffects ICON_FAC_SparklesCircleCheck
#define ReGui_Icon_Effects ICON_FA_Sparkles

#if WIN32
#define ReGui_Icon_KeySuper "Ctrl"
#define ReGui_Icon_KeyShift "Shift"
#define ReGui_Menu_Shortcut2(key1, key2) key1 " + " key2
#define ReGui_Menu_Shortcut3(key1, key2, key3) key1 " + " key2 " + " key3
#else
#define ReGui_Icon_KeySuper "CMD"
#define ReGui_Icon_KeyShift "SFT"
#define ReGui_Menu_Shortcut2(key1, key2) key1 " " key2
#define ReGui_Menu_Shortcut3(key1, key2, key3) key1 " " key2 " " key3
#endif


//------------------------------------------------------------------------
// ReGui::ResetButton
//------------------------------------------------------------------------
inline bool ResetButton()
{
  return ImGui::Button(kResetIcon);
}

//------------------------------------------------------------------------
// ReGui::MenuButton
//------------------------------------------------------------------------
inline bool MenuButton()
{
  return ImGui::Button(kMenuIcon);
}

//------------------------------------------------------------------------
// ReGui::SpacingY
//------------------------------------------------------------------------
inline void SpacingY()
{
  auto cp = ImGui::GetCursorScreenPos();
  cp.y += ImGui::GetStyle().ItemSpacing.y;
  ImGui::SetCursorScreenPos(cp);
}

//------------------------------------------------------------------------
// ReGui::VisibilityButton
//------------------------------------------------------------------------
inline bool VisibilityButton(bool isHidden, bool iByProperty, bool isSelected)
{
  static ImVec2 kButtonSize{};
  static ImGuiOnceUponAFrame kOaf{};

  if(kOaf)
    kButtonSize = ImGui::CalcTextSize(ReGui_Icon_Visibility_ByProperty_Widget);

  if(isHidden)
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().DisabledAlpha / 2.0f);

  auto res = ImGui::Selectable(iByProperty ? ReGui_Icon_Visibility_ByProperty_Widget : ReGui_Icon_Visibility_Manual_Widget,
                               isSelected,
                               0,
                               kButtonSize);

  if(isHidden)
    ImGui::PopStyleVar();

  return res;
}

//------------------------------------------------------------------------
// ReGui::ErrorIcon
//------------------------------------------------------------------------
inline void ErrorIcon()
{
  ImGui::PushStyleColor(ImGuiCol_Text, kErrorColorU32);
  ImGui::TextUnformatted(ReGui::kErrorIcon);
  ImGui::PopStyleColor();
}

//------------------------------------------------------------------------
// ReGui::TipIcon
//------------------------------------------------------------------------
inline void TipIcon()
{
  ImGui::PushStyleColor(ImGuiCol_Text, kTipColorU32);
  ImGui::TextUnformatted(ReGui::kTipIcon);
  ImGui::PopStyleColor();
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
// ReGui::CenterNextWindow
//------------------------------------------------------------------------
inline void CenterNextWindow(ImGuiCond iFlags = ImGuiCond_Appearing)
{
  static ImVec2 mid = {0.5f, 0.5f};
  const auto center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, iFlags, mid);
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
// ReGui::IsKeyAlt
//------------------------------------------------------------------------
inline bool IsKeyAlt()
{
  return ImGui::GetIO().KeyAlt;
}

//------------------------------------------------------------------------
// ReGui::IsFilterEnabled
//------------------------------------------------------------------------
inline bool IsFilterEnabled()
{
  return !IsKeyAlt();
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
// ReGui::BeginDisabled
// return whether disabled or not
//------------------------------------------------------------------------
inline bool BeginDisabled(bool disabled = true)
{
  ImGui::BeginDisabled(disabled);
  return disabled;
}

//------------------------------------------------------------------------
// ReGui::ShowTooltip
//------------------------------------------------------------------------
inline bool ShowTooltip()
{
  return ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal);
}

//------------------------------------------------------------------------
// ReGui::IsQuickView
//------------------------------------------------------------------------
inline bool IsQuickView()
{
  return ImGui::IsKeyDown(ImGuiKey_Q);
}


//------------------------------------------------------------------------
// ReGui::ShowQuickView
//------------------------------------------------------------------------
inline bool ShowQuickView()
{
  return ImGui::IsItemHovered() && IsQuickView();
}

//------------------------------------------------------------------------
// ReGui::ToolTip
//------------------------------------------------------------------------
template<typename F>
void ToolTip(F &&iTooltipContent)
{
  ImGui::BeginTooltip();
  ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
  iTooltipContent();
  ImGui::PopTextWrapPos();
  ImGui::EndTooltip();
}

//------------------------------------------------------------------------
// ReGui::CopyToClipboard
//------------------------------------------------------------------------
template<typename F>
void CopyToClipboard(F &&iContent)
{
  ImGui::PushID(&iContent);
  bool copy_to_clipboard = ImGui::Button(ReGui_Icon_Copy);
  if(ReGui::ShowTooltip())
  {
    ReGui::ToolTip([]{
      ImGui::TextUnformatted("Copy to clipboard");
    });
  }
  if(copy_to_clipboard)
  {
    ImGui::LogToClipboard();
  }
  iContent();
  if(copy_to_clipboard)
  {
    ImGui::LogFinish();
  }
  ImGui::PopID();
}

//------------------------------------------------------------------------
// ReGui::MultiLineText
// Issues one ImGui::Text per line of text
//------------------------------------------------------------------------
void MultiLineText(std::string const &iText);

//------------------------------------------------------------------------
// ReGui::Modifier
// Similar concept to jetpack compose
//------------------------------------------------------------------------
struct Modifier
{
  ImVec4 fPadding{};
  ImU32 fBackgroundColor{};
  ImU32 fBorderColor{};

  constexpr Modifier &padding(float iTop, float iRight, float iBottom, float iLeft) { fPadding = {iTop, iRight, iBottom, iLeft}; return *this; } // x/y/z/w
  constexpr Modifier &padding(float iPadding) { return padding(iPadding, iPadding, iPadding, iPadding); }
  constexpr Modifier &padding(float iHorizontalPadding, float iVerticalPadding) { return padding(iVerticalPadding, iHorizontalPadding, iVerticalPadding, iHorizontalPadding); }

  constexpr Modifier &backgroundColor(ImU32 iColor) { fBackgroundColor = iColor; return *this; }
  constexpr Modifier &borderColor(ImU32 iColor) { fBorderColor = iColor; return *this; }
};

/**
 * Renders the content inside a "box" defined by its padding and background/border color.
 *
 * Note that the implementation is optimized in the event that only padding is provided. If you want to nest boxes
 * you must provide your own splitter like this:
 *
 * ```cpp
 *  ReGui::Box(ReGui::Modifier{}.padding(10.0f).backgroundColor(ReGui::kRedColorU32), []{
 *    static ImDrawListSplitter kNestedSplitter{}; // MUST be static for performance reasons
 *    ImGui::Text("Inside before");
 *    ReGui::Box(ReGui::Modifier{}.padding(15.0f).borderColor(ReGui::kWhiteColorU32), [] {
 *                 ImGui::Text("Nested");
 *               },
 *               &kNestedSplitter);
 *    ImGui::Text("Inside after");
 *  });
 *  ```
 *
 * @param iModifier the box definition
 * @param iBoxContent the content to render inside the box
 * @param iSplitter optional unless you want to nest boxes
 */
void Box(Modifier const &iModifier, std::function<void()> const &iBoxContent, ImDrawListSplitter *iSplitter = nullptr);

//------------------------------------------------------------------------
// ReGui::RadioButton
// Like a TextRadioButton but for any kind of content
//------------------------------------------------------------------------
template<typename T>
inline bool RadioButton(char const *iLabel, T *ioValue, T iTrueValue, std::function<void()> const &iContent)
{
  ImGui::PushID(iLabel);
  auto res = false;
  if(*ioValue != iTrueValue)
  {
    auto &style = ImGui::GetStyle();

    auto const modifier = ReGui::Modifier{}
      .padding(style.FramePadding.x, style.FramePadding.y)
      .backgroundColor(ReGui::GetColorU32(style.Colors[ImGuiCol_Button], ImGui::GetStyle().DisabledAlpha));

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().DisabledAlpha);
    ReGui::Box(modifier, iContent);
    ImGui::PopStyleVar();

    if(ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
      *ioValue = iTrueValue;
      res = true;
    }
  }
  else
  {
    auto &style = ImGui::GetStyle();

    auto const modifier = ReGui::Modifier{}
      .padding(style.FramePadding.x, style.FramePadding.y)
      .backgroundColor(ReGui::GetColorU32(style.Colors[ImGuiCol_Button]))
      .borderColor(ReGui::GetColorU32(style.Colors[ImGuiCol_Text]));

    ReGui::Box(modifier, iContent);
  }
  ImGui::PopID();
  return res;
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

    ~Lifecycle();

    constexpr explicit operator bool() const { return fContentEnabled; }

  private:
    constexpr Lifecycle() : fContentEnabled{false}, fEndRequired{false} {}
    explicit constexpr Lifecycle(bool iContentEnabled) : fContentEnabled{iContentEnabled}, fEndRequired{true} {}

  private:
    bool fContentEnabled;
    bool fEndRequired;
  };
public:
  Window(char const *iKey, std::optional<bool> iVisible, ImGuiWindowFlags iFlags = 0) :
    fKey{iKey},
    fName{iKey},
    fVisible{iVisible == std::nullopt || *iVisible},
    fDisableClosingWidget{iVisible == std::nullopt},
    fFlags{iFlags} {}

  void requestSizeToFit() { fSizeToFitRequested = 2; }

  constexpr bool isVisible() const { return fVisible; }
  constexpr void setIsVisible(bool iVisible) { fVisible = iVisible; }
  void setName(std::string const &iName);

  [[nodiscard]] Lifecycle begin(ImGuiWindowFlags flags = 0);

  void menuItem()
  {
    ImGui::MenuItem(fName.c_str(), nullptr, &fVisible);
  }

private:
  char const *fKey;
  std::string fName;
  bool fVisible;
  bool fDisableClosingWidget;
  ImGuiWindowFlags fFlags;

  int fSizeToFitRequested{};
};

//------------------------------------------------------------------------
// ReGui::Rect
// Copied from imgui_internal (not public api) and made constexpr
//------------------------------------------------------------------------
struct Rect
{
  ImVec2      Min;    // Upper-left
  ImVec2      Max;    // Lower-right

  constexpr Rect()                                        : Min(0.0f, 0.0f), Max(0.0f, 0.0f)  {}
  constexpr Rect(const ImVec2& min, const ImVec2& max)    : Min(min), Max(max)                {}
  constexpr explicit Rect(const ImVec4& v)                : Min(v.x, v.y), Max(v.z, v.w)      {}
  constexpr Rect(float x1, float y1, float x2, float y2)  : Min(x1, y1), Max(x2, y2)          {}

  constexpr ImVec2      GetCenter() const                   { return {(Min.x + Max.x) * 0.5f, (Min.y + Max.y) * 0.5f}; }
  constexpr ImVec2      GetSize() const                     { return {Max.x - Min.x, Max.y - Min.y}; }
  constexpr float       GetWidth() const                    { return Max.x - Min.x; }
  constexpr float       GetHeight() const                   { return Max.y - Min.y; }
//  constexpr float       GetArea() const                     { return (Max.x - Min.x) * (Max.y - Min.y); }
  constexpr ImVec2      GetTL() const                       { return Min; }                   // Top-left
  constexpr ImVec2      GetTR() const                       { return {Max.x, Min.y}; }  // Top-right
  constexpr ImVec2      GetBL() const                       { return {Min.x, Max.y}; }  // Bottom-left
  constexpr ImVec2      GetBR() const                       { return Max; }                   // Bottom-right
//  constexpr bool        Contains(const ImVec2& p) const     { return p.x     >= Min.x && p.y     >= Min.y && p.x     <  Max.x && p.y     <  Max.y; }
//  constexpr bool        Contains(const Rect& r) const       { return r.Min.x >= Min.x && r.Min.y >= Min.y && r.Max.x <= Max.x && r.Max.y <= Max.y; }
//  constexpr bool        Overlaps(const Rect& r) const       { return r.Min.y <  Max.y && r.Max.y >  Min.y && r.Min.x <  Max.x && r.Max.x >  Min.x; }
//  constexpr void        Add(const ImVec2& p)                { if (Min.x > p.x)     Min.x = p.x;     if (Min.y > p.y)     Min.y = p.y;     if (Max.x < p.x)     Max.x = p.x;     if (Max.y < p.y)     Max.y = p.y; }
//  constexpr void        Add(const Rect& r)                  { if (Min.x > r.Min.x) Min.x = r.Min.x; if (Min.y > r.Min.y) Min.y = r.Min.y; if (Max.x < r.Max.x) Max.x = r.Max.x; if (Max.y < r.Max.y) Max.y = r.Max.y; }
//  constexpr void        Expand(const float amount)          { Min.x -= amount;   Min.y -= amount;   Max.x += amount;   Max.y += amount; }
//  constexpr void        Expand(const ImVec2& amount)        { Min.x -= amount.x; Min.y -= amount.y; Max.x += amount.x; Max.y += amount.y; }
//  constexpr void        Translate(const ImVec2& d)          { Min.x += d.x; Min.y += d.y; Max.x += d.x; Max.y += d.y; }
//  constexpr void        TranslateX(float dx)                { Min.x += dx; Max.x += dx; }
//  constexpr void        TranslateY(float dy)                { Min.y += dy; Max.y += dy; }
//  constexpr void        ClipWith(const Rect& r)             { Min = ImMax(Min, r.Min); Max = ImMin(Max, r.Max); }                   // Simple version, may lead to an inverted rectangle, which is fine for Contains/Overlaps test but not for display.
  constexpr void        ClipWithFull(const Rect& r)         { Min = ImClamp(Min, r.Min, r.Max); Max = ImClamp(Max, r.Min, r.Max); } // Full version, ensure both points are fully clipped.
//  constexpr void        Floor()                             { Min.x = IM_FLOOR(Min.x); Min.y = IM_FLOOR(Min.y); Max.x = IM_FLOOR(Max.x); Max.y = IM_FLOOR(Max.y); }
//  constexpr bool        IsInverted() const                  { return Min.x > Max.x || Min.y > Max.y; }
  constexpr ImVec4      ToVec4() const                      { return {Min.x, Min.y, Max.x, Max.y}; }

private:
  static constexpr ImVec2 ImClamp(const ImVec2& v, const ImVec2& mn, ImVec2 mx)      { return ImVec2((v.x < mn.x) ? mn.x : (v.x > mx.x) ? mx.x : v.x, (v.y < mn.y) ? mn.y : (v.y > mx.y) ? mx.y : v.y); }
};

}

#endif //RE_EDIT_RE_GUI_H
