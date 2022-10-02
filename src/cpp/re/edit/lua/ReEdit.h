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
  bool fShowProperties{true};
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
Size=421,204
Collapsed=0

[Window][Panel]
Pos=1,424
Size=1278,295
Collapsed=0

[Window][Panel Widgets]
Pos=423,19
Size=552,404
Collapsed=0

[Window][Widgets]
Pos=976,19
Size=303,404
Collapsed=0

[Window][Properties]
Pos=1,224
Size=421,199
Collapsed=0

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