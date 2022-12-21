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

#ifndef RE_EDIT_CONSTANTS_H
#define RE_EDIT_CONSTANTS_H

#include <string>
#include <imgui.h>
#include <bitmask_operators.hpp>
#include <re/mock/fmt.h>

namespace re::edit {

enum class PanelType : int
{
  kUnknown     = 0,
  kFront       = 1 << 0,
  kBack        = 1 << 1,
  kFoldedFront = 1 << 2,
  kFoldedBack  = 1 << 3,
};

}

template<>
struct enable_bitmask_operators<re::edit::PanelType> {
  static const bool enable=true;
};

namespace re::edit {

constexpr bool isPanelOfType(PanelType iSelf, PanelType iOfType)
{
  return (iSelf & iOfType) != PanelType::kUnknown;
}

constexpr auto kPanelTypeAny         = PanelType::kFront | PanelType::kBack | PanelType::kFoldedFront | PanelType::kFoldedBack;
constexpr auto kPanelTypeAnyFront    = PanelType::kFront | PanelType::kFoldedFront;
constexpr auto kPanelTypeAnyUnfolded = PanelType::kFront | PanelType::kBack;

enum class WidgetType : int
{
  kAnalogKnob = 0,
  kAudioInputSocket,
  kAudioOutputSocket,
  kCustomDisplay,
  kCVInputSocket,
  kCVOutputSocket,
  kCVTrimKnob,
  kDeviceName,
  kMomentaryButton,
  kPatchBrowseGroup,
  kPatchName,
  kPitchWheel,
  kPlaceholder,
  kPopupButton,
  kRadioButton,
  kSampleBrowseGroup,
  kSampleDropZone,
  kSequenceFader,
  kSequenceMeter,
  kStaticDecoration,
  kStepButton,
  kToggleButton,
  kUpDownButton,
  kValueDisplay,
  kZeroSnapKnob,

  kPanelDecal // re-edit widget
};

char const *toString(WidgetType iType);

constexpr int k1UPixelSize = 345;
constexpr int kDevicePixelWidth = 3770;
constexpr int kFoldedDevicePixelHeight = 150;


constexpr auto kNoGraphics = ImVec2{100, 100};

constexpr auto kAudioSocketSize = ImVec2{95, 105};
constexpr auto kCVSocketSize = ImVec2{75, 85};
constexpr auto kCVTrimKnobSize = ImVec2{100, 100};
constexpr auto kPatchBrowseGroupSize = ImVec2{290, 110};
constexpr auto kPlaceholderSize = ImVec2{300, 100};
constexpr auto kSampleBrowseGroupSize = ImVec2{290, 110};
constexpr auto kDeviceNameHorizontal = ImVec2{395, 65};
constexpr auto kDeviceNameVertical = ImVec2{65, 395};

constexpr float toFloatColor(int iColor) { return static_cast<float>(iColor) / 255.0f; }
constexpr ImVec4 toFloatColor(int r, int g, int b, int a = 255) { return ImVec4{toFloatColor(r), toFloatColor(g), toFloatColor(b), toFloatColor(a)}; }

constexpr auto kHitBoundariesColor = toFloatColor(60, 255, 2);
constexpr auto kWhiteColor = ImVec4{1, 1, 1, 1};
constexpr auto kBlackColor = ImVec4{0, 0, 0, 1};
constexpr auto kErrorColor = ImVec4{1,0,0,1};
constexpr auto kTipColor = toFloatColor(92, 184, 92);
constexpr auto kXRayColor = ImVec4{1, 1, 1, 0.4};

constexpr int toPixelHeight(int iDeviceHeightRU)
{
  return k1UPixelSize * static_cast<int>(iDeviceHeightRU);
}

constexpr int toIntColor(float iColor) { return static_cast<int>(iColor * 255.0f); }

namespace fmt {
template<typename ... Args>
inline std::string printf(const std::string& format, Args ... args)
{
  return re::mock::fmt::printf(format, args...);
}

namespace Bool {
constexpr char const *to_chars(bool b) { return b ? "true" : "false"; }
inline std::string to_string(bool b) { return to_chars(b); }
}

}

}

#endif //RE_EDIT_CONSTANTS_H
