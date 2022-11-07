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
#include "TextureManager.h"
#include "FontManager.h"
#include "UserPreferences.h"
#include "PropertyManager.h"
#include "UndoManager.h"
#include "Constants.h"
#include "Errors.h"
#include "lua/ReEdit.h"

namespace re::edit {

class Panel;
class PanelState;
class Widget;

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
  explicit AppContext(fs::path iRoot);

  static AppContext &GetCurrent();

  ImVec2 getCurrentPanelSize() const;
  void renderAddWidgetMenuView(ImVec2 const &iPosition = {});
  PanelState *getPanelState(PanelType iType) const;
  Panel *getPanel(PanelType iType) const;
  Panel *getCurrentPanel() const;

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
  int getPropertyValueAsInt(std::string const &iPropertyPath) const { return fPropertyManager->getIntValue(iPropertyPath); }
  void setPropertyValueAsInt(std::string const &iPropertyPath, int iValue) { return fPropertyManager->setIntValue(iPropertyPath, iValue); }
  void propertyEditView(std::string const &iPropertyPath) { fPropertyManager->editView(iPropertyPath); }
  void addPropertyToWatchlist(std::string const &iPropertyPath, bool iShowProperties = true) {
    fPropertyManager->addToWatchlist(iPropertyPath);
    if(iShowProperties)
      fPropertiesWindow.setIsVisible(true);
  }
  void removePropertyFromWatchlist(std::string const &iPropertyPath) { fPropertyManager->removeFromWatchlist(iPropertyPath); }
  constexpr int getUserSamplesCount() const { return fPropertyManager->getUserSamplesCount(); }

public: // Texture
  inline std::vector<std::string> const &getTextureKeys() const { return fTextureManager->getTextureKeys(); };
  inline std::vector<std::string> findTextureKeys(FilmStrip::Filter const &iFilter) const { return fTextureManager->findTextureKeys(iFilter); }
  inline std::shared_ptr<Texture> getTexture(std::string const &iKey) const { return fTextureManager->getTexture(iKey); };
  inline std::shared_ptr<Texture> getHDTexture(std::string const &iKey) const { return fTextureManager->getHDTexture(iKey); }
  void overrideTextureNumFrames(std::string const &iKey, int iNumFrames) { fTextureManager->overrideNumFrames(iKey, iNumFrames); markEdited(); }

  void TextureItem(Texture const *iTexture,
                   ImVec2 const &iPosition = {0,0},
                   int iFrameNumber = 0,
                   ImU32 iBorderColor = ReGui::kTransparentColorU32,
                   ImU32 iTextureColor = ReGui::kWhiteColorU32) const;

  void drawTexture(Texture const *iTexture,
                   ImVec2 const &iPosition = {0,0},
                   int iFrameNumber = 0,
                   ImU32 iBorderColor = ReGui::kTransparentColorU32,
                   ImU32 iTextureColor = ReGui::kWhiteColorU32) const;

  void drawRect(ImVec2 const &iPosition, ImVec2 const &iSize, ImU32 iColor) const;
  void RectFilledItem(ImVec2 const &iPosition, ImVec2 const &iSize, ImU32 iColor, float iRounding = 0.0f, ImDrawFlags iFlags = 0) const;
  void drawRectFilled(ImVec2 const &iPosition, ImVec2 const &iSize, ImU32 iColor, float iRounding = 0.0f, ImDrawFlags iFlags = 0) const;
  void drawLine(const ImVec2& iP1, const ImVec2& iP2, ImU32 iColor, float iThickness = 1.0f) const;

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

  void addUndoAction(std::shared_ptr<UndoAction> iAction);

  template<typename T, typename UndoLambda, typename RedoLambda>
  inline void addOrMergeUndoLambdas(void *iMergeKey, T const &iOldValue, T const &iNewValue, std::string const &iDescription, UndoLambda u, RedoLambda r)
  {
    addOrMergeUndoAction(iMergeKey, iOldValue, iNewValue, iDescription, [&iOldValue, &iNewValue, u = std::move(u), r = std::move(r)] {
      return std::make_shared<LambdaMergeableUndoAction<T, UndoLambda, RedoLambda>>(iOldValue, iNewValue, std::move(u), std::move(r));
    });
  }

