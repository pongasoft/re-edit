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

#ifndef RE_EDIT_GRAPHICS_NODE_H
#define RE_EDIT_GRAPHICS_NODE_H

#include "WidgetAttribute.h"

namespace re::edit::widget::attribute {

class Graphics : public Attribute
{
public:
  Graphics() : Attribute("graphics") {}

  void hdgui2D(std::string const &iNodeName, attribute_list_t &oAttributes) const;

  inline bool contains(ImVec2 const &iPosition) const {
    auto size = getSize();
    return iPosition.x > fPosition.x
           && iPosition.y > fPosition.y
           && iPosition.x < fPosition.x + size.x
           && iPosition.y < fPosition.y + size.y;
  }

  constexpr ImVec2 getSize() const { return hasTexture() ? getTexture()->frameSize() : fSize; }
  constexpr ImVec2 getPosition() const { return fPosition; }
  constexpr ImVec2 getTopLeft() const { return fPosition; }
  constexpr ImVec2 getBottomRight() const { return fPosition + getSize(); }

  constexpr void setPosition(ImVec2 const &iPosition) { fPosition = iPosition; }

  constexpr void move(ImVec2 const &iDelta) { fPosition = fPosition + iDelta; }

  constexpr bool hasTexture() const { return getTexture() != nullptr; }
  constexpr Texture const *getTexture() const { return fTexture.get(); }
  void setTexture(std::shared_ptr<Texture> iTexture) { fTexture = std::move(iTexture); }
  void setSize(ImVec2 const &iSize) { fSize = iSize; }

  void reset() override;

  void editView(EditContext &iCtx) override;

  void editView(std::vector<std::string> const &iTextureKeys,
                const std::function<void()>& iOnReset,
                std::function<void(std::string const &)> const &iOnTextureUpdate,
                std::function<void(ImVec2 const &)> const &iOnSizeUpdate) const;

  void draw(DrawContext &iCtx, int iFrameNumber, const ImVec4& iBorderCol) const;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Graphics>(*this); }

public:
  ImVec2 fPosition{};
  std::shared_ptr<Texture> fTexture{};
  ImVec2 fSize{100, 100};
};

}

#endif //RE_EDIT_GRAPHICS_NODE_H
