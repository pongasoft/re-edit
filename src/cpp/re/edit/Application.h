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
#include "Config.h"
#include <future>

namespace re::edit {

class Application
{
public:
  class Context
  {
  public:
    explicit Context(std::shared_ptr<NativePreferencesManager> iPreferencesManager) : fPreferencesManager{std::move(iPreferencesManager)} {}
    explicit Context(bool iHeadless) : fHeadless{iHeadless} {}

    virtual ~Context() = default;
    virtual std::shared_ptr<TextureManager> newTextureManager() const = 0;
    virtual std::shared_ptr<NativeFontManager> newNativeFontManager() const = 0;
    virtual ImVec4 getWindowPositionAndSize() const = 0;
    virtual void setWindowPositionAndSize(std::optional<ImVec2> const &iPosition, ImVec2 const &iSize) const = 0;
    virtual void centerWindow() const = 0;
    virtual void setWindowTitle(std::string const &iTitle) const = 0;

    std::shared_ptr<NativePreferencesManager> getPreferencesManager() const { return fPreferencesManager; }

    constexpr bool isHeadless() const { return fHeadless; }

  private:
    bool fHeadless{};
    std::shared_ptr<NativePreferencesManager> fPreferencesManager;
  };

  struct Config
  {
    config::Global fGlobalConfig{};
    std::optional<fs::path> fProjectRoot{};
  };

public:
  explicit Application(std::shared_ptr<Context> iContext);
  Application(std::shared_ptr<Context> iContext, Application::Config const &iConfig);
  ~Application() { kCurrent = nullptr; }

  static Application &GetCurrent() { RE_EDIT_INTERNAL_ASSERT(kCurrent != nullptr); return *kCurrent; }

  inline AppContext &getAppContext() { RE_EDIT_INTERNAL_ASSERT(fAppContext != nullptr); return *fAppContext; }
  inline AppContext const &getAppContext() const { RE_EDIT_INTERNAL_ASSERT(fAppContext != nullptr); return *fAppContext; }

  static Config parseArgs(NativePreferencesManager const *iPreferencesManager, std::vector<std::string> iArgs);

  void loadProject(fs::path const &iRoot);
  void loadProjectDeferred(fs::path const &iRoot);
  void closeProjectDeferred();
  void maybeCloseProject();

  inline float getCurrentFontSize() const { return fFontManager->getCurrentFont().fSize; }
  inline float getCurrentFontDpiScale() const { return fFontManager->getCurrentFontDpiScale(); }
  std::shared_ptr<Texture> getLogo() const;

  void onNativeWindowFontDpiScaleChange(float iFontDpiScale);
  void onNativeWindowFontScaleChange(float iFontScale);
//  inline void onNativeWindowPositionChange(int x, int y, float iFontScale, float iFontDpiScale) { fAppContext.onNativeWindowPositionChange(x, y, iFontScale, iFontDpiScale); }

  bool newFrame() noexcept;
  bool render() noexcept;
  void renderMainMenu();
  void maybeExit();
  inline bool running() const { return fState != State::kDone; }
  void exit();
  ReGui::Dialog &newDialog(std::string iTitle, bool iHighPriority = false);
  void newExceptionDialog(std::string iMessage, bool iSaveButton, std::exception_ptr const &iException);
  void savePreferences(UserError *oErrors = nullptr) noexcept;

  constexpr bool hasException() const { return fState == State::kException; }

  static void saveFile(fs::path const &iFile, std::string const &iContent, UserError *oErrors = nullptr);
  static std::optional<std::string> readFile(fs::path const &iFile, UserError *oErrors = nullptr);
  static std::string what(std::exception_ptr const &p);

public:
  float clear_color[4] = {0.55f, 0.55f, 0.55f, 1.00f};

private:
  enum class State
  {
    kNone,
    kNoReLoaded,
    kReLoading,
    kReLoaded,
    kException,
    kDone
  };

private:
  using gui_action_t = std::function<void()>;

  void init();
  void initAppContext(fs::path const &iRoot, config::Device const &iConfig);
  void renderWelcome(); // may throw exception
  void renderAppContext(); // may throw exception
  void renderLoading(); // may throw exception
  ReGui::Dialog::Result renderDialog();
  void about() const;
  inline bool hasDialog() const { return fCurrentDialog != nullptr || !fDialogs.empty(); }
  template<typename F>
  void executeCatchAllExceptions(F f) noexcept;
  void renderLoadDialogBlocking();
  void deferNextFrame(std::function<void()> iAction) { fNewFrameActions.emplace_back(std::move(iAction)); }

private:
  State fState{State::kNoReLoaded};
  std::shared_ptr<Context> fContext;
  std::shared_ptr<TextureManager> fTextureManager;
  std::shared_ptr<FontManager> fFontManager;
  config::Global fConfig{};
  std::shared_ptr<AppContext> fAppContext{};
  bool fShowDemoWindow{false};
  bool fShowMetricsWindow{false};

  std::unique_ptr<std::future<gui_action_t>> fReLoadingFuture{};
  std::vector<gui_action_t> fNewFrameActions{};
  std::vector<std::unique_ptr<ReGui::Dialog>> fDialogs{};
  std::unique_ptr<ReGui::Dialog> fCurrentDialog{};
  std::unique_ptr<ReGui::Dialog> fWelcomeDialog{};

  inline static Application *kCurrent{};
};

}

#endif //RE_EDIT_APPLICATION_H