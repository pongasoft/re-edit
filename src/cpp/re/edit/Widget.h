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

#ifndef RE_EDIT_WIDGET_H
#define RE_EDIT_WIDGET_H

#include "View.h"

namespace re::edit {

class Widget : public View
{
public:
  Widget() = default;
  Widget(ImVec2 iPosition, std::shared_ptr<Texture> iTexture) : fPosition{iPosition}, fTexture{std::move(iTexture)} {}
  ~Widget() override = default;

  constexpr ImVec2 getPosition() const { return fPosition; }
  constexpr void setPosition(ImVec2 const &iPosition) { fPosition = iPosition; }

  constexpr bool isSelected() const { return fSelected; }
  constexpr void setSelected(bool iSelected) { fSelected = iSelected; }
  constexpr void toggleSelection() { fSelected = !fSelected; }

  constexpr void move(ImVec2 const &iDelta) { fPosition = fPosition + iDelta; }

  inline void setTexture(std::shared_ptr<Texture> iTexture) { fTexture = std::move(iTexture); }
  constexpr void setFrameNumber(int iFrameNumber) { fFrameNumber = iFrameNumber; }

  constexpr bool contains(ImVec2 const &iPosition)
  {
    return iPosition.x > fPosition.x
           && iPosition.y > fPosition.y
           && iPosition.x < fPosition.x + fTexture->frameWidth()
           && iPosition.y < fPosition.y + fTexture->frameHeight();
  }

  void draw(DrawContext &iCtx) override;

protected:
  ImVec2 fPosition{};
  std::shared_ptr<Texture> fTexture{};
  int fFrameNumber{};
  bool fSelected{false};
};

class AnalogKnobWidget : public Widget
{

};

}

#endif //RE_EDIT_WIDGET_H