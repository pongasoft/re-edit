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

#ifndef RE_EDIT_UI_CONTEXT_H
#define RE_EDIT_UI_CONTEXT_H

#include "Errors.h"
#include <functional>
#include <vector>
#include <mutex>
#include <thread>

namespace re::edit {

class UIContext
{
public:
  using ui_action_t = std::function<void()>;

public:
  explicit UIContext(std::thread::id iUIThreadId = std::this_thread::get_id()) : fUIThreadId{iUIThreadId} {}

  static UIContext &GetCurrent() { RE_EDIT_INTERNAL_ASSERT(kCurrent != nullptr); return *kCurrent; }

  /**
   * Execute the provided action on the UI thread. If the current thread is the UI thread, then `iAction` is executed
   * synchronously. Otherwise, the action is enqueued and will be executed on the UI thread in the next frame loop. */
  void execute(ui_action_t iAction);

  void processUIActions();

  inline static UIContext *kCurrent{};

private:
  std::thread::id fUIThreadId;
  mutable std::mutex fMutex;
  std::vector<ui_action_t> fUIActions{};
};

}

#endif //RE_EDIT_UI_CONTEXT_H