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

namespace re::edit {

enum class PanelType : int
{
  kInvalid     = 0,
  kFront       = 1 << 0,
  kBack        = 1 << 1,
  kFoldedFront = 1 << 2,
  kFoldedBack  = 1 << 3,
  kAnyFront    = kFront | kFoldedFront,
  kAnyBack     = kBack | kFoldedBack,
  kAnyUnfolded = kFront | kBack,
  kAnyFolded   = kFoldedFront | kFoldedBack,
  kAny         = kFront | kBack | kFoldedFront | kFoldedBack
};

constexpr bool isPanelType(PanelType self, PanelType ofKind)
{
  return (static_cast<int>(self) & static_cast<int>(ofKind)) != 0;
}

enum class WidgetType : int
{
  kAnalogKnob = 0,
  kAudioInputSocket,
  kAudioOutputSocket,
  kCustomDisplay,
  kCVInputSocket,
  kCVOutputSocket,
  kDeviceName,
  kPlaceholder,
  kSequenceFader,
  kStaticDecoration,
  kPanelDecal
};

char const *toString(WidgetType iType);

constexpr int k1UPixelSize = 345;
constexpr int kDevicePixelWidth = 3770;
constexpr int kFoldedDevicePixelHeight = 150;

constexpr auto kHitBoundariesColor = ImVec4{ 60.0f / 255.0f, 1.0f, 2.0f / 255.0f, 1};

constexpr auto kAudioSocketSize = ImVec2{95, 105};
constexpr auto kCVSocketSize = ImVec2{80, 90};
constexpr auto kPlaceholderSize = ImVec2{300, 100};

constexpr int toPixelHeight(int iDeviceHeightRU)
{
  return k1UPixelSize * static_cast<int>(iDeviceHeightRU);
}

}

#endif //RE_EDIT_CONSTANTS_H
