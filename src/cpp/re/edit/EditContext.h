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

#ifndef RE_EDIT_EDIT_CONTEXT_H
#define RE_EDIT_EDIT_CONTEXT_H

#include <vector>
#include <string>
#include <memory>
#include "TextureManager.h"
#include "UserPreferences.h"
#include "PropertyManager.h"

namespace re::edit {

class PanelState;
class Widget;

class EditContext
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

public:
  EditContext() = default;

  ImVec2 getPanelSize() const;

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

  void renderAddWidgetMenuView(ImVec2 const &iPosition = {});

  inline std::vector<std::string> const &getTextureKeys() const { return fTextureManager->getTextureKeys(); };
  inline std::vector<std::string> findTextureKeys(FilmStrip::Filter const &iFilter) const { return fTextureManager->findTextureKeys(iFilter); }
  inline std::shared_ptr<Texture> getTexture(std::string const &iKey) const { return fTextureManager->getTexture(iKey); };
  inline std::shared_ptr<Texture> findTexture(std::string const &iKey) const { return fTextureManager->findTexture(iKey); }
  inline std::shared_ptr<Texture> findHDTexture(std::string const &iKey) const { return fTextureManager->findHDTexture(iKey); }

  inline Widget *getCurrentWidget() const { return fCurrentWidget; }

  friend class PanelState;
  friend class Widget;

public:
  ShowBorder fShowBorder{ShowBorder::kNone};
  ShowCustomDisplay fShowCustomDisplay{ShowCustomDisplay::kMain};

protected:
  inline void setCurrentWidget(Widget *iWidget) { fCurrentWidget = iWidget; }

protected:
  PanelState *fPanelState{};
  std::shared_ptr<TextureManager> fTextureManager{};
  std::shared_ptr<UserPreferences> fUserPreferences{};
  std::shared_ptr<PropertyManager> fPropertyManager{};
  bool fShowProperties{};
  Widget *fCurrentWidget{};
};

}
#endif //RE_EDIT_EDIT_CONTEXT_H