  template<typename T, typename UndoRedoLambda>
  inline void addOrMergeUndoLambda(void *iMergeKey, T const &iOldValue, T const &iNewValue, std::string const &iDescription, UndoRedoLambda u)
  {
    addOrMergeUndoLambdas(iMergeKey, iOldValue, iNewValue, iDescription, u, [u](RedoAction *iAction, T const &iValue) { u(iAction->fUndoAction.get(), iValue); });
  }

  template<typename T, typename UndoRedoLambda>
  inline void addUndoLambda(T const &iOldValue, T const &iNewValue, std::string const &iDescription, UndoRedoLambda u)
  {
    addOrMergeUndoLambda(nullptr, iOldValue, iNewValue, iDescription, std::move(u));
  }

  void addUndoWidgetChange(Widget const *iWidget, std::string iDescription);
  inline void addUndoCurrentWidgetChange(std::string iDescription) { addUndoWidgetChange(fCurrentWidget, std::move(iDescription)); }

  template<typename T>
  void addOrMergeUndoWidgetChange(Widget const *iWidget, void *iMergeKey, T const &iOldValue, T const &iNewValue, std::string const &iDescription)
  {
    addOrMergeUndoAction(iMergeKey, iOldValue, iNewValue, iDescription, [this, iWidget]() {
      auto action = std::make_unique<MergeableWidgetUndoAction<T>>();
      populateWidgetUndoAction(action.get(), iWidget);
      return action;
    });
  }

  template<typename T>
  inline void addOrMergeUndoCurrentWidgetChange(void *iMergeKey, T const &iOldValue, T const &iNewValue, std::string const &iDescription)
  {
    addOrMergeUndoWidgetChange(fCurrentWidget, iMergeKey, iOldValue, iNewValue, iDescription);
  }

  inline void addUndoAttributeChange(widget::Attribute const *iAttribute)
  {
    RE_EDIT_INTERNAL_ASSERT(fCurrentWidget != nullptr);
    return addUndoCurrentWidgetChange(std::move(computeUpdateDescription(fCurrentWidget, iAttribute)));
  }

  inline void addUndoAttributeReset(widget::Attribute const *iAttribute)
  {
    RE_EDIT_INTERNAL_ASSERT(fCurrentWidget != nullptr);
    return addUndoCurrentWidgetChange(std::move(computeResetDescription(fCurrentWidget, iAttribute)));
  }

  template<typename T>
  inline void addOrMergeUndoAttributeChange(widget::Attribute const *iAttribute,
                                            T const &iOldValue,
                                            T const &iNewValue)
  {
    RE_EDIT_INTERNAL_ASSERT(fCurrentWidget != nullptr);
    addOrMergeUndoCurrentWidgetChange(const_cast<widget::Attribute *>(iAttribute),
                                      iOldValue,
                                      iNewValue,
                                      computeUpdateDescription(fCurrentWidget, iAttribute));
  }

  bool beginUndoTx(std::string const &iDescription, void *iMergeKey = nullptr);
  void commitUndoTx();

  void resetUndoMergeKey();

  void undoLastAction() { fUndoManager->undoLastAction(); }
  void redoLastAction() { fUndoManager->redoLastAction(); }

  inline Widget const *getCurrentWidget() const { return fCurrentWidget; }

  friend class PanelState;
  friend class Widget;
  friend class Application;

public:
  EWidgetRendering fWidgetRendering{EWidgetRendering::kNormal};
  EBorderRendering fBorderRendering{EBorderRendering::kNone};
  ECustomDisplayRendering fCustomDisplayRendering{ECustomDisplayRendering::kMain};
  ESampleDropZoneRendering fSampleDropZoneRendering{ESampleDropZoneRendering::kFill};
  ENoGraphicsRendering fNoGraphicsRendering{ENoGraphicsRendering::kFill};

  float fZoom{0.20f};
  ImVec2 fGrid{1.0f, 1.0f};
  float fItemWidth{300.0f};

protected:
  void init(lua::Config const &iConfig);
  std::string getLuaConfig() const;
  void reloadTextures();
  void markEdited();
  bool checkForErrors();
  void initDevice();
  void initGUI2D();
  void reloadDevice();
  void save();
  void saveConfig();
  std::string hdgui2D() const;
  std::string device2D() const;

