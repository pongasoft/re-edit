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

#ifndef RE_EDIT_DRAWCONTEXT_H
#define RE_EDIT_DRAWCONTEXT_H

#include <imgui.h>
#include "TextureManager.h"

namespace re::edit {

class DrawContext
{
public:
  explicit DrawContext(std::shared_ptr<TextureManager> iTextureManager) : fTextureManager{std::move(iTextureManager)} {}

  constexpr float getZoom() const { return fZoom; }
  float &getZoom() { return fZoom; }
  constexpr void setZoom(float iZoom) { fZoom = iZoom; }

public:
  inline std::shared_ptr<Texture> getTexture(std::string const &iPath) const { return fTextureManager->getTexture(iPath); }
  void drawTexture(Texture const *iTexture, ImVec2 const &iPosition = {0,0}, int iFrameNumber = 0) const;

private:
  std::shared_ptr<TextureManager> fTextureManager;
  float fZoom{0.25f};
};

}

#endif //RE_EDIT_DRAWCONTEXT_H