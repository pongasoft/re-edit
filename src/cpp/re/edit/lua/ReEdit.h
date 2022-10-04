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

#ifndef RE_EDIT_RE_EDIT_H
#define RE_EDIT_RE_EDIT_H

#include "Base.h"
#include <imgui.h>

namespace re::edit::lua {

constexpr float kDefaultFontSize = 12.0f;

constexpr char const *kDefaultHorizontalLayout = R"(
[Window][DockSpaceViewport_11111111]
Pos=0,18
Size=1280,702
Collapsed=0

[Window][re-edit]
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
Size=247,367
Collapsed=0
DockId=0x00000001,0

[Window][Properties]
Pos=1107,18
Size=173,367
Collapsed=0
DockId=0x00000002,0

[Window][Debug##Default]
Pos=60,60
Size=400,400
Collapsed=0

[Window][Save | Warning]
Pos=374,310
Size=532,100
Collapsed=0

[Window][Log]
Pos=0,571
Size=1280,149
Collapsed=0
DockId=0x0000000A,0

[Window][values Editor]
Pos=266,171
Size=748,378
Collapsed=0

[Window][Debug]
Pos=357,308
Size=541,142
Collapsed=0

[Docking][Data]
DockSpace         ID=0x8B93E3BD Window=0xA787BDB4 Pos=0,18 Size=1280,702 Split=Y
  DockNode        ID=0x00000005 Parent=0x8B93E3BD SizeRef=1280,367 Split=X Selected=0xE560F6EE
    DockNode      ID=0x00000007 Parent=0x00000005 SizeRef=389,351 CentralNode=1 Selected=0xE560F6EE
    DockNode      ID=0x00000008 Parent=0x00000005 SizeRef=889,351 Split=X Selected=0xC781E574
      DockNode    ID=0x00000003 Parent=0x00000008 SizeRef=465,351 Selected=0xC781E574
      DockNode    ID=0x00000004 Parent=0x00000008 SizeRef=422,351 Split=X Selected=0x939C4135
        DockNode  ID=0x00000001 Parent=0x00000004 SizeRef=247,351 Selected=0x939C4135
        DockNode  ID=0x00000002 Parent=0x00000004 SizeRef=173,351 Selected=0x199AB496
  DockNode        ID=0x00000006 Parent=0x8B93E3BD SizeRef=1280,333 Split=Y Selected=0xFFEA1EA4
    DockNode      ID=0x00000009 Parent=0x00000006 SizeRef=1280,182 Selected=0xFFEA1EA4
    DockNode      ID=0x0000000A Parent=0x00000006 SizeRef=1280,149 Selected=0x64F50EE5
)";

constexpr char const *kDefaultVerticalLayout = R"(
[Window][DockSpaceViewport_11111111]
Pos=0,18
Size=1280,702
Collapsed=0

[Window][re-edit]
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
Size=486,195
Collapsed=0
DockId=0x00000005,0

[Window][Widgets]
Pos=0,445
Size=486,275
Collapsed=0
DockId=0x00000007,0

[Window][Properties]
Pos=0,605
Size=428,115
Collapsed=0
DockId=0x00000008,0

[Window][Debug##Default]
Pos=60,60
Size=400,400
Collapsed=0

[Window][Save | Warning]
Pos=374,310
Size=532,100
Collapsed=0

[Window][Log]
Pos=430,527
Size=850,193
Collapsed=0
DockId=0x0000000A,0

[Window][values Editor]
Pos=266,171
Size=748,378
Collapsed=0

[Window][Debug]
Pos=357,308
Size=541,142
Collapsed=0

[Docking][Data]
DockSpace         ID=0x8B93E3BD Window=0xA787BDB4 Pos=0,18 Size=1280,702 Split=X Selected=0xE560F6EE
  DockNode        ID=0x00000001 Parent=0x8B93E3BD SizeRef=486,702 Split=Y Selected=0xE560F6EE
    DockNode      ID=0x00000003 Parent=0x00000001 SizeRef=428,228 CentralNode=1 Selected=0xE560F6EE
    DockNode      ID=0x00000004 Parent=0x00000001 SizeRef=428,472 Split=Y Selected=0xC781E574
      DockNode    ID=0x00000005 Parent=0x00000004 SizeRef=428,195 Selected=0xC781E574
      DockNode    ID=0x00000006 Parent=0x00000004 SizeRef=428,275 Split=Y Selected=0x939C4135
        DockNode  ID=0x00000007 Parent=0x00000006 SizeRef=428,158 Selected=0x939C4135
        DockNode  ID=0x00000008 Parent=0x00000006 SizeRef=428,115 Selected=0x199AB496
  DockNode        ID=0x00000002 Parent=0x8B93E3BD SizeRef=792,702 Split=Y Selected=0xFFEA1EA4
    DockNode      ID=0x00000009 Parent=0x00000002 SizeRef=850,507 Selected=0xFFEA1EA4
    DockNode      ID=0x0000000A Parent=0x00000002 SizeRef=850,193 Selected=0x64F50EE5
)";

struct Config
{
  int fNativeWindowWidth{1280};
  int fNativeWindowHeight{720};
  bool fShowProperties{false};
  bool fShowPanel{true};
  bool fShowPanelWidgets{true};
  bool fShowWidgets{true};
  float fFontSize{kDefaultFontSize};
  ImVec2 fGrid{10.0f, 10.0f};

  std::string fImGuiIni{kDefaultHorizontalLayout};
};

class ReEdit : public Base
{
public:
  ReEdit() = default;

  Config getConfig();

  static std::unique_ptr<ReEdit> fromFile(std::string const &iLuaFilename);

protected:
  Config loadConfig();

private:
  std::optional<Config> fConfig{};
};

}


#endif //RE_EDIT_RE_EDIT_H