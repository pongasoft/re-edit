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

#include "WidgetView.h"
#include <cmath>

namespace re::edit {

//------------------------------------------------------------------------
// WidgetView::draw
//------------------------------------------------------------------------
void WidgetView::draw(DrawContext &iCtx)
{
  if(fHidden)
    return;

  ImVec4 borderColor{};
  if(fSelected)
    borderColor = iCtx.getUserPreferences().fSelectedWidgetColor;
  else
  {
    if(iCtx.getUserPreferences().fShowWidgetBorder)
      borderColor = iCtx.getUserPreferences().fWidgetBorderColor;
  }

  iCtx.drawTexture(fTexture.get(), fPosition, fFrameNumber, borderColor);
  if(fError)
    iCtx.drawRectFilled(fPosition, fTexture->frameSize(), iCtx.getUserPreferences().fWidgetErrorColor);
}

//------------------------------------------------------------------------
// WidgetView::renderEdit
//------------------------------------------------------------------------
void WidgetView::editView(EditContext &iCtx)
{
  auto x = static_cast<int>(std::round(fPosition.x));
  ImGui::InputInt("x", &x, 1, 5);
  auto y = static_cast<int>(std::round(fPosition.y));
  ImGui::InputInt("y", &y, 1, 5);
  if(x != static_cast<int>(std::round(fPosition.x)) || y != static_cast<int>(std::round(fPosition.y)))
    fPosition = {static_cast<float>(x), static_cast<float>(y)};

  if(fTexture->numFrames() > 2)
  {
    ImGui::SliderInt("Frame", &fFrameNumber, 0, fTexture->numFrames() - 1);
  }

}

}