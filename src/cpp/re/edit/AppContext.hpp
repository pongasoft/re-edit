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


namespace re::edit {

//------------------------------------------------------------------------
// AppContext::addOrMergeUndoWidgetChange
//------------------------------------------------------------------------
template<typename T>
void AppContext::addOrMergeUndoWidgetChange(Widget const *iWidget,
                                            void *iMergeKey,
                                            T const &iOldValue,
                                            T const &iNewValue,
                                            std::string const &iDescription)
{
  addOrMergeUndoAction(iMergeKey, iOldValue, iNewValue, iDescription, [this, iWidget]() {
    auto action = std::make_unique<MergeableWidgetUndoAction<T>>();
    populateWidgetUndoAction(action.get(), iWidget);
    return action;
  });
}

}

#endif //RE_EDIT_APP_CONTEXT_HPP

