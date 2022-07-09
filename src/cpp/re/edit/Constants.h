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

constexpr char const *kWidgetTypes[] = { "analog_knob", "static_decoration", "panel_decal" };

enum class WidgetType : int
{
  kAnalogKnob = 0,
  kStaticDecoration,
  kPanelDecal
};

constexpr char const *toString(WidgetType iType)
{
  return kWidgetTypes[static_cast<int>(iType)];
}

constexpr int k1UPixelSize = 345;
constexpr int kDevicePixelWidth = 3770;
constexpr int kFoldedDevicePixelHeight = 150;

constexpr int toPixelHeight(int iDeviceHeightRU)
{
  return k1UPixelSize * static_cast<int>(iDeviceHeightRU);
}

}

#endif //RE_EDIT_CONSTANTS_H
