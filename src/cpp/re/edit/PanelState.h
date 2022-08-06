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

#ifndef RE_EDIT_PANEL_STATE_H
#define RE_EDIT_PANEL_STATE_H

#include "Panel.h"
#include "lua/HDGui2D.h"
#include "lua/Device2D.h"

namespace re::edit {

class PanelState
{
public:
  PanelState(PanelType iPanelType,
             std::shared_ptr<TextureManager> iTextureManager,
             std::shared_ptr<UserPreferences> iUserPreferences,
             std::shared_ptr<PropertyManager> iPropertyManager);

  void initPanel(std::shared_ptr<lua::panel_nodes> const &iPanelNodes,
                 std::shared_ptr<lua::jbox_panel> const &iPanel);

  void render();

  friend class EditContext;

protected:
  void renderWidgets();
  void renderPanel();
  void renderPanelWidgets();
  void renderProperties();

public:
  Panel fPanel;

private:
  DrawContext fDrawContext;
  std::vector<WidgetDef> fWidgetDefs{};
  bool fShowPanel{true};
  bool fShowPanelWidgets{true};
  bool fShowWidgets{};
};

}

#endif //RE_EDIT_PANEL_STATE_H