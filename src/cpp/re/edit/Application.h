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
#include "Panel.h"
#include "AppContext.h"
#include "PropertyManager.h"
#include "lua/HDGui2D.h"
#include "lua/Device2D.h"
#include "PanelState.h"

namespace re::edit {

class Application
{
public:
  explicit Application(std::shared_ptr<TextureManager> iTextureManager);

  bool init(std::vector<std::string> iArgs);
  void setDeviceHeightRU(int iDeviceHeightRU);

  void render();
  void renderMainMenu();
  void renderSavePopup();
  void save();

public:
  float clear_color[4] = {0.45f, 0.55f, 0.60f, 1.00f};

protected:
  void initPanels(std::string const &iDevice2DFile, std::string const &iHDGui2DFile);

private:
  std::string hdgui2D();
  std::string device2D() const;
  void saveFile(std::string const &iFile, std::string const &iContent) const;

private:
  int fDeviceHeightRU{1};
  AppContext fAppContext;
  PanelState fFrontPanel;
  PanelState fFoldedFrontPanel;
  PanelState fBackPanel;
  PanelState fFoldedBackPanel;
  bool show_demo_window{false};

  std::string fRoot{};
  bool fSavingRequested{};
};

}

#endif //RE_EDIT_APPLICATION_H