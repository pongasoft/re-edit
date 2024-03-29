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
#include "NetworkManager.h"
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
#include "Notification.h"
#include <version.h>
#include <future>
#include <map>

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
    virtual std::shared_ptr<TextureManager> newTextureManager() const {  return std::make_shared<TextureManager>(); };
    virtual std::shared_ptr<NetworkManager> newNetworkManager() const = 0;
    virtual ImVec4 getWindowPositionAndSize() const = 0;
    virtual void setWindowPositionAndSize(std::optional<ImVec2> const &iPosition, ImVec2 const &iSize) const = 0;
    virtual ImVec2 getRenderScale() const = 0;
    virtual void centerWindow() const = 0;
    virtual void setWindowTitle(std::string const &iTitle) const = 0;
    virtual void openURL(std::string const &iURL) const = 0;
    virtual void setTargetFrameRate(int iFrameRate) const = 0;
    virtual void setVSyncEnabled(bool iEnabled) const = 0;

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

  using gui_action_t = std::function<void()>;

public:
  explicit Application(std::shared_ptr<Context> iContext);
  Application(std::shared_ptr<Context> iContext, Application::Config const &iConfig);
  bool shutdown(long iTimeoutMillis);
  ~Application();

  static Application &GetCurrent() { RE_EDIT_INTERNAL_ASSERT(kCurrent != nullptr); return *kCurrent; }
  static bool HasCurrent() { return kCurrent != nullptr; }

  static Config parseArgs(NativePreferencesManager const *iPreferencesManager, std::vector<std::string> iArgs);

  void loadProject(fs::path const &iRoot);
  void loadProjectDeferred(fs::path const &iRoot);
  void maybeLoadProject(fs::path const &iProjectRoot);
  void maybeSaveProject(gui_action_t const &iNextAction = {});
  void saveProject();
  void closeProject();
  void maybeCloseProject(std::optional<std::string> const &iDialogTitle = {}, gui_action_t const &iNextAction = {});
  void asyncCheckForUpdates();
  inline bool hasNewVersion() const { return fLatestRelease && fLatestRelease->fVersion != std::string(kGitTag); }
  void downloadLatestVersion() const;

  inline float getCurrentFontSize() const { return fFontManager->getCurrentFont().fSize; }
  inline float getCurrentFontDpiScale() const { return fFontManager->getCurrentFontDpiScale(); }
  std::shared_ptr<Texture> getLogo() const;
  inline std::shared_ptr<Texture> getTexture(FilmStrip::key_t const &iKey) const { return fTextureManager->getTexture(iKey); }
  Icon getDeviceTypeIcon(config::Device const &iDevice) const;
  inline ImVec2 getRenderScale() const { return fContext->getRenderScale(); };

  constexpr int getTargetFrameRate() const { return fConfig.fTargetFrameRate; }
  constexpr bool isVSyncEnabled() const { return fConfig.fVSyncEnabled; }
  constexpr bool isShowPerformance() const { return fConfig.fShowPerformance; }

  void onNativeWindowFontDpiScaleChange(float iFontDpiScale);
  void onNativeWindowFontScaleChange(float iFontScale);
  void onNativeDropFiles(std::vector<fs::path> const &iPaths);
//  inline void onNativeWindowPositionChange(int x, int y, float iFontScale, float iFontDpiScale) { fAppContext.onNativeWindowPositionChange(x, y, iFontScale, iFontDpiScale); }

  bool newFrame(std::vector<gui_action_t> const &iFrameActions) noexcept;
  bool render() noexcept;
  void renderMainMenu();
  void maybeExit();
  inline bool running() const { return fState != State::kDone; }
  void exit();
  ReGui::Dialog &newDialog(std::string iTitle, bool iHighPriority = false);
  void newExceptionDialog(std::string iMessage, bool iSaveButton, std::exception_ptr const &iException);
  ReGui::Notification &newNotification();
  ReGui::Notification &newUniqueNotification(ReGui::Notification::Key const &iKey);
  void savePreferences(UserError *oErrors = nullptr) noexcept;
  AppContext *getAppContext() const { return fAppContext.get(); }

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

  template<typename T>
  struct CancellableFuture
  {
    template< class Function, class... Args >
    void launch(Function&& f, Args&&... args) { fFuture = std::async(std::launch::async, std::forward<Function>(f), std::forward<Args>(args)...); }

    inline void cancel() { fCancellable->cancel(); }
    inline bool cancelled() const { return fCancellable->cancelled(); }
    inline std::pair<int, std::string> progress() const { return fCancellable->progress(); }

    std::shared_ptr<Utils::Cancellable> fCancellable{std::make_shared<Utils::Cancellable>()};
    std::future<T> fFuture{};
  };

private:
  void init();
  std::shared_ptr<AppContext> initAppContext(fs::path const &iRoot, config::Device const &iConfig, Utils::CancellableSPtr const &iCancellable);
  void renderWelcome(); // may throw exception
  void renderAppContext(); // may throw exception
  void renderLoading(); // may throw exception
  void renderDialog();
  void renderNotifications(ImVec2 const &iOffset);
  void renderLogoBox(float iPadding = 10.0f);
  void renderApplicationMenuItems();
  void newAboutDialog();
  void about() const;
  void newHelpDialog();
  void help() const;
  inline bool hasDialog() const { return fCurrentDialog != nullptr || !fDialogs.empty(); }
  template<typename F>
  void executeAndAbortOnException(F&& f) noexcept;
  template<typename R, typename F>
  R executeAndLogOnException(F&& f) noexcept;
  void renderLoadDialogBlocking();
  void deferNextFrame(gui_action_t iAction) { if(iAction) fNewFrameActions.emplace_back(std::move(iAction)); }
  void applyConfigStyle() const;

  template<class Function, class... Args>
  bool async(std::string const &iKey, Function&& f, Args&&... args);
  bool hasAsyncAction(std::string const &iKey) const { return fAsyncActions.find(iKey) != fAsyncActions.end(); }

  void handleNewFrameActions();
  void handleAsyncActions();
  void handleFontChangeRequest();

private:
  std::thread::id fGUIThreadID{};
  State fState{State::kNoReLoaded};
  std::shared_ptr<Context> fContext;
  std::shared_ptr<TextureManager> fTextureManager;
  std::shared_ptr<FontManager> fFontManager;
  std::shared_ptr<NetworkManager> fNetworkManager;
  config::Global fConfig{};
  std::optional<Release> fLatestRelease{};
  std::shared_ptr<AppContext> fAppContext{};
  bool fShowDemoWindow{false};
  bool fShowMetricsWindow{false};

  std::unique_ptr<CancellableFuture<gui_action_t>> fReLoadingFuture{};
  std::map<std::string, std::future<gui_action_t>> fAsyncActions{};
  std::vector<gui_action_t> fNewFrameActions{};
  std::vector<std::unique_ptr<ReGui::Dialog>> fDialogs{};
  std::vector<std::unique_ptr<ReGui::Notification>> fNotifications{};
  std::unique_ptr<ReGui::Dialog> fCurrentDialog{};
  std::unique_ptr<ReGui::Dialog> fWelcomeDialog{};

  inline static thread_local Application *kCurrent{};
};

}

#endif //RE_EDIT_APPLICATION_H