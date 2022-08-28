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
#include "TextureManager.h"
#include "UserPreferences.h"
#include "PropertyManager.h"
#include "UndoManager.h"
#include "Constants.h"

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
  enum class ShowBorder
  {
    kNone,
    kWidget,
    kHitBoundaries
  };

  enum class ShowCustomDisplay
  {
    kNone,
    kMain,
    kBackgroundSD,
    kBackgroundHD
  };

  enum class ShowSampleDropZone
  {
    kNone,
    kFill
  };

public:
  AppContext();

  ImVec2 getPanelSize() const;
  void renderAddWidgetMenuView(ImVec2 const &iPosition = {});
  PanelState *getPanelState(PanelType iType) const;
  Panel *getPanel(PanelType iType) const;

public: // UserPreferences
  constexpr UserPreferences const &getUserPreferences() const { return *fUserPreferences; }
  constexpr UserPreferences &getUserPreferences() { return *fUserPreferences; }

public: // Properties
  inline std::vector<Object const *> findObjects(Object::Filter const &iFilter) const { return fPropertyManager->findObjects(iFilter); }

  inline std::vector<Property const *> findProperties(Property::Filter const &iFilter) const { return fPropertyManager->findProperties(iFilter); };
  inline std::vector<std::string> findPropertyNames(Property::Filter const &iFilter) const { return fPropertyManager->findPropertyNames(iFilter); }
  inline std::vector<Property const *> findProperties() const { return findProperties(Property::Filter{}); }
  inline Property const *findProperty(std::string const &iPropertyPath) const { return fPropertyManager->findProperty(iPropertyPath); };
  inline std::string getPropertyInfo(std::string const &iPropertyPath) const { return fPropertyManager->getPropertyInfo(iPropertyPath); }
  int getPropertyValueAsInt(std::string const &iPropertyPath) const { return fPropertyManager->getIntValue(iPropertyPath); }
  void propertyEditView(std::string const &iPropertyPath) { fPropertyManager->editView(iPropertyPath); }
  void addPropertyToWatchlist(std::string const &iPropertyPath, bool iShowProperties = true) {
    fPropertyManager->addToWatchlist(iPropertyPath);
    fShowProperties = iShowProperties;
  }
  void removePropertyFromWatchlist(std::string const &iPropertyPath) { fPropertyManager->removeFromWatchlist(iPropertyPath); }
  constexpr int getUserSamplesCount() const { return fPropertyManager->getUserSamplesCount(); }

public: // Texture
  inline std::vector<std::string> const &getTextureKeys() const { return fTextureManager->getTextureKeys(); };
  inline std::vector<std::string> findTextureKeys(FilmStrip::Filter const &iFilter) const { return fTextureManager->findTextureKeys(iFilter); }
  inline std::shared_ptr<Texture> getTexture(std::string const &iKey) const { return fTextureManager->getTexture(iKey); };
  inline std::shared_ptr<Texture> findTexture(std::string const &iKey) const { return fTextureManager->findTexture(iKey); }
  inline std::shared_ptr<Texture> findHDTexture(std::string const &iKey) const { return fTextureManager->findHDTexture(iKey); }

  void TextureItem(Texture const *iTexture, ImVec2 const &iPosition = {0,0}, int iFrameNumber = 0, const ImVec4& iBorderCol = ImVec4(0,0,0,0)) const;

  void drawTexture(Texture const *iTexture, ImVec2 const &iPosition = {0,0}, int iFrameNumber = 0, const ImVec4& iBorderCol = ImVec4(0,0,0,0)) const;
  void drawRect(ImVec2 const &iPosition, ImVec2 const &iSize, ImU32 iColor) const;
  void drawRectFilled(ImVec2 const &iPosition, ImVec2 const &iSize, ImU32 iColor, float iRounding = 0.0f, ImDrawFlags iFlags = 0) const;
  inline void drawRectFilled(ImVec2 const &iPosition, ImVec2 const &iSize, const ImVec4& iColor, float iRounding = 0.0f, ImDrawFlags iFlags = 0) const {
    drawRectFilled(iPosition, iSize, ImGui::GetColorU32(iColor), iRounding, iFlags);
  }
  void drawRect(ImVec2 const &iPosition, ImVec2 const &iSize, const ImVec4& iColor) const { drawRect(iPosition, iSize, ImGui::GetColorU32(iColor)); }
  void drawLine(const ImVec2& iP1, const ImVec2& iP2, ImU32 iColor, float iThickness = 1.0f) const;
  inline void drawLine(const ImVec2& iP1, const ImVec2& iP2, const ImVec4& iColor, float iThickness = 1.0f) const { drawLine(iP1, iP2, ImGui::GetColorU32(iColor), iThickness); }

public: // Undo
  void addUndoAction(std::shared_ptr<UndoAction> iAction);
  inline void beginUndoTx(std::string iDescription) { fUndoManager->beginUndoTx(fCurrentFrame, std::move(iDescription)); }
  inline void commitUndoTx() { fUndoManager->commitUndoTx(); }
  inline void rollbackUndoTx() { fUndoManager->rollbackUndoTx(); }
//  void addRedoAction(std::shared_ptr<UndoAction> iRedoAction);
  std::shared_ptr<WidgetUndoAction> createWidgetUndoAction(Widget const *iWidget,
                                                           widget::Attribute const *iAttribute = nullptr,
                                                           std::optional<std::string> const &iDescription = std::nullopt) const;
  void addUndoAttributeChange(widget::Attribute const *iAttribute);
  void undoLastAction() { fUndoManager->undoLastAction(*this); }
  void redoLastAction() { fUndoManager->redoLastAction(*this); }

  inline Widget const *getCurrentWidget() const { return fCurrentWidget; }

  friend class PanelState;
  friend class Widget;
  friend class Application;

public:
  ShowBorder fShowBorder{ShowBorder::kNone};
  ShowCustomDisplay fShowCustomDisplay{ShowCustomDisplay::kMain};
  ShowSampleDropZone fShowSampleDropZone{ShowSampleDropZone::kFill};
  float fZoom{0.20f};

protected:
  void initPanels(std::string const &iDevice2DFile, std::string const &iHDGui2DFile);
  inline void setCurrentWidget(Widget const *iWidget) { fCurrentWidget = iWidget; }
  void render();

protected:
  std::shared_ptr<TextureManager> fTextureManager{};
  std::shared_ptr<UserPreferences> fUserPreferences{};
  std::shared_ptr<PropertyManager> fPropertyManager{};
  std::shared_ptr<UndoManager> fUndoManager{};
  std::unique_ptr<PanelState> fFrontPanel;
  std::unique_ptr<PanelState> fFoldedFrontPanel;
  std::unique_ptr<PanelState> fBackPanel;
  std::unique_ptr<PanelState> fFoldedBackPanel;
  bool fShowProperties{};
  bool fShowPanel{true};
  bool fShowPanelWidgets{true};
  bool fShowWidgets{};
  long fCurrentFrame{};
  PanelState *fCurrentPanelState{};
  Widget const *fCurrentWidget{};
};

}
#endif //RE_EDIT_APP_CONTEXT_H
