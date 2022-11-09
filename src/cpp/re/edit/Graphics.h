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

namespace re::edit::panel {

class Graphics : public Editable
{
public:
  Graphics() = default;

  std::string device2D() const;

  inline bool hasTexture() const { return fDNZTexture != nullptr; }
  inline bool hasValidTexture() const { return fDNZTexture && fDNZTexture->isValid(); }
  inline Texture const *getTexture() const { RE_EDIT_INTERNAL_ASSERT(fDNZTexture != nullptr); return fDNZTexture.get(); }
  inline Texture::key_t getTextureKey() const { return fTextureKey; }
  void setTextureKey(Texture::key_t const &iTextureKey) { fTextureKey = iTextureKey; fDNZTexture = AppContext::GetCurrent().getTexture(iTextureKey); fEdited = true; }

  void reset();
  void editView(AppContext &iCtx);
  void findErrors(AppContext &iCtx, UserError &oErrors) const override;

public:
  Texture::key_t fTextureKey{};
  std::shared_ptr<Texture> fDNZTexture{};
  FilmStrip::Filter fFilter{};
};

}

namespace re::edit::widget::attribute {

namespace impl {

constexpr bool isContained(ImVec2 const &iPosition, ImVec2 const &iTopLeft, ImVec2 const &iBottomRight)
{
  return iPosition.x >= iTopLeft.x
      && iPosition.y >= iTopLeft.y
      && iPosition.x <= iBottomRight.x
      && iPosition.y <= iBottomRight.y;
}

}

class Graphics : public Attribute
{
public:
  Graphics() : Attribute("graphics") {}

  void hdgui2D(attribute_list_t &oAttributes) const override;
  void hdgui2D(std::string const &iNodeName, attribute_list_t &oAttributes) const;
  void getUsedTexturePaths(std::set<fs::path> &oPaths) const override;

  std::string device2D() const;

  inline bool contains(ImVec2 const &iPosition) const { return impl::isContained(iPosition, fPosition, getBottomRight()); };

  inline bool overlaps(ImVec2 const &iTopLeft, ImVec2 const &iBottomRight) const {
    auto const &topLeft = fPosition;
    auto bottomRight = getBottomRight();
    if(topLeft.x > iBottomRight.x)
      return false;
    if(bottomRight.x < iTopLeft.x)
      return false;
    if(topLeft.y > iBottomRight.y)
      return false;
    if(bottomRight.y < iTopLeft.y)
      return false;
    return true;
  }

  inline ImVec2 getSize() const { return hasTexture() ? getTexture()->frameSize() : std::get<ImVec2>(fTexture); }
  constexpr ImVec2 getPosition() const { return fPosition; }
  constexpr ImVec2 getTopLeft() const { return fPosition; }
  constexpr ImVec2 getBottomRight() const { return fPosition + getSize(); }

  constexpr void setPosition(ImVec2 const &iPosition) { fPosition = iPosition; fEdited = true; }

  constexpr void setHitBoundaries(HitBoundaries const &iHitBoundaries) { fHitBoundaries = iHitBoundaries; fEdited = true; }
  constexpr bool hasHitBoundaries() const { return fHitBoundaries.fLeftInset > 0 || fHitBoundaries.fTopInset > 0 || fHitBoundaries.fRightInset > 0 || fHitBoundaries.fBottomInset > 0; }

  constexpr void move(ImVec2 const &iDelta) { fPosition = fPosition + iDelta; fEdited = true; }

  inline bool hasTexture() const { return std::holds_alternative<Texture::key_t>(fTexture); }
  inline bool hasSize() const { return std::holds_alternative<ImVec2>(fTexture); }

  inline Texture const *getTexture() const { RE_EDIT_INTERNAL_ASSERT(fDNZTexture != nullptr); return fDNZTexture.get(); }
  inline Texture::key_t getTextureKey() const { return std::get<Texture::key_t>(fTexture); }
  void setTextureKey(Texture::key_t const &iTextureKey) { fTexture = iTextureKey; fDNZTexture = AppContext::GetCurrent().getTexture(iTextureKey); fEdited = true; }
  void setSize(ImVec2 const &iSize) { fTexture = iSize; fDNZTexture.reset(); fEdited = true; }

  void reset() override;

  void editPositionView(AppContext &iCtx);
  void editView(AppContext &iCtx) override;
  void editHitBoundariesView(AppContext &iCtx);

  void editView(AppContext &iCtx,
                FilmStrip::Filter const &iFilter,
                std::function<void(std::string const &)> const &iOnTextureUpdate,
                std::function<void(ImVec2 const &)> const &iOnSizeUpdate);

  void findErrors(AppContext &iCtx, UserError &oErrors) const override;

  void draw(AppContext &iCtx, ImU32 iBorderColor, bool iXRay) const;
  void drawBorder(AppContext &iCtx, ImU32 iBorderColor) const;
  void drawHitBoundaries(AppContext &iCtx, ImU32 iColor) const;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Graphics>(*this); }

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) {
//      return l->fPosition == r->fPosition &&
//             l->fHitBoundaries == r->fHitBoundaries &&
//             l->fTexture == r->fTexture &&
//             l->fSize == r->fSize;
//    });
//  }

  bool copyFrom(Attribute const *iFromAttribute) override;

public:
  ImVec2 fPosition{};
  HitBoundaries fHitBoundaries{};
  bool fHitBoundariesEnabled{true};
  std::variant<ImVec2, Texture::key_t> fTexture{kNoGraphics};
  bool fSizeEnabled{true};
  std::shared_ptr<Texture> fDNZTexture{};
  FilmStrip::Filter fFilter{};
  int fFrameNumber{};
};

class Background : public String
{
public:
  explicit Background(char const *iName) : String(iName) {}

  std::string getValueAsLua() const override;
  void getUsedTexturePaths(std::set<fs::path> &oPaths) const override;

  void editView(AppContext &iCtx) override;
  bool draw(AppContext &iCtx, Graphics const *iParent, ImU32 iBorderColor, bool xRay) const;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Background>(*this); }

//  bool eq(Attribute const *iAttribute) const override
//  {
//    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) { return l->fValue == r->fValue;});
//  }

};

}

#endif //RE_EDIT_GRAPHICS_NODE_H