  void initPanels(fs::path const &iDevice2DFile, fs::path const &iHDGui2DFile);
  void onNativeWindowFontDpiScaleChange(float iFontDpiScale);
  void onNativeWindowFontScaleChange(float iFontScale);
  inline void setCurrentWidget(Widget const *iWidget) { fCurrentWidget = iWidget; }
  void newFrame();
  void beforeRenderFrame();
  void afterRenderFrame();
  void renderMainMenu();
  void renderTabs();
  void render();
  void populateWidgetUndoAction(WidgetUndoAction *iAction, Widget const *iWidget);
  constexpr bool needsSaving() const { return fNeedsSaving; }

  template<typename T, typename F>
  void addOrMergeUndoAction(void *iMergeKey,
                            T const &iOldValue,
                            T const &iNewValue,
                            std::string const &iDescription,
                            F iUndoActionFactory);

  static std::string computeUpdateDescription(Widget const *iWidget, widget::Attribute const *iAttribute);
  static std::string computeResetDescription(Widget const *iWidget, widget::Attribute const *iAttribute);

protected:
  fs::path fRoot;
  ReGui::Window fMainWindow{"re-edit", std::nullopt, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar};
  std::shared_ptr<TextureManager> fTextureManager{};
  std::shared_ptr<FontManager> fFontManager{};
  std::shared_ptr<UserPreferences> fUserPreferences{};
  std::shared_ptr<PropertyManager> fPropertyManager{};
  std::shared_ptr<UndoManager> fUndoManager{};
  std::shared_ptr<CompositeUndoAction> fUndoTransaction{};
  std::unique_ptr<PanelState> fFrontPanel;
  std::unique_ptr<PanelState> fFoldedFrontPanel;
  std::unique_ptr<PanelState> fBackPanel;
  std::unique_ptr<PanelState> fFoldedBackPanel;
  ReGui::Window fPanelWindow{"Panel", true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoCollapse};
  ReGui::Window fPanelWidgetsWindow{"Panel Widgets", true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoCollapse};
  ReGui::Window fWidgetsWindow{"Widgets", true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoCollapse};
  ReGui::Window fPropertiesWindow{"Properties", true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoCollapse};
  long fCurrentFrame{};
  PanelState *fCurrentPanelState{};
  PanelState *fPreviousPanelState{};
  Widget const *fCurrentWidget{};
  bool fNeedsSaving{};
  int fNativeWindowWidth{1280};
  int fNativeWindowHeight{720};
  bool fRecomputeDimensionsRequested{};
  bool fReloadTexturesRequested{};
  bool fReloadDeviceRequested{};
  std::optional<std::string> fNewLayoutRequested{};
};

//------------------------------------------------------------------------
// AppContext::addOrMergeUndoAction
//------------------------------------------------------------------------
template<typename T, typename F>
void AppContext::addOrMergeUndoAction(void *iMergeKey,
                                      T const &iOldValue,
                                      T const &iNewValue,
                                      std::string const &iDescription,
                                      F iUndoActionFactory)
{
  auto lastUndoAction = fUndoManager->getLastUndoAction();

  if(iMergeKey != nullptr && lastUndoAction && lastUndoAction->getMergeKey() == iMergeKey)
  {
    // at the minimum it is a merge => we won't add a new action
    auto mua = dynamic_cast<MergeableUndoValue<T> *>(lastUndoAction.get());

    if(mua)
    {
      if(mua->fOldValue == iNewValue)
        // it's a cancel since the old value matches the new value
        fUndoManager->popLastUndoAction();
      else
        // new value needs to be updated
        mua->fNewValue = iNewValue;
    }
  }
  else
  {
    auto action = iUndoActionFactory();
    action->fDescription = iDescription;
    action->fMergeKey = iMergeKey;
    action->fOldValue = iOldValue;
    action->fNewValue = iNewValue;
    addUndoAction(std::move(action));
  }
}

}
#endif //RE_EDIT_APP_CONTEXT_H
