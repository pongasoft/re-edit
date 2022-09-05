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
  Application() = default;

  bool parseArgs(std::vector<std::string> iArgs);

  bool init(std::shared_ptr<TextureManager> iTextureManager);

  inline void setNativeWindowSize(int iWidth, int iHeight) {
    fAppContext.fNativeWindowWidth = iWidth;
    fAppContext.fNativeWindowHeight = iHeight;
  }

  inline int getNativeWindowWidth() const { return fAppContext.fNativeWindowWidth; }
  inline int getNativeWindowHeight() const { return fAppContext.fNativeWindowHeight; }

  void setDeviceHeightRU(int iDeviceHeightRU);

  void render();
  void renderMainMenu();
  void renderSavePopup();
  void save();
  void saveConfig();

  static void saveFile(std::string const &iFile, std::string const &iContent);
  [[nodiscard]] static bool fileExists(std::string const &iFile);

public:
  float clear_color[4] = {0.45f, 0.55f, 0.60f, 1.00f};

private:
  std::string hdgui2D();
  std::string device2D() const;


private:
  int fDeviceHeightRU{1};
  AppContext fAppContext{};
  bool fShowDemoWindow{false};
  bool fShowMetricsWindow{false};

  std::string fRoot{};
  bool fSavingRequested{};
};

}

#endif //RE_EDIT_APPLICATION_H