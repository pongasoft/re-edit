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

#ifndef RE_EDIT_CONFIG_H
#define RE_EDIT_CONFIG_H

#include <imgui.h>
#include <chrono>
#include <vector>

namespace re::edit::config {

enum class Style
{
  kDark,
  kLight,
  kClassic
};

constexpr char const *to_string(Style s)
{
  switch(s)
  {
    case Style::kDark:
      return "Dark";
    case Style::kLight:
      return "Light";
    case Style::kClassic:
      return "Classic";
  }
}

constexpr float kDefaultFontSize = 12.0f;
constexpr int kDefaultDeviceWindowWidth = 1280;
constexpr int kDefaultDeviceWindowHeight = 720;
constexpr int kWelcomeWindowWidth = 600;
constexpr int kWelcomeWindowHeight = 500;
constexpr char const *kWelcomeWindowTitle = "Welcome to RE Edit";
constexpr Style kDefaultStyle = Style::kDark;

constexpr char const *kDefaultHorizontalLayout = R"(
[Window][DockSpaceViewport_11111111]
Pos=0,18
Size=1280,702
Collapsed=0

[Window][###re-edit]
Pos=0,18
Size=389,367
Collapsed=0
DockId=0x00000007,0

[Window][Panel]
Pos=0,387
Size=1280,333
Collapsed=0
DockId=0x00000009,0

[Window][Panel Widgets]
Pos=391,18
Size=465,367
Collapsed=0
DockId=0x00000003,0

[Window][Widgets]
Pos=858,18
Size=422,367
Collapsed=0
DockId=0x00000001,0

[Window][Properties]
Pos=1107,18
Size=87,367
Collapsed=0
DockId=0x0000000B,0
[Window][Debug##Default]
Pos=60,60
Size=400,400
Collapsed=0

[Window][Log]
Pos=0,571
Size=1280,149
Collapsed=0
DockId=0x0000000A,0

[Window][Debug]
Pos=357,308
Size=541,142
Collapsed=0

[Window][Undo History]
Pos=1107,18
Size=173,367
Collapsed=0
DockId=0x0000000C,0

[Docking][Data]
DockSpace           ID=0x8B93E3BD Window=0xA787BDB4 Pos=0,18 Size=1280,702 Split=Y
  DockNode          ID=0x00000005 Parent=0x8B93E3BD SizeRef=1280,367 Split=X Selected=0xE560F6EE
    DockNode        ID=0x00000007 Parent=0x00000005 SizeRef=389,351 Selected=0x8F424DF5
    DockNode        ID=0x00000008 Parent=0x00000005 SizeRef=889,351 Split=X Selected=0xC781E574
      DockNode      ID=0x00000003 Parent=0x00000008 SizeRef=465,351 Selected=0xC781E574
      DockNode      ID=0x00000004 Parent=0x00000008 SizeRef=422,351 Split=X Selected=0x939C4135
        DockNode    ID=0x00000001 Parent=0x00000004 SizeRef=247,351 Selected=0x939C4135
        DockNode    ID=0x00000002 Parent=0x00000004 SizeRef=173,351 Split=X Selected=0x199AB496
          DockNode  ID=0x0000000B Parent=0x00000002 SizeRef=87,367 Selected=0x199AB496
          DockNode  ID=0x0000000C Parent=0x00000002 SizeRef=84,367 Selected=0x922CAA46
  DockNode          ID=0x00000006 Parent=0x8B93E3BD SizeRef=1280,333 Split=Y Selected=0xFFEA1EA4
    DockNode        ID=0x00000009 Parent=0x00000006 SizeRef=1280,182 Selected=0xFFEA1EA4
    DockNode        ID=0x0000000A Parent=0x00000006 SizeRef=1280,149 Selected=0x64F50EE5
)";

constexpr char const *kDefaultVerticalLayout = R"(
[Window][DockSpaceViewport_11111111]
Pos=0,18
Size=1280,702
Collapsed=0

[Window][###re-edit]
Pos=0,18
Size=486,228
Collapsed=0
DockId=0x00000003,0

[Window][Panel]
Pos=488,18
Size=792,702
Collapsed=0
DockId=0x00000009,0

[Window][Panel Widgets]
Pos=0,248
Size=486,143
Collapsed=0
DockId=0x00000005,0

[Window][Widgets]
Pos=0,393
Size=486,156
Collapsed=0
DockId=0x00000007,0

[Window][Properties]
Pos=0,551
Size=486,77
Collapsed=0
DockId=0x0000000B,0
[Window][Debug##Default]
Pos=60,60
Size=400,400
Collapsed=0

[Window][Log]
Pos=430,527
Size=850,193
Collapsed=0
DockId=0x0000000A,0

[Window][Debug]
Pos=357,308
Size=541,142
Collapsed=0

[Window][Undo History]
Pos=0,630
Size=486,90
Collapsed=0
DockId=0x0000000C,0

[Docking][Data]
DockSpace           ID=0x8B93E3BD Window=0xA787BDB4 Pos=0,18 Size=1280,702 Split=X Selected=0xE560F6EE
  DockNode          ID=0x00000001 Parent=0x8B93E3BD SizeRef=486,702 Split=Y Selected=0xE560F6EE
    DockNode        ID=0x00000003 Parent=0x00000001 SizeRef=428,228 Selected=0x8F424DF5
    DockNode        ID=0x00000004 Parent=0x00000001 SizeRef=428,472 Split=Y Selected=0xC781E574
      DockNode      ID=0x00000005 Parent=0x00000004 SizeRef=428,143 Selected=0xC781E574
      DockNode      ID=0x00000006 Parent=0x00000004 SizeRef=428,327 Split=Y Selected=0x939C4135
        DockNode    ID=0x00000007 Parent=0x00000006 SizeRef=428,156 Selected=0x939C4135
        DockNode    ID=0x00000008 Parent=0x00000006 SizeRef=428,169 Split=Y Selected=0x199AB496
          DockNode  ID=0x0000000B Parent=0x00000008 SizeRef=486,77 Selected=0x199AB496
          DockNode  ID=0x0000000C Parent=0x00000008 SizeRef=486,90 Selected=0x922CAA46
  DockNode          ID=0x00000002 Parent=0x8B93E3BD SizeRef=792,702 Split=Y Selected=0xFFEA1EA4
    DockNode        ID=0x00000009 Parent=0x00000002 SizeRef=850,507 Selected=0xFFEA1EA4
    DockNode        ID=0x0000000A Parent=0x00000002 SizeRef=850,193 Selected=0x64F50EE5
)";

inline auto now() { return std::chrono::system_clock::now().time_since_epoch().count(); }

struct Device
{
  using time_t = decltype(now());

  std::string fName{};
  std::string fPath{};
  std::string fType{};
  bool fShowProperties{false};
  bool fShowPanel{true};
  bool fShowPanelWidgets{true};
  bool fShowWidgets{true};
  bool fShowUndoHistory{false};
  ImVec2 fGrid{10.0f, 10.0f};
  std::string fImGuiIni{kDefaultHorizontalLayout};
  ImVec2 fNativeWindowSize{kDefaultDeviceWindowWidth, kDefaultDeviceWindowHeight};
  std::optional<ImVec2> fNativeWindowPos{};
  time_t fLastAccessTime{};
};

struct Global
{
  float fFontSize{kDefaultFontSize};
  Style fStyle{kDefaultStyle};

  std::vector<Device> fDeviceHistory{};

  void addDeviceConfigToHistory(Device const &iItem)
  {
    removeDeviceConfigFromHistory(iItem.fPath);
    fDeviceHistory.emplace_back(iItem);
  }

  void removeDeviceConfigFromHistory(std::string const &iPath)
  {
    auto iter = std::find_if(fDeviceHistory.begin(), fDeviceHistory.end(), [&iPath](auto const &item) { return item.fPath == iPath; });
    if(iter != fDeviceHistory.end())
      fDeviceHistory.erase(iter);
  }

  Device getDeviceConfigFromHistory(std::string const &iPath) const
  {
    auto iter = std::find_if(fDeviceHistory.begin(), fDeviceHistory.end(), [&iPath](auto const &item) { return item.fPath == iPath; });
    if(iter != fDeviceHistory.end())
      return *iter;
    else
      return {};
  }

  void clearDeviceConfigHistory()
  {
    fDeviceHistory.clear();
  }
};


}

#endif //RE_EDIT_CONFIG_H
