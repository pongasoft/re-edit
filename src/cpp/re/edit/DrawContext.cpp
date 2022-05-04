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

#include "DrawContext.h"

namespace re::edit {

//------------------------------------------------------------------------
// DrawContext::drawTexture
//------------------------------------------------------------------------
void DrawContext::drawTexture(Texture const *iTexture, ImVec2 const &iPosition, int iFrameNumber) const
{
  auto cp = ImGui::GetCursorPos();

  ImGui::SetCursorPos({cp.x + iPosition.x * fZoom, cp.y + iPosition.y * fZoom});

  if(iTexture->isValid())
  {
    auto frameWidth = static_cast<float>(iTexture->frameWidth());
    auto frameHeight = static_cast<float>(iTexture->frameHeight());
    auto frameY = frameHeight * static_cast<float>(iFrameNumber);
    auto width = static_cast<float>(iTexture->width());
    auto height = static_cast<float>(iTexture->height());

    ImGui::Image(iTexture->data(),
                 { frameWidth * fZoom, frameHeight * fZoom},
                 ImVec2(0, (frameY) / height),
                 ImVec2((0 + frameWidth) / width, (frameY + frameHeight) / height)
    );
  }
}

}