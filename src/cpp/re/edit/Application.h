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
#include <optional>
#include "fs.h"

namespace re::edit {

class Application
{
public:
  Application() = default;

  std::optional<lua::Config> parseArgs(std::vector<std::string> iArgs);

  bool init(lua::Config const &iConfig,
            std::shared_ptr<TextureManager> iTextureManager,
            std::shared_ptr<NativeFontManager> iNativeFontManager);

  inline void setNativeWindowSize(int iWidth, int iHeight) {
    fAppContext.fNativeWindowWidth = iWidth;
    fAppContext.fNativeWindowHeight = iHeight;
  }

  inline int getNativeWindowWidth() const { return fAppContext.fNativeWindowWidth; }
  inline int getNativeWindowHeight() const { return fAppContext.fNativeWindowHeight; }

  void setDeviceHeightRU(int iDeviceHeightRU);

  inline void onNativeWindowFontDpiScaleChange(float iFontDpiScale) { fAppContext.onNativeWindowFontDpiScaleChange(iFontDpiScale); }
  inline void onNativeWindowFontScaleChange(float iFontScale) { fAppContext.onNativeWindowFontScaleChange(iFontScale); }
//  inline void onNativeWindowPositionChange(int x, int y, float iFontScale, float iFontDpiScale) { fAppContext.onNativeWindowPositionChange(x, y, iFontScale, iFontDpiScale); }

  void newFrame();
  bool render() noexcept;
  void renderMainMenu();
  void renderSavePopup();
  void save();
  void saveConfig();

  constexpr bool hasException() const { return fException != std::nullopt; }

  static void saveFile(fs::path const &iFile, std::string const &iContent);

public:
  float clear_color[4] = {0.45f, 0.55f, 0.60f, 1.00f};

private:
  bool doRenderException();
  bool doRender(); // may throw exception
  std::string hdgui2D();
  std::string device2D() const;

private:
  int fDeviceHeightRU{1};
  AppContext fAppContext{};
  ReGui::Window fMainWindow{"re-edit", std::nullopt, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar};
  bool fShowDemoWindow{false};
  bool fShowMetricsWindow{false};

  fs::path fRoot{};
  bool fSavingRequested{};
  bool fNeedsSaving{};
  bool fRecomputeDimensionsRequested{};
  std::optional<std::string> fNewLayoutRequested{};
  std::optional<std::exception_ptr> fException{};
};

}

#endif //RE_EDIT_APPLICATION_H