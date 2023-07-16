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

#include "UIContext.h"

namespace re::edit {

//------------------------------------------------------------------------
// GPUContext::execute
//------------------------------------------------------------------------
void UIContext::execute(ui_action_t iAction)
{
  if(std::this_thread::get_id() == fUIThreadId)
    iAction();
  else
  {
    std::lock_guard<std::mutex> lock(fMutex);
    fUIActions.emplace_back(std::move(iAction));
  }
}

//------------------------------------------------------------------------
// GPUContext::processUIActions
//------------------------------------------------------------------------
void UIContext::processUIActions()
{
  std::unique_lock<std::mutex> lock(fMutex);
  auto actions = std::move(fUIActions);
  lock.unlock();

  for(auto &action: actions)
    action();
}

}