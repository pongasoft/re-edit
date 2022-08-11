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

namespace re::edit {

struct HitBoundaries
{
  friend bool operator==(HitBoundaries const &lhs, HitBoundaries const &rhs);
  friend bool operator!=(HitBoundaries const &lhs, HitBoundaries const &rhs);

  float fLeftInset{};
  float fTopInset{};
  float fRightInset{};
  float fBottomInset{};
};

}

namespace re::edit::widget::attribute {

class Graphics : public Attribute
{
public:
  Graphics() : Attribute("graphics") {}

  void hdgui2D(EditContext &iCtx, attribute_list_t &oAttributes) const override;
  void hdgui2D(std::string const &iNodeName, attribute_list_t &oAttributes) const;

  std::string device2D() const;

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

  constexpr void setPosition(ImVec2 const &iPosition) { fPosition = iPosition; fEdited = true; }

  constexpr void setHitBoundaries(HitBoundaries const &iHitBoundaries) { fHitBoundaries = iHitBoundaries; fEdited = true; }
  constexpr bool hasHitBoundaries() const { return fHitBoundaries.fLeftInset > 0 || fHitBoundaries.fTopInset > 0 || fHitBoundaries.fRightInset > 0 || fHitBoundaries.fBottomInset > 0; }

  constexpr void move(ImVec2 const &iDelta) { fPosition = fPosition + iDelta; fEdited = true; }

  constexpr bool hasTexture() const { return getTexture() != nullptr; }
  constexpr Texture const *getTexture() const { return fTexture.get(); }
  void setTexture(std::shared_ptr<Texture> iTexture) { fTexture = std::move(iTexture); fEdited = true; }
  void setSize(ImVec2 const &iSize) { fSize = iSize; fEdited = true; }

  void reset() override;

  void editPositionView(EditContext &iCtx);
  void editView(EditContext &iCtx) override;
  void editHitBoundariesView(EditContext &iCtx);

  void editView(EditContext &iCtx,
                FilmStrip::Filter const &iFilter,
                std::function<void(std::string const &)> const &iOnTextureUpdate,
                std::function<void(ImVec2 const &)> const &iOnSizeUpdate);

  error_t checkForErrors(EditContext &iCtx) const override;

  void draw(DrawContext &iCtx, const ImVec4& iBorderCol) const;
  void drawBorder(DrawContext &iCtx, const ImVec4& iBorderCol) const;
  void drawHitBoundaries(DrawContext &iCtx, const ImVec4& iColor) const;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Graphics>(*this); }

public:
  ImVec2 fPosition{};
  HitBoundaries fHitBoundaries{};
  bool fHitBoundariesEnabled{true};
  std::shared_ptr<Texture> fTexture{};
  ImVec2 fSize{100, 100};
  FilmStrip::Filter fFilter{};
  int fFrameNumber{};
};

class Background : public String
{
public:
  explicit Background(char const *iName) : String(iName) {}

  std::string getValueAsLua() const override;

  void editView(EditContext &iCtx) override;
  bool draw(DrawContext &iCtx, Graphics const *iParent) const;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Background>(*this); }
};

}

#endif //RE_EDIT_GRAPHICS_NODE_H
