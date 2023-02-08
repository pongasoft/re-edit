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

#ifndef RE_EDIT_APP_CONTEXT_H
#define RE_EDIT_APP_CONTEXT_H

#include <vector>
#include <string>
#include <memory>
#include <exception>
#include <atomic>
#include <functional>
#include "TextureManager.h"
#include "FontManager.h"
#include "PreferencesManager.h"
#include "PropertyManager.h"
#include "UndoManager.h"
#include "Constants.h"
#include "Errors.h"
#include "Config.h"
#include "Utils.h"
#include "Canvas.h"
#include "Clipboard.h"

namespace efsw {
class FileWatcher;
class FileWatchListener;
}

namespace re::edit {

class Panel;
class PanelState;
class Widget;
struct WidgetDef;

namespace widget {
class Attribute;
}

class AppContext
{
public:
  enum class EWidgetRendering : int
  {
    kNone,
    kNormal,
    kXRay
  };

  enum class EPanelRendering : int
  {
    kNone,
    kNormal,
    kBorder,
    kXRay
  };

  enum class EBorderRendering : int
  {
    kNone,
    kNormal,
    kHitBoundaries
  };

  enum class ECustomDisplayRendering : int
  {
    kNone,
    kMain,
    kBackgroundSD,
    kBackgroundHD
  };

  enum class ESampleDropZoneRendering : int
  {
    kNone,
    kFill
  };

  enum class ENoGraphicsRendering : int
  {
    kNone,
    kBorder,
    kFill
  };

public:
  AppContext(fs::path const &iRoot, std::shared_ptr<TextureManager> iTextureManager);
  ~AppContext();

  static AppContext &GetCurrent() { RE_EDIT_INTERNAL_ASSERT(kCurrent != nullptr); return *kCurrent; }

  ImVec2 getCurrentPanelSize() const;
  bool renderWidgetDefMenuItems(PanelType iPanelType, std::function<void(WidgetDef const &)> const &iAction);
  bool isWidgetAllowed(PanelType iPanelType, WidgetType iWidgetType) const;
  void renderZoomSelection();
  void renderGridSelection();
  PanelState *getPanelState(PanelType iType) const;
  Panel *getPanel(PanelType iType) const;
  Panel *getCurrentPanel() const;
  inline void setMouseCursorNextFrame(ImGuiMouseCursor iMouseCursor) { fMouseCursor = iMouseCursor; }
  void toggleWidgetRenderingXRay();
  void toggleWidgetBorder();
  void toggleRails();

public: // UserPreferences
  inline UserPreferences const &getUserPreferences() const { return *fUserPreferences; }
  inline UserPreferences &getUserPreferences() { return *fUserPreferences; }

public: // Properties
  inline Object const *findObject(std::string const &iObjectPath) const { return fPropertyManager->findObject(iObjectPath); };
  inline std::vector<Object const *> findObjects(Object::Filter const &iFilter) const { return fPropertyManager->findObjects(iFilter); }

