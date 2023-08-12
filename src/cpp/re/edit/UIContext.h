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

struct ImVec4;
struct Shader;

namespace re::edit {

class UIContext
{
public:
  using ui_action_t = std::function<void()>;

public:
  explicit UIContext(int iMaxTextureSize, std::thread::id iUIThreadId = std::this_thread::get_id());

  void init();

  static UIContext &GetCurrent() { RE_EDIT_INTERNAL_ASSERT(kCurrent != nullptr); return *kCurrent; }
  static bool HasCurrent() { return kCurrent != nullptr; }

  /**
   * Execute the provided action on the UI thread. If the current thread is the UI thread, then `iAction` is executed
   * synchronously. Otherwise, the action is enqueued and will be executed on the UI thread in the next frame loop. */
  void execute(ui_action_t iAction);

  void processUIActions();

  void beginFXShader(ImVec4 const &iTint, float iBrightness, float iContrast);
  void endFXShader();

  constexpr int maxTextureSize() const { return fMaxTextureSize; }

  inline static UIContext *kCurrent{};

private:
  struct RLShader
  {
    RLShader();
    RLShader(char const *iVertexShader, char const *iFragmentShader);
    RLShader(RLShader &&iOther) noexcept;
    ~RLShader();

    RLShader &operator=(RLShader &&iOther) noexcept;

    inline ::Shader *getShader() const { return fShader.get(); }

  private:
    std::unique_ptr<::Shader> fShader;
  };

private:
  int fMaxTextureSize;
  std::thread::id fUIThreadId;
  mutable std::mutex fMutex;
  RLShader fFXShader{};
  int fShaderTintLocation{};
  int fShaderBrightnessLocation{};
  int fShaderContrastLocation{};
  std::vector<ui_action_t> fUIActions{};
};

}

#endif //RE_EDIT_UI_CONTEXT_H