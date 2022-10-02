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

  std::string fImGuiIni{R"(
[Window][Debug##Default]
Pos=60,60
Size=400,400
Collapsed=0

[Window][re-edit]
Pos=1,19
Size=340,329
Collapsed=0
DockId=0x00000002,0

[Window][Panel]
Pos=1,350
Size=1278,367
Collapsed=0
DockId=0x00000009,0

[Window][Panel Widgets]
Pos=343,19
Size=514,329
Collapsed=0
DockId=0x00000003,0

[Window][Widgets]
Pos=859,19
Size=420,329
Collapsed=0
DockId=0x00000004,0

[Window][Properties]
Pos=1084,19
Size=195,329
Collapsed=0
DockId=0x00000005,0

[Window][Log]
Pos=372,65
Size=567,590
Collapsed=0

[Window][Debug]
Pos=189,356
Size=900,122
Collapsed=0

[Window][values Editor]
Pos=260,166
Size=788,404
Collapsed=0

[Window][Dear ImGui Demo]
Pos=650,39
Size=550,680
Collapsed=0

[Window][Dear ImGui Metrics/Debugger]
Pos=407,158
Size=339,290
Collapsed=0

[Window][Save | Warning]
Pos=344,307
Size=532,100
Collapsed=0

[Docking][Data]
DockNode        ID=0x00000001 Pos=1,19 Size=1278,698 Split=Y Selected=0xC781E574
  DockNode      ID=0x00000008 Parent=0x00000001 SizeRef=1278,329 Split=X
    DockNode    ID=0x00000006 Parent=0x00000008 SizeRef=856,204 Split=X
      DockNode  ID=0x00000002 Parent=0x00000006 SizeRef=340,204 Selected=0xE560F6EE
      DockNode  ID=0x00000003 Parent=0x00000006 SizeRef=514,204 Selected=0xC781E574
    DockNode    ID=0x00000007 Parent=0x00000008 SizeRef=420,204 Split=X Selected=0x939C4135
      DockNode  ID=0x00000004 Parent=0x00000007 SizeRef=223,329 Selected=0x939C4135
      DockNode  ID=0x00000005 Parent=0x00000007 SizeRef=195,329 Selected=0x199AB496
  DockNode      ID=0x00000009 Parent=0x00000001 SizeRef=1278,367 Selected=0xFFEA1EA4
)"};

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