  inline std::vector<Property const *> findProperties(Property::Filter const &iFilter) const { return fPropertyManager->findProperties(iFilter); };
  inline std::vector<std::string> findPropertyNames(Property::Filter const &iFilter) const { return fPropertyManager->findPropertyNames(iFilter); }
  void sortProperties(std::vector<std::string> &ioProperties, Property::Comparator const &iComparator) const { fPropertyManager->sortProperties(ioProperties, iComparator); }
  inline std::vector<Property const *> findProperties() const { return findProperties(Property::Filter{}); }
  inline Property const *findProperty(std::string const &iPropertyPath) const { return fPropertyManager->findProperty(iPropertyPath); };
  inline std::string getPropertyInfo(std::string const &iPropertyPath) const { return fPropertyManager->getPropertyInfo(iPropertyPath); }
  int getPropertyValueAsInt(std::string const &iPropertyPath) const { return fPropertyManager->getValueAsInt(iPropertyPath); }
  void setPropertyValueAsInt(std::string const &iPropertyPath, int iValue) { return fPropertyManager->setValueAsInt(iPropertyPath, iValue); }
  void propertyEditView(std::string const &iPropertyPath) { fPropertyManager->editView(iPropertyPath); }
  void addPropertyToWatchlist(std::string const &iPropertyPath, bool iShowProperties = true) {
    fPropertyManager->addToWatchlist(iPropertyPath);
    if(iShowProperties)
      fPropertiesWindow.setIsVisible(true);
  }
  void removePropertyFromWatchlist(std::string const &iPropertyPath) { fPropertyManager->removeFromWatchlist(iPropertyPath); }
  constexpr int getUserSamplesCount() const { return fPropertyManager->getUserSamplesCount(); }

public: // Clipboard
  inline bool isClipboardMatchesType(clipboard::DataType iType) const { return fClipboard.matchesType(iType); }
  inline std::string const &getClipboardDescription() const { return fClipboard.getData()->getDescription(); }
  void renderClipboardTooltip() const;
  bool isClipboardWidgetAllowedForPanel(PanelType iType) const;
  template<typename T>
  inline T const *getClipboardData() const { return dynamic_cast<T const *>(fClipboard.getData()); }
  void copyToClipboard(Widget const *iWidget, int iAttributeId = -1);
  void copyToClipboard(widget::Attribute const *iAttribute);
  void copyToClipboard(std::vector<Widget *> const &iWidgets);
  bool pasteFromClipboard(Widget *oWidget);
  bool pasteFromClipboard(std::vector<Widget *> const &oWidgets);
  bool pasteFromClipboard(Panel &oPanel, ImVec2 const &iPosition);

public: // Texture
  inline std::vector<FilmStrip::key_t> getTextureKeys() const { return fTextureManager->getTextureKeys(); };
  inline std::vector<FilmStrip::key_t> findTextureKeys(FilmStrip::Filter const &iFilter) const { return fTextureManager->findTextureKeys(iFilter); }
  inline bool checkTextureKeyMatchesFilter(FilmStrip::key_t const &iKey, FilmStrip::Filter const &iFilter) const { return fTextureManager->checkTextureKeyMatchesFilter(iKey, iFilter); }
  inline std::shared_ptr<Texture> getTexture(FilmStrip::key_t const &iKey) const { return fTextureManager->getTexture(iKey); };
  std::shared_ptr<Texture> getBuiltInTexture(FilmStrip::key_t const &iKey) const;
  inline std::shared_ptr<Texture> findTexture(FilmStrip::key_t const &iKey) const { return fTextureManager->findTexture(iKey); };
  inline std::shared_ptr<Texture> findHDTexture(FilmStrip::key_t const &iKey) const { return fTextureManager->findHDTexture(iKey); }
  int overrideTextureNumFramesAction(FilmStrip::key_t const &iKey, int iNumFrames);
  void overrideTextureNumFrames(FilmStrip::key_t const &iKey, int iNumFrames);
  std::optional<FilmStrip::key_t> importTextureBlocking();
  std::size_t importTexturesBlocking();

public: // Undo
  constexpr bool isUndoEnabled() const { return fUndoManager->isEnabled(); }
  constexpr void enableUndo() { fUndoManager->enable(); }
  constexpr void disableUndo() { fUndoManager->disable(); }

  template<typename F>
  inline void withUndoDisabled(F &&f)
  {
    auto undoEnabled = isUndoEnabled();
    if(undoEnabled)
      disableUndo();
    f();
    if(undoEnabled)
      enableUndo();
  }

  void addUndo(std::unique_ptr<Action> iAction);
  void beginUndoTx(std::string iDescription, MergeKey const &iMergeKey = MergeKey::none());
  void commitUndoTx();
  void rollbackUndoTx();
  void setNextUndoActionDescription(std::string iDescription);

  template<typename R>
  R execute(std::unique_ptr<ExecutableAction<R>> iAction);

  template<class T, class... Args >
  typename T::result_t executeAction(Args&&... args);

  void resetUndoMergeKey();

  void undoLastAction();
  void redoLastAction();

  inline Widget const *getCurrentWidget() const { RE_EDIT_INTERNAL_ASSERT(fCurrentWidget != nullptr); return fCurrentWidget; }
  inline Widget *getCurrentWidget() { RE_EDIT_INTERNAL_ASSERT(fCurrentWidget != nullptr); return const_cast<Widget *>(fCurrentWidget); }

  friend class PanelState;
  friend class Widget;
  friend class Application;

  inline bool maybeReloadTextures() const { return fMaybeReloadTextures; }
  inline bool maybeReloadDevice() const { return fMaybeReloadDevice; }
  inline bool hasNotifications() const { return maybeReloadDevice() || maybeReloadTextures(); }

  void maybeReloadTextures(bool b) { fMaybeReloadTextures = b; }
  void maybeReloadDevice(bool b) { fMaybeReloadDevice = b; }

  std::string getDeviceName() const;
  constexpr bool hasFoldedPanels() const { return fHasFoldedPanels; }

public:
  EWidgetRendering fWidgetRendering{EWidgetRendering::kNormal};
  EPanelRendering fPanelRendering{EPanelRendering::kNormal};
  EBorderRendering fBorderRendering{EBorderRendering::kNone};
  ECustomDisplayRendering fCustomDisplayRendering{ECustomDisplayRendering::kMain};
  ESampleDropZoneRendering fSampleDropZoneRendering{ESampleDropZoneRendering::kFill};
  ENoGraphicsRendering fNoGraphicsRendering{ENoGraphicsRendering::kFill};
  bool fShowFoldButton{true};
  bool fShowRackRails{false};

  constexpr float getZoom() const { return fDpiAdjustedZoom; }
  constexpr bool isZoomFitContent() const { return fZoomFitContent; }

  ImVec2 fGrid{1.0f, 1.0f};
  float fItemWidth{300.0f};

  inline static thread_local AppContext *kCurrent{};

protected:
  void init(config::Device const &iConfig);
  config::Device getConfig() const;
  void reloadTextures();
  void markEdited();
  bool checkForErrors();
  bool computeErrors();
  bool computeErrors(PanelType iType);
  void renderErrors();
  void renderErrors(Panel const &iPanel);
  void renderUndoHistory();
  void initDevice();
  void initGUI2D(Utils::CancellableSPtr const &iCancellable);
  void reloadDevice();
  void save();
  void importBuiltIns(UserError *oErrors = nullptr);
  std::string hdgui2D() const;
  std::string device2D() const;
  std::string cmake() const;
  std::optional<std::string> getReEditVersion() const { return fReEditVersion; }

  void initPanels(fs::path const &iDevice2DFile,
                  fs::path const &iHDGui2DFile,
                  Utils::CancellableSPtr const &iCancellable);
  inline void setCurrentWidget(Widget const *iWidget) { fCurrentWidget = iWidget; }
  inline ReGui::Canvas &getPanelCanvas() { return fPanelCanvas; }
  void newFrame();
  void beforeRenderFrame();
  void afterRenderFrame();
  void renderMainMenu();
  void renderTabs();
  void render();
  void handleKeyboardShortcuts();
  void setUserZoom(float iZoom);
  void setZoom(ReGui::Canvas::Zoom const &iZoom);
  void requestZoomToFit() { fZoomFitContent = true; }
  void incrementZoom();
  void decrementZoom();
  constexpr bool needsSaving() const { return fNeedsSaving; }

  void enableFileWatcher();
  void disableFileWatcher();

protected:
  fs::path fRoot;
  ReGui::Window fMainWindow{"re-edit", std::nullopt, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar};
  std::shared_ptr<TextureManager> fTextureManager{};
  std::shared_ptr<UserPreferences> fUserPreferences{};
  std::shared_ptr<PropertyManager> fPropertyManager{};
  std::shared_ptr<UndoManager> fUndoManager{};
  std::unique_ptr<UndoTx> fUndoTx{};
  std::vector<std::unique_ptr<UndoTx>> fNestedUndoTxs{};
  std::vector<std::string> fNextUndoActionDescriptions{};
  std::unique_ptr<PanelState> fFrontPanel;
  std::unique_ptr<PanelState> fFoldedFrontPanel;
  std::unique_ptr<PanelState> fBackPanel;
  std::unique_ptr<PanelState> fFoldedBackPanel;
  bool fHasFoldedPanels{};
  float fUserZoom{0.20f};
  float fDpiAdjustedZoom{fUserZoom};
  bool fZoomFitContent{true};
  std::optional<std::string> fReEditVersion{};
  ReGui::Window fPanelWindow{"Panel", true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse};
  ReGui::Window fPanelWidgetsWindow{"Panel Widgets", true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoCollapse};
  ReGui::Window fWidgetsWindow{"Widgets", true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoCollapse};
  ReGui::Window fPropertiesWindow{"Properties", true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoCollapse};
  ReGui::Window fUndoHistoryWindow{"Undo History", true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoCollapse};
  long fCurrentFrame{};
  PanelState *fCurrentPanelState{};
  PanelState *fPreviousPanelState{};
  Widget const *fCurrentWidget{};
  ReGui::Canvas fPanelCanvas{};
  Clipboard fClipboard{};
  bool fNeedsSaving{};
  void *fLastSavedUndoAction{};
  bool fRecomputeDimensionsRequested{true};
  bool fReloadTexturesRequested{};
  std::atomic<bool> fMaybeReloadTextures{};
  bool fReloadDeviceRequested{};
  std::atomic<bool> fMaybeReloadDevice{};
  std::optional<std::string> fNewLayoutRequested{};
  ImGuiMouseCursor fMouseCursor{ImGuiMouseCursor_None};

  std::shared_ptr<efsw::FileWatcher> fRootWatcher{};
  std::shared_ptr<efsw::FileWatchListener> fRootListener{};
  std::optional<long> fRootWatchID{};
};

//------------------------------------------------------------------------
// AppContext::execute
//------------------------------------------------------------------------
template<>
void AppContext::execute<void>(std::unique_ptr<ExecutableAction<void>> iAction);

}
#endif //RE_EDIT_APP_CONTEXT_H
