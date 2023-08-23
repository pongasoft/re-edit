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
#include "fx.h"
#include <optional>
#include <variant>

namespace re::edit {

class Panel;

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
  std::string device2D() const;

  inline bool hasTexture() const { return fDNZTexture != nullptr; }
  inline bool hasValidTexture() const { return fDNZTexture && fDNZTexture->isValid(); }
  inline Texture const *getTexture() const { RE_EDIT_INTERNAL_ASSERT(fDNZTexture != nullptr); return fDNZTexture.get(); }
  inline Texture::key_t getTextureKey() const { return fTextureKey; }
  void setTextureKey(Texture::key_t const &iTextureKey) { fTextureKey = iTextureKey; fDNZTexture = AppContext::GetCurrent().getTexture(iTextureKey); fEdited = true; }
  void initTextureKey(Texture::key_t const &iTextureKey, std::optional<Texture::key_t> const &iOriginalTextureKey, texture::FX const &iEffects);
  void setEffects(texture::FX const &iEffects) { fEffects = iEffects; fEdited = true; }
  bool isSizeValid() const;

  void collectFilmStripEffects(std::vector<FilmStripFX> &oEffects) const;
  void collectUsedTexturePaths(std::set<fs::path> &oPaths) const;
  void collectAllUsedTextureKeys(std::set<FilmStrip::key_t> &oKeys) const;

  void reset();
  void editView(AppContext &iCtx);
  void findErrors(AppContext &iCtx, UserError &oErrors) const override;

  std::unique_ptr<Graphics> clone() const { return std::make_unique<Graphics>(Graphics(*this)); }

  bool operator==(Graphics const &rhs) const
  {
    return fTextureKey == rhs.fTextureKey &&
           fEffects == rhs.fEffects;
  }

  bool operator!=(Graphics const &rhs) const
  {
    return !(rhs == *this);
  }

  friend class re::edit::Panel;

public:
  Texture::key_t fTextureKey{};
  std::shared_ptr<Texture> fDNZTexture{};
  FilmStrip::Filter fFilter{};
  texture::FX fEffects{};

private:
  explicit Graphics(Panel *iParent) : fParent{iParent} {}

private:
  Panel *fParent;
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
  void collectFilmStripEffects(std::vector<FilmStripFX> &oEffects) const override;
  void collectUsedTexturePaths(std::set<fs::path> &oPaths) const override;
  void collectAllUsedTextureKeys(std::set<FilmStrip::key_t> &oKeys) const override;
  void collectUsedTextureBuiltIns(std::set<FilmStrip::key_t> &oKeys) const override;

  std::string device2D() const;

  std::string toValueString() const override;

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

  inline ImVec2 getSize() const { return hasSize() ? std::get<ImVec2>(fTexture) :
                                         fEffects.hasSizeOverride() ? *fEffects.fSizeOverride : getTexture()->frameSize(); }
  constexpr ImVec2 getPosition() const { return fPosition; }
  constexpr ImVec2 getTopLeft() const { return fPosition; }
  constexpr ImVec2 getBottomRight() const { return fPosition + getSize(); }

  constexpr void setPosition(ImVec2 const &iPosition) { fPosition = iPosition; fEdited = true; }

  constexpr void setHitBoundaries(HitBoundaries const &iHitBoundaries) { fHitBoundaries = iHitBoundaries; fEdited = true; }
  constexpr bool hasHitBoundaries() const { return fHitBoundaries.fLeftInset > 0 || fHitBoundaries.fTopInset > 0 || fHitBoundaries.fRightInset > 0 || fHitBoundaries.fBottomInset > 0; }

  constexpr void move(ImVec2 const &iDelta) { fPosition = fPosition + iDelta; fEdited = true; }

  constexpr bool hasTexture() const { return std::holds_alternative<Texture::key_t>(fTexture); }
  inline bool hasValidTexture() const { return hasTexture() && getTexture()->isValid(); }
  constexpr bool hasSize() const { return std::holds_alternative<ImVec2>(fTexture); }

  inline Texture const *getTexture() const { RE_EDIT_INTERNAL_ASSERT(fDNZTexture != nullptr); return fDNZTexture.get(); }
  inline Texture::key_t getTextureKey() const { return std::get<Texture::key_t>(fTexture); }
  void setTextureKey(Texture::key_t const &iTextureKey); // action only
  void updateTextureKey(Texture::key_t const &iTextureKey); // action with undo
  void initTextureKey(Texture::key_t const &iTextureKey,
                      std::optional<Texture::key_t> const &iOriginalTextureKey,
                      texture::FX const &iEffects);
  void setSize(ImVec2 const &iSize);

  void reset() override;

  void editPositionView(AppContext &iCtx);
  void editView(AppContext &iCtx) override;
  void editHitBoundariesView(AppContext &iCtx);

  void editView(AppContext &iCtx,
                FilmStrip::Filter const &iFilter,
                std::function<void(std::string const &)> const &iOnTextureUpdate,
                std::function<void(ImVec2 const &)> const &iOnSizeUpdate,
                std::function<void(char const *iName, texture::FX const &fx, MergeKey const &iMergeKey)> const &iOnFXUpdate);

  void findErrors(AppContext &iCtx, UserError &oErrors) const override;

  void draw(AppContext &iCtx, ReGui::Canvas &iCanvas, ImU32 iBorderColor, bool iXRay) const;
  void drawBorder(ReGui::Canvas &iCanvas, ImU32 iBorderColor) const;
  void drawHitBoundaries(ReGui::Canvas &iCanvas, ImU32 iColor) const;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Graphics>(*this); }

  bool eq(Attribute const *iAttribute) const override
  {
    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) {
      return l->fPosition == r->fPosition &&
             l->fHitBoundaries == r->fHitBoundaries &&
             l->fTexture == r->fTexture &&
             l->fEffects == r->fEffects;
    });
  }

  bool copyFromAction(Attribute const *iFromAttribute) override;

protected:
  inline ImVec2 getOriginalSize() const { return hasTexture() ? fDNZTexture->frameSize() : getSize(); }

public:
  ImVec2 fPosition{};
  HitBoundaries fHitBoundaries{};
  bool fHitBoundariesEnabled{true};
  std::variant<ImVec2, Texture::key_t> fTexture{kNoGraphics};
  bool fSizeEnabled{true};
  bool fCheckForOOBError{true};
  std::shared_ptr<Texture> fDNZTexture{};
  FilmStrip::Filter fFilter{};
  int fFrameNumber{};

  texture::FX fEffects{};
};

class Background : public String
{
public:
  explicit Background(char const *iName) : String(iName) {}

  std::string getValueAsLua() const override;
  void collectUsedTexturePaths(std::set<fs::path> &oPaths) const override;
  void collectAllUsedTextureKeys(std::set<FilmStrip::key_t> &oKeys) const override;

  std::string toValueString() const override { return fmt::printf("%s = \"%s\"", fName, fValue); }

  void editView(AppContext &iCtx) override;
  bool draw(AppContext &iCtx, ReGui::Canvas &iCanvas, Graphics const *iParent, ImU32 iBorderColor, bool xRay) const;

  std::unique_ptr<Attribute> clone() const override { return Attribute::clone<Background>(*this); }

  bool eq(Attribute const *iAttribute) const override
  {
    return Attribute::eq(this, iAttribute, [](auto *l, auto *r) { return l->fValue == r->fValue;});
  }

};

}

#endif //RE_EDIT_GRAPHICS_NODE_H
