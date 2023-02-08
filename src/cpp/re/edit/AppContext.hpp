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

namespace re::edit {

//------------------------------------------------------------------------
// AppContext::execute
//------------------------------------------------------------------------
template<typename R>
R AppContext::execute(std::unique_ptr<ExecutableAction<R>> iAction)
{
  auto &&result = iAction->execute();
  if(isUndoEnabled() && iAction->isUndoEnabled())
  {
    addUndo(std::move(iAction));
  }
  return result;
}


//------------------------------------------------------------------------
// AppContext::executeAction
//------------------------------------------------------------------------
template<class T, class... Args>
typename T::result_t AppContext::executeAction(PanelType iPanelType, Args &&... args)
{
  auto action = std::make_unique<T>();
  action->setPanelType(iPanelType);
  action->init(std::forward<Args>(args)...);
  return execute<typename T::result_t>(std::move(action));
}

}

#endif //RE_EDIT_APP_CONTEXT_HPP

