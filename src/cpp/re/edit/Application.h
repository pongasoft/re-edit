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

#ifndef RE_EDIT_APPLICATION_H
#define RE_EDIT_APPLICATION_H

#include "TextureManager.h"
#include "imgui.h"
#include "DrawContext.h"
#include "PanelView.h"

namespace re::edit {

class Application
{
public:
  explicit Application(std::shared_ptr<TextureManager> const &iTextureManager);

  void init();

  bool loadFilmStrip(char const *iPath, int iNumFrames = 1);

  void render();

  float clear_color[4] = {0.45f, 0.55f, 0.60f, 1.00f};

private:
  struct PanelState
  {
    explicit PanelState(std::shared_ptr<TextureManager> iTextureManager);
    DrawContext fDrawContext;
    PanelView fPanelView{};
    bool fVisible{true};
  };

private:
  std::shared_ptr<TextureManager> fTextureManager;
  PanelState fFrontPanel;
  bool show_demo_window{false};
};

}

#endif //RE_EDIT_APPLICATION_H