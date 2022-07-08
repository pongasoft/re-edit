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

#ifndef RE_EDIT_APPLICATION_H
#define RE_EDIT_APPLICATION_H

#include "TextureManager.h"
#include "imgui.h"
#include "DrawContext.h"
#include "Panel.h"
#include "EditContext.h"
#include "PropertyManager.h"
#include "lua/HDGui2D.h"
#include "lua/Device2D.h"

namespace re::edit {

class Application
{
public:
  explicit Application(std::shared_ptr<TextureManager> const &iTextureManager);

  void init();

  void render();

public:
  float clear_color[4] = {0.45f, 0.55f, 0.60f, 1.00f};

protected:
  void initPanels(std::string const &iDevice2DFile, std::string const &iHDGui2DFile);
  void initPanel(std::shared_ptr<lua::panel_nodes> const &iPanelNodes,
                 std::shared_ptr<lua::jbox_panel> const &iPanel,
                 Panel &oPanel);

private:
  class PanelState
  {
  public:
    PanelState(Panel::Type iPanelType,
               std::shared_ptr<TextureManager> iTextureManager,
               std::shared_ptr<UserPreferences> iUserPreferences,
               std::shared_ptr<PropertyManager> iPropertyManager);

    void render();

  protected:
    void renderWidgets();
    void renderPanel();
    void renderPanelWidgets();
    void renderProperties();

  public:
    Panel fPanel;

  private:
    class InternalDrawContext : public DrawContext
    {
    public:
      InternalDrawContext(std::shared_ptr<TextureManager> iTextureManager,
                          std::shared_ptr<UserPreferences> iUserPreferences,
                          std::shared_ptr<PropertyManager> iPropertyManager) :
        DrawContext(std::move(iTextureManager), std::move(iUserPreferences), std::move(iPropertyManager))
      {}

      friend class PanelState;
    };

  private:
    InternalDrawContext fDrawContext;
    bool fShowPanel{true};
    bool fShowPanelWidgets{true};
    bool fShowWidgets{};
    bool fShowProperties{};
  };

private:
  std::shared_ptr<TextureManager> fTextureManager;
  std::shared_ptr<UserPreferences> fUserPreferences;
  std::shared_ptr<PropertyManager> fPropertyManager{};
  PanelState fFrontPanel;
  PanelState fBackPanel;
  bool show_demo_window{false};
};

}

#endif //RE_EDIT_APPLICATION_H