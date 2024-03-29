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
  explicit PanelState(PanelType iPanelType);

  constexpr PanelType getType() const { return fPanel.getType(); }
  constexpr bool isUnfoldedPanel() const { return isPanelOfType(getType(), kPanelTypeAnyUnfolded); }

  std::vector<WidgetDef> const &getAllowedWidgets() const { return fWidgetDefs; }
  inline bool isWidgetAllowed(WidgetType iType) const { return fAllowedWidgetTypes[iType]; }

  void initPanel(AppContext &iCtx,
                 std::shared_ptr<lua::panel_nodes> const &iPanelNodes,
                 std::shared_ptr<lua::jbox_panel> const &iPanel);

  void beforeRender(AppContext &iCtx);
  void render(AppContext &iCtx);

  friend class AppContext;

protected:
  void renderWidgets(AppContext &iCtx);
  void renderPanel(AppContext &iCtx);
  void renderPanelWidgets(AppContext &iCtx);
  void renderProperties(AppContext &iCtx);

public:
  Panel fPanel;

private:
  std::vector<WidgetDef> fWidgetDefs{};
  WidgetTypeArray<bool> fAllowedWidgetTypes{};
};

}

#endif //RE_EDIT_PANEL_STATE_H