/*
 * Copyright (c) 2023 pongasoft
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

#ifndef RE_EDIT_APP_CONTEXT_HPP
#define RE_EDIT_APP_CONTEXT_HPP

#include "AppContext.h"
#include "Widget.h"
#include "Panel.h"
#include "TextureManager.h"
#include <imgui.h>

namespace re::edit {

//------------------------------------------------------------------------
// AppContext::executeAction
//------------------------------------------------------------------------
template<class T, class... Args>
typename T::result_t AppContext::executeAction(PanelType iPanelType, Args &&... args)
{
  auto action = std::make_unique<T>();
  action->setPanelType(iPanelType);
  action->init(std::forward<Args>(args)...);
  return fUndoManager->execute<typename T::result_t, typename T::action_t>(std::move(action));
}

//------------------------------------------------------------------------
// AppContext::textureMenu
//------------------------------------------------------------------------
template<typename F>
bool AppContext::textureMenu(FilmStrip::Filter const &iFilter, F &&f)
{
  bool res = false;
  auto const keys = fTextureManager->findTextureKeys(iFilter);
  for(auto const &key: keys)
  {
    if(ImGui::MenuItem(key.c_str()))
    {
      f(key);
      res = true;
    }
    if(ReGui::ShowQuickView())
      textureTooltip(key);
  }
  return res;
}


}

#endif //RE_EDIT_APP_CONTEXT_HPP

