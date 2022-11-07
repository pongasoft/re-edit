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
#include "Dialog.h"
#include "fs.h"

namespace re::edit {

class Application
{
public:
  Application();

  static Application &GetCurrent() { RE_EDIT_INTERNAL_ASSERT(kCurrent != nullptr); return *kCurrent; }

  inline AppContext &getAppContext() { return *fAppContext; }
  inline AppContext const &getAppContext() const { return *fAppContext; }

  std::optional<lua::Config> parseArgs(std::vector<std::string> iArgs);

  bool init(lua::Config const &iConfig,
            std::shared_ptr<TextureManager> iTextureManager,
            std::shared_ptr<NativeFontManager> iNativeFontManager);

  inline void setNativeWindowSize(int iWidth, int iHeight) {
    fAppContext->fNativeWindowWidth = iWidth;
    fAppContext->fNativeWindowHeight = iHeight;
  }

  inline int getNativeWindowWidth() const { return fAppContext->fNativeWindowWidth; }
  inline int getNativeWindowHeight() const { return fAppContext->fNativeWindowHeight; }

  inline void onNativeWindowFontDpiScaleChange(float iFontDpiScale) { fAppContext->onNativeWindowFontDpiScaleChange(iFontDpiScale); }
  inline void onNativeWindowFontScaleChange(float iFontScale) { fAppContext->onNativeWindowFontScaleChange(iFontScale); }
//  inline void onNativeWindowPositionChange(int x, int y, float iFontScale, float iFontDpiScale) { fAppContext.onNativeWindowPositionChange(x, y, iFontScale, iFontDpiScale); }

  bool newFrame() noexcept;
  bool render() noexcept;
  void renderMainMenu();
  void maybeExit();
  inline bool running() const { return !fExitRequested; }
  inline void abort() { fExitRequested = true; };
  ReGui::Dialog &newDialog(std::string iTitle, bool iHighPriority = false);
  void newExceptionDialog(std::string iMessage, bool iSaveButton, std::exception_ptr const &iException);

  constexpr bool hasException() const { return fHasException; }

  static void saveFile(fs::path const &iFile, std::string const &iContent);
  static std::string what(std::exception_ptr const &p);

public:
  float clear_color[4] = {0.45f, 0.55f, 0.60f, 1.00f};

private:
  bool doRender(); // may throw exception
  ReGui::Dialog::Result renderDialog();
  void about() const;
  inline bool hasDialog() const { return fCurrentDialog != nullptr || !fDialogs.empty(); }
  template<typename F>
  void executeCatchAllExceptions(F f) noexcept;

private:
  std::shared_ptr<AppContext> fAppContext{};
  bool fShowDemoWindow{false};
  bool fShowMetricsWindow{false};

  bool fExitRequested{};
  bool fHasException{};
  std::vector<std::unique_ptr<ReGui::Dialog>> fDialogs{};
  std::unique_ptr<ReGui::Dialog> fCurrentDialog{};

  inline static Application *kCurrent{};
};

}

#endif //RE_EDIT_APPLICATION_